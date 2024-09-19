#include <telegram/client.hpp>

#include <stuff/core/try.hpp>
#include <stuff/core/visitor.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <charconv>

namespace asio = boost::asio;
using anyhow::result;
using asio::awaitable;

namespace john::telegram {

auto client::worker(bot& bot) -> awaitable<result<void>> {
    m_bot = &bot;
    auto res = co_await worker_inner();
    if (!res) {
        spdlog::error("telegram worker returned error: {}", static_cast<error const&>(res.error()));
    }

    co_return result<void>{};
}

template<typename T>
auto client::handle_update(T const& update) -> awaitable<result<void>> {
    spdlog::warn("unhandled telegram update");
    co_return result<void>{};  //
}

template<>
auto client::handle_update(api::types::message const& msg) -> awaitable<result<void>> {
    using namespace std::string_literals;
    auto from_str = msg
                      .m_from  //
                      .and_then([](auto const& user) { return user.m_username; })
                      .transform([](auto const& str) { return fmt::format("@{}", str); })
                      .value_or("unknown user"s);

    if (!msg.m_text) {
        spdlog::debug("new message with id {} from {} with no content", msg.m_id, from_str);
    } else {
        spdlog::debug("new message with id {} from {} with content: {}", msg.m_id, from_str, *msg.m_text);
    }

    auto payload = payloads::incoming_message{
      .m_thing_id = m_config.m_identifier,
      .m_sender_identifier = {{"ident", m_config.m_identifier}, {"id", std::to_string(msg.m_from->m_id)}},
      .m_return_to_sender = {{"ident", m_config.m_identifier}, {"id", std::to_string(msg.m_chat->m_id)}},
      .m_content = msg.m_text.value_or(""),
    };

    if (!m_bot->display_name(payload.m_sender_identifier)) {
        m_bot->set_display_name(payload.m_sender_identifier, std::move(from_str));
    }

    co_await m_bot->queue_message(message{
      .m_from = std::string{get_id()},
      .m_to = "",

      .m_serial = 0,
      .m_reply_serial = std::nullopt,

      .m_payload = std::move(payload),
    });

    co_return result<void>{};
}

auto client::worker_inner() -> awaitable<result<void>> {
    using namespace std::string_literals;

    if (auto res = co_await m_connection.init_or_reinit(); !res) {
        spdlog::error("failed to initialize a connection to telegram: {}", static_cast<error const&>(res.error()));
    }

    if (auto res = co_await m_update_connection.init_or_reinit(); !res) {
        spdlog::error("failed to initialize the update connection to telegram: {}", static_cast<error const&>(res.error()));
    }

    auto highest_id = (u64)0;

    for (;;) {
        const auto poll_result =
          TRYC(co_await api::get_updates(m_update_connection, api::types::update_type::message | api::types::update_type::edited_message, 60, highest_id + 1));

        if (poll_result.empty()) {
            spdlog::debug("received an empty update, re-polling");
            continue;
        }

        highest_id = poll_result.back().m_update_id;

        spdlog::debug("received {} update(s) with the highest id of {}", poll_result.size(), highest_id);

        for (auto const& update : poll_result) {
            auto res = co_await std::visit([this](auto const& update) { return this->handle_update(update); }, update.m_message);

            if (!res) {
                spdlog::warn("error while handling a telegram update: {}", static_cast<john::error const&>(res.error()));
            }
        }
    }

    co_return result<void>{};
}

template<typename Payload>
auto client::bot_message_handler(john::message const& msg, Payload const& payload) -> awaitable<result<void>> {
    co_return result<void>{};
}

template<>
auto client::bot_message_handler(john::message const& msg, payloads::outgoing_message const& payload) -> awaitable<result<void>> {
    if (payload.m_target["ident"] != m_config.m_identifier) {
        co_return result<void>{};
    }

    const auto target = payload.m_target["id"].and_then([](std::string_view chars) -> std::optional<i64> {
        auto channel_id = (i64)0;
        auto res = std::from_chars(chars.begin(), chars.end(), channel_id, 10);

        if (res.ec != std::error_code{}) {
            return std::nullopt;
        }

        return channel_id;
    });

    if (!target) {
        co_return result<void>{};
    }

    auto res = co_await api::send_message(m_connection, *target, payload.m_content);
    if (!res) {
        spdlog::warn("{}", res.error().description());
    }

    co_return result<void>{};
}

auto client::handle(message const& msg) -> awaitable<result<void>> {
    return std::visit(
      [this, &msg](auto const& payload) {
          return bot_message_handler(msg, payload);  //
      },
      msg.m_payload
    );
}

}  // namespace john::telegram
