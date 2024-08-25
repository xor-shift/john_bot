#include <alloc.hpp>
#include <as_expected.hpp>
#include <common.hpp>
#include <error.hpp>
#include <irc/message.hpp>
#include <try.hpp>

#include <spdlog/spdlog.h>
#include <magic_enum.hpp>

#include <boost/asio.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

// pure ass
namespace john::assio {

using default_token = as_expected_t<boost::asio::use_awaitable_t<>>;

namespace tcp {
using socket = default_token::as_default_on_t<boost::asio::ip::tcp::socket>;
using resolver = default_token::as_default_on_t<boost::asio::ip::tcp::resolver>;
}  // namespace tcp

namespace udp {
using socket = default_token::as_default_on_t<boost::asio::ip::udp::socket>;
using resolver = default_token::as_default_on_t<boost::asio::ip::udp::resolver>;
}  // namespace udp

}  // namespace john::assio

namespace john {

struct backend {
    virtual auto worker() -> asio::awaitable<void>;
};

}  // namespace john

namespace john::irc {

struct configuration {
    std::string m_identifier;
    std::vector<std::string> m_nicks;
    std::string m_user;
    std::string m_realname;

    std::vector<std::string> m_channels;
};

inline static const auto g_config = configuration{
  .m_identifier = "irc_retardnet_bohnjot",
  .m_nicks{"das", "BohnJot"},
  .m_user = "BohnJot",
  .m_realname = "Bohn Jot",
  .m_channels = {"#retard"},
};

void print_irc_message(message_view const& message) {
    spdlog::debug("raw message: \"{}\"", message.m_original_message);

    auto print_optional = []<typename T>(std::string_view name, std::optional<T> const& v) {
        if (!v) {
            spdlog::debug("{} is a std::nullopt", name);
        } else {
            spdlog::debug("{}: \"{}\"", name, *v);
        }
    };

    print_optional("name", message.m_prefix_name);
    print_optional("user", message.m_prefix_user);
    print_optional("host", message.m_prefix_host);

    spdlog::debug("command: {}", message.m_command);

    for (auto i = 0uz; auto const& param : message.params()) {
        spdlog::debug("param #{}: \"{}\"", i++, param);
    }

    print_optional("trailing", message.m_trailing);
}

namespace state {

struct disconnected {};

struct connected {
    usize m_nick_try;
};

struct registered {};

struct failure_connection {};
struct failure_registration {};

}  // namespace state

using state_t = std::variant<state::disconnected, state::connected, state::registered, state::failure_connection, state::failure_registration>;

struct irc_client {
    irc_client(asio::any_io_executor& executor, usize max_message_length = 512, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : m_executor(executor)
        , m_resource(resource)
        , m_incoming_buffer(john::create_n<char>(resource, max_message_length), max_message_length)
        , m_socket(m_executor) {}

    ~irc_client() {
        john::destroy(m_resource, m_incoming_buffer.data(), m_incoming_buffer.size());
        m_incoming_buffer = {};
    }

    irc_client(irc_client const&) = delete;
    irc_client(irc_client&&) = delete;

    auto run() -> asio::awaitable<anyhow::result<void>> {
        for (auto try_no = 1uz;; try_no++) {
            auto res = co_await run_inner();

            if (!res) {
                spdlog::error("IRC loop exited with error: {}", res.error().what());

                const auto* const ordinal_indicator =
                  (try_no >= 10 && try_no < 20) || (try_no % 10) == 0 || (try_no % 10) > 3 ? "th" : (const char*[]){"st", "nd", "rd"}[try_no % 10 - 1];
                spdlog::error("retrying for the {}{} time in 5 seconds.", try_no, ordinal_indicator);

                co_await asio::deadline_timer(m_executor, boost::posix_time::seconds(5)).async_wait();
                continue;
            }
        }
    }

private:
    asio::any_io_executor& m_executor;
    std::pmr::memory_resource* m_resource;

    state_t m_state = state::disconnected{};

    bool m_error_cleanup = false;
    std::span<char> m_incoming_buffer;
    usize m_incoming_buffer_usage = 0uz;

    assio::tcp::socket m_socket;

    auto run_inner() -> asio::awaitable<std::expected<void, boost::system::error_code>> {
        auto resolver = assio::tcp::resolver(m_executor);

        auto endpoint = TRYC(co_await resolver.async_resolve("198.18.0.1", "6667"));
        co_await m_socket.async_connect(endpoint->endpoint());

        co_await state_change(state::connected{});

        for (;;) {
            auto unused_buffer = std::span{m_incoming_buffer.data() + m_incoming_buffer_usage, m_incoming_buffer.size() - m_incoming_buffer_usage};
            auto read_byte_ct = TRYC(co_await m_socket.async_read_some(asio::buffer(unused_buffer)));

            m_incoming_buffer_usage += read_byte_ct;

            for (;;) {
                const auto used_buffer = std::string_view{m_incoming_buffer.data(), m_incoming_buffer_usage};
                const auto message_suffix_start = used_buffer.find("\r\n");
                if (message_suffix_start == std::string_view::npos) {
                    break;
                }

                const auto raw_message = used_buffer.substr(0, message_suffix_start + 2);
                auto parse_result = message_view::from_chars(raw_message);

                if (!parse_result) {
                    spdlog::warn("failed to parse IRC message: {}", raw_message);
                    spdlog::warn("reason: {}", parse_result.error().description());
                } else {
                    asio::co_spawn(m_executor, message_handler(*parse_result), asio::detached);
                }

                m_incoming_buffer_usage -= raw_message.size();
                std::fill(std::copy(m_incoming_buffer.begin() + raw_message.size(), m_incoming_buffer.end(), m_incoming_buffer.begin()), m_incoming_buffer.end(), 0);
            }
        }

        co_return std::expected<void, boost::system::error_code>{};
    }

    auto message_handler(message_view const& message) -> asio::awaitable<void> {
        print_irc_message(message);

        if (message.m_command == reply{"PING"}) {
            if (message.m_trailing) {
                co_await send_message(message::bare("PONG").with_trailing(std::string(*(message.m_trailing))));
            } else {
                co_await send_message(message::bare("PONG"));
            }
        }

        const auto state_visitor = multi_visitor{
          [&](state::connected) -> asio::awaitable<void> {
              if (message.m_command == reply{001}) {
                  co_await state_change(state::registered{});
              } else if (message.m_command == reply{numeric_reply::ERR_NICKNAMEINUSE}) {
                  co_await state_change(state::failure_registration{});
              }
          },
          [&](auto const&) -> asio::awaitable<void> { co_return; },
        };
        co_await std::visit(state_visitor, m_state);

        co_return;
    }

    auto state_change(state_t new_state) -> asio::awaitable<void> {
        const auto old_state = m_state;
        m_state = new_state;

        auto try_register = [&](std::string_view nick, std::string_view user, std::string_view realname) -> asio::awaitable<void> {
            co_await send_message(message::bare("NICK").with_param(std::string(nick)));
            co_await send_message(message::bare("USER")  //
                                    .with_param(g_config.m_user)
                                    .with_param("*")
                                    .with_param("*")
                                    .with_trailing(g_config.m_realname));
        };

        auto try_register_n = [&](usize n) { return try_register(g_config.m_nicks[n], g_config.m_user, g_config.m_realname); };

        const auto state_visitor = multi_visitor{
          [&](state::connected& state) -> asio::awaitable<void> {
              if (state.m_nick_try >= g_config.m_nicks.size()) {
                  spdlog::error("ran out of nicks to try, oh well!");
                  co_await state_change(state::failure_registration{});
              }
              co_await try_register_n(state.m_nick_try);
          },
          [&](state::failure_registration) -> asio::awaitable<void> {
              if (!std::holds_alternative<state::connected>(old_state)) {
                  spdlog::error(
                    "a supposed registration failure despite the previous state being something other than state::connected?? how abnormal!! ive never seen such a thing- i must "
                    "inquire this further with my supervisor post-haste!!"
                  );
                  co_return;
              }

              auto prev_state = std::get<state::connected>(old_state);
              if (prev_state.m_nick_try >= g_config.m_nicks.size()) {
                  spdlog::error("not going to try anymore to register");
              }

              prev_state.m_nick_try++;

              spdlog::error("registration with the nick \"{}\" failed, going to try \"{}\"", g_config.m_nicks[prev_state.m_nick_try - 1], g_config.m_nicks[prev_state.m_nick_try]);

              co_await state_change(prev_state);
          },
          [&](state::registered) -> asio::awaitable<void> {
              for (auto const& chan : g_config.m_channels) {
                  co_await send_message(irc::message::bare("JOIN").with_param(chan));
              }
          },
          [&](auto const&) -> asio::awaitable<void> { co_return; },
        };
        co_await std::visit(state_visitor, m_state);

        co_return;
    }

    auto send_message(message const& msg) -> asio::awaitable<void> {
        auto strings_to_send = msg.encode();

        for (auto const& str : strings_to_send) {
            spdlog::trace("sending a message with command {}", msg.m_command);
            co_await m_socket.async_send(asio::const_buffer{str.data(), str.size()});
        }
    }
};

}  // namespace john::irc

auto async_main() -> asio::awaitable<void> {
    auto executor = co_await asio::this_coro::executor;
    auto client = john::irc::irc_client{executor};
    auto res = co_await client.run();

    if (!res) {
        spdlog::error("{}", res.error().description());
    }
}

auto main() -> int {
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Hello, world!");

    auto context = asio::io_context{1};

    asio::co_spawn(context, async_main(), asio::detached);

    context.run();
}
