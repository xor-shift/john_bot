#include <telegram/client.hpp>

#include <stuff/core/try.hpp>
#include <stuff/core/visitor.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <charconv>

namespace asio = boost::asio;

namespace john::telegram {

auto client::worker(bot& bot) -> asio::awaitable<void> {
    auto res = co_await worker_inner(bot);
    if (!res) {
        spdlog::error("telegram worker returned error: {}", static_cast<error const&>(res.error()));
    }

    co_return;
}

auto client::handle(internal_message const& message) -> boost::asio::awaitable<void> {
    const auto visitor = stf::multi_visitor{
      [&](john::outgoing_message const& message) -> boost::asio::awaitable<void> {
          if (message.m_target["ident"] != m_config.m_identifier) {
              co_return;
          }

          const auto target = message.m_target["id"].and_then([](std::string_view chars) -> std::optional<i64> {
              auto channel_id = (i64)0;
              auto res = std::from_chars(chars.begin(), chars.end(), channel_id, 10);

              if (res.ec != std::error_code{}) {
                  return std::nullopt;
              }

              return channel_id;
          });

          if (!target) {
              co_return;
          }

          auto res = co_await api::send_message(m_connection, *target, message.m_content);
          if (!res) {
              spdlog::warn("{}", res.error().description());
          }

          co_return;
      },
      [](auto const&) -> boost::asio::awaitable<void> { co_return; }
    };

    co_await std::visit(visitor, message);

    co_return;
}

auto client::worker_inner(bot& bot) -> boost::asio::awaitable<anyhow::result<void>> {
    using namespace std::string_literals;

    if (auto res = co_await m_connection.init_or_reinit(); !res) {
        spdlog::error("failed to initialize a connection to telegram: {}", static_cast<error const&>(res.error()));
    }

    if (auto res = co_await m_update_connection.init_or_reinit(); !res) {
        spdlog::error("failed to initialize the update connection to telegram: {}", static_cast<error const&>(res.error()));
    }

    auto highest_id = (u64)0;

    for (;;) {
        const auto poll_result = TRYC(co_await api::get_updates(m_update_connection, api::types::update_type::message | api::types::update_type::edited_message, 60, highest_id + 1));

        if (poll_result.empty()) {
            spdlog::debug("received an empty update, re-polling");
            continue;
        }

        highest_id = poll_result.back().m_update_id;

        spdlog::debug("received {} update(s) with the highest id of {}", poll_result.size(), highest_id);

        const auto update_visitor = stf::multi_visitor{
          [&](api::types::message const& message) -> asio::awaitable<void> {
              using namespace std::string_literals;
              const auto from_str = message
                                      .m_from  //
                                      .and_then([](auto const& user) { return user.m_username; })
                                      .transform([](auto const& str) { return fmt::format("@{}", str); })
                                      .value_or("unknown user"s);

              if (!message.m_text) {
                  spdlog::debug("new message with id {} from {} with no content", message.m_id, from_str);
              } else {
                  spdlog::debug("new message with id {} from {} with content: {}", message.m_id, from_str, *message.m_text);
              }

              co_await bot.new_message(incoming_message{
                .m_thing_id = m_config.m_identifier,
                .m_sender_identifier = {{"ident", m_config.m_identifier}, {"id", std::to_string(message.m_from->m_id)}},
                .m_return_to_sender = {{"ident", m_config.m_identifier}, {"id", std::to_string(message.m_chat->m_id)}},
                .m_content = message.m_text.value_or(""),
              });

              co_return;
          },
          [](api::types::edited_message const& message) -> asio::awaitable<void> {
              using namespace std::string_literals;
              const auto from_str = message
                                      .m_from  //
                                      .and_then([](auto const& user) { return user.m_username; })
                                      .transform([](auto const& str) { return fmt::format("@{}", str); })
                                      .value_or("unknown user"s);

              if (!message.m_text) {
                  spdlog::debug("message {} edited by {} with the new content being empty (how?)", message.m_id, from_str);
              } else {
                  spdlog::debug("message {} edited by {} with the new content being: {}", message.m_id, from_str, *message.m_text);
              }

              co_return;
          },
          [](auto const&) -> asio::awaitable<void> { co_return; },  //
        };

        for (auto const& update : poll_result) {
            co_await std::visit(update_visitor, update.m_message);
        }
    }

    co_return anyhow::result<void>{};
}

}  // namespace john::telegram
