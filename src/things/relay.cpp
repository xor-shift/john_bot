#include <things/relay.hpp>

#include <sqlite/exec.hpp>
#include <sqlite/query.hpp>

namespace asio = boost::asio;
using anyhow::result;
using asio::awaitable;

namespace john::things {

auto relay::worker(john::bot& bot) -> awaitable<result<void>> {
    m_bot = &bot;

    co_await bot.queue_message(message{
      .m_from = get_id(),
      .m_to = "bot",

      .m_serial = 0uz,
      .m_reply_serial{},

      .m_payload =
        payloads::command_decl{
          .m_command = "addmap",
          .m_description = "adds a unidirectional relay mapping",
          .m_min_level = user_level::op,
        },
    });

    co_return result<void>{};
}

auto relay::handle(john::message const& msg) -> awaitable<result<void>> {
    const auto visitor = stf::multi_visitor{
      [&](john::payloads::incoming_message const& payload) -> awaitable<result<void>> {
          using namespace std::string_literals;
          using namespace std::string_view_literals;

          auto serialized_sender = payload.m_sender_identifier.serialize();
          const auto pretty_nick = m_bot->display_name(payload.m_sender_identifier).value_or(std::move(serialized_sender));

          for (auto const& to : TRYC(sqlite::query<std::string>(m_bot->get_db(), "select to_kv from relay_mappings where from_kv = ?", payload.m_return_to_sender.serialize()))) {
              co_await m_bot->queue_message(john::message{
                .m_from = get_id(),
                .m_to = "",

                .m_serial = 0,
                .m_reply_serial = std::nullopt,

                .m_payload =
                  john::payloads::outgoing_message{
                    .m_target = john::mini_kv::deserialize(to),
                    .m_content = fmt::format("{}: {}", pretty_nick, payload.m_content),
                  },
              });
          }

          co_return result<void>{};
      },
      [&](payloads::command const& cmd) -> awaitable<result<void>> {
          if (cmd.m_argv.empty()) {
              co_return result<void>{};
          }

          if (cmd.m_argv.front() != "addmap") {
              co_return result<void>{};
          }

          if (cmd.m_argv.size() != 3) {
              co_await m_bot->queue_a_reply(
                msg, get_id(),
                payloads::outgoing_message{
                  .m_target = cmd.m_return_to_sender,
                  .m_content = "addmap requires 3 arguments",
                }
              );
              co_return result<void>{};
          }

          auto res = sqlite::exec(m_bot->get_db(), "insert into relay_mappings (from_kv, to_kv) values (?, ?)", cmd.m_argv[1], cmd.m_argv[2]);

          if (!res) {
              spdlog::error("sqlite error while executing command with serial #{}: {}", msg.m_serial, static_cast<error const&>(res.error()));
              co_await m_bot->queue_a_reply(
                msg, get_id(),
                payloads::outgoing_message{
                  .m_target = cmd.m_return_to_sender,
                  .m_content = "an error has ocurred, check logs",
                }
              );
          } else {
              co_await m_bot->queue_a_reply(
                msg, get_id(),
                payloads::outgoing_message{
                  .m_target = cmd.m_return_to_sender,
                  .m_content = "added a map",
                }
              );
          }

          co_return result<void>{};
      },
      [](auto const&) -> awaitable<result<void>> { co_return result<void>{}; }
    };

    TRYC(co_await std::visit(visitor, msg.m_payload));

    co_return result<void>{};
}

}  // namespace john::things
