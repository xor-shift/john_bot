#include <things/relay.hpp>

#include <sqlite/query.hpp>

namespace asio = boost::asio;
using anyhow::result;
using asio::awaitable;

namespace john::things {

auto relay::worker(john::bot& bot) -> awaitable<result<void>> {
    m_bot = &bot;
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
              /*co_await m_bot->new_message(john::outgoing_message{
                .m_target = john::mini_kv::deserialize(to),
                .m_content = fmt::format("{}: {}", pretty_nick, message.m_content),
              });*/

              co_await m_bot->queue_message(john::message{
                .m_from = "relay",
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
      [](auto const&) -> awaitable<result<void>> { co_return result<void>{}; }
    };

    TRYC(co_await std::visit(visitor, msg.m_payload));

    co_return result<void>{};
}

}  // namespace john::things
