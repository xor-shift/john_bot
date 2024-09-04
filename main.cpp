#include <irc/client.hpp>

#include <alloc.hpp>
#include <assio/as_expected.hpp>
#include <error.hpp>
#include <telegram/client.hpp>

#include <stuff/core/integers.hpp>
#include <stuff/core/try.hpp>
#include <stuff/core/visitor.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <boost/mysql.hpp>
#include <magic_enum.hpp>

struct echoer final : john::thing {
    ~echoer() override = default;

    auto worker(john::bot& bot) -> boost::asio::awaitable<void> override {
        m_bot = &bot;
        co_return;
    }

    auto handle(john::internal_message const& message) -> boost::asio::awaitable<void> override {
        const auto visitor = stf::multi_visitor{
          [&](john::incoming_message const& message) -> boost::asio::awaitable<void> {
              /*co_await m_bot->new_message(john::outgoing_message{
                .m_target = message.m_return_to_sender,
                .m_content = message.m_content,
              });*/

              using namespace std::string_literals;
              using namespace std::string_view_literals;

              const auto try_relay = [this, &message](auto const& from, auto const& to, auto const& when) -> boost::asio::awaitable<void> {
                  if (from.strongly_equals(when)) {
                      co_await m_bot->new_message(john::outgoing_message{
                        .m_target = to,
                        .m_content = message.m_content,
                      });
                  }

                  co_return;
              };

              for (auto const& [k, v] : m_mappings) {
                  co_await try_relay(message.m_return_to_sender, v, k);
                  co_await try_relay(message.m_return_to_sender, k, v);
              }

              const auto tokens = std::vector(
                std::from_range,                     //
                std::string_view(message.m_content)  //
                  | std::views::split(" "sv)         //
                  | std::views::transform([](auto const& v) { return std::string_view(v); })
              );

              if (tokens.empty()) {
                  co_return;
              }

              auto break_up_token = std::views::split(";"sv)  //
                                  | std::views::transform([](auto str_) {
                                        auto str = std::string_view(str_);
                                        return std::pair{str.substr(0, str.find(':')), str.substr(str.find(':') + 1)};
                                    });

              if (tokens.front() == "!addmap"sv) {
                  auto from = john::mini_kv(std::from_range, std::string_view(tokens[1]) | break_up_token);
                  auto to = john::mini_kv(std::from_range, std::string_view(tokens[2]) | break_up_token);

                  m_mappings.emplace_back(std::move(from), std::move(to));
              }

              co_return;
          },
          [](auto const&) -> boost::asio::awaitable<void> { co_return; }
        };

        co_await std::visit(visitor, message);
    }

private:
    john::bot* m_bot = nullptr;

    std::vector<std::pair<john::mini_kv, john::mini_kv>> m_mappings{};
};

auto launch_bot() -> boost::asio::awaitable<anyhow::result<void>> {
    namespace asio = boost::asio;
    using boost::asio::ip::tcp;

    using resolver = john::assify<tcp::resolver>;

    auto executor = co_await asio::this_coro::executor;

    auto endpoints = TRYC(co_await resolver(executor).async_resolve("127.0.0.1", boost::mysql::default_port_string));

    auto connection = john::assify<boost::mysql::tcp_connection>(executor);
    TRYC(co_await connection.async_connect(*endpoints.begin(), {"john", "password", "john"}));

    auto bot = john::bot{};

    auto results = boost::mysql::results{};

    for (TRYC(co_await connection.async_execute("select * from clients_telegram", results)); auto const& row : results.rows()) {
        const auto bot_id = row.at(0).as_int64();
        const auto bot_token = std::string_view(row.at(1).as_string());

        bot.add_thing<john::telegram::client>(
          executor,
          john::telegram::configuration{
            .m_identifier = fmt::format("telegram_{}", bot_id),
            .m_token = fmt::format("{}:{}", bot_id, bot_token),
            .m_host = "api.telegram.org",
          }
        );
    }

    auto nicks_stmt = TRYC(co_await connection.async_prepare_statement("select * from irc_nick_choices where irc_id = ?"));
    auto channels_stmt = TRYC(co_await connection.async_prepare_statement("select * from irc_channels where irc_id = ?"));

    for (TRYC(co_await connection.async_execute("select * from clients_irc", results)); auto const& row : results.rows()) {
        const auto id = row.at(0).as_int64();

        const auto host = std::string_view(std::string_view(row.at(1).as_string()));
        const auto port = row.at(2).as_uint64();

        auto nicks_result = boost::mysql::results {};
        TRYC(co_await connection.async_execute(nicks_stmt.bind(id), nicks_result));
        
        auto nicks = std::vector<std::string> {};
        nicks.reserve(nicks_result.size());
        std::transform(nicks_result.rows().begin(), nicks_result.rows().end(), std::back_inserter(nicks), [](auto const& row) { return std::string(row.at(2).as_string()); });

        auto channels_result = boost::mysql::results {};
        TRYC(co_await connection.async_execute(channels_stmt.bind(id), channels_result));

        auto channels = std::vector<std::string> {};
        channels.reserve(channels_result.size());
        std::transform(channels_result.rows().begin(), channels_result.rows().end(), std::back_inserter(channels), [](auto const& row) { return std::string(row.at(2).as_string()); });

        auto config = john::irc::configuration {
            .m_identifier = fmt::format("irc_{}", id),
            .m_nicks = std::move(nicks),
            .m_user = std::string(row.at(3).as_string()),
            .m_realname = std::string(row.at(4).as_string()),
            .m_channels = std::move(channels),
        };

        bot.add_thing<john::irc::irc_client>(executor, std::move(config));
    }

    auto res = co_await bot.run();

    if (!res) {
        spdlog::error("{}", res.error().description());
    }

    co_return anyhow::result<void>{};  //
}

auto async_main() -> boost::asio::awaitable<void> {
    auto executor = co_await boost::asio::this_coro::executor;

    if (auto res = co_await launch_bot(); !res) {
        spdlog::error("db error: {}", static_cast<john::error const&>(res.error()));
    }

    co_return;
}

auto main() -> int {
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Hello, world!");

    auto context = boost::asio::io_context{1};

    boost::asio::co_spawn(context, async_main(), boost::asio::detached);

    context.run();
}
