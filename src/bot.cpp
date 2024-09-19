#include <bot.hpp>

#include <stuff/core/visitor.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>

namespace asio = boost::asio;
using anyhow::result;
using asio::awaitable;

#define ARST "\033[0m"
#define ABLD "\033[1m"

namespace john {

auto bot::run() -> awaitable<result<void>> {
    for (;;) {
        spdlog::trace("awaiting message retreival");
        auto res = co_await m_message_channel.async_receive();

        if (!res && !m_message_channel.is_open()) {
            spdlog::info("message channel is closed, exiting");
            break;
        }

        if (!res) {
            spdlog::error("failed to receive from the message channel, error: {}", res.error().what());
            spdlog::error("pray that all workers have quit or we're about to hang");
            break;
        }

        spdlog::trace("new message");
        co_await handle_message(std::move(*res));
    }

    spdlog::debug("going to join the workers");

    for (auto i = 0uz; i < m_things.size(); i++) {
        auto res = co_await m_completion_channel.async_receive();

        if (!res) {
            spdlog::error("failed to read from the completion channel with {} remaining workers: {}", m_things.size() - i, res.error().what());
            break;
        }

        spdlog::debug("the `thing` with the id \"{}\" exited successfuly", *res);
    }

    spdlog::info("successfully exited");

    co_return result<void>{};
}

auto bot::display_name(john::mini_kv const& kv) const -> std::optional<std::string> {
    auto _ = std::unique_lock{m_display_names_mutex};

    if (auto it = m_display_names.find(kv); it == m_display_names.end()) {
        return std::nullopt;
    } else {
        return it->second;
    }
}

void bot::set_display_name(john::mini_kv const& kv, std::string name) {
    auto _ = std::unique_lock{m_display_names_mutex};

    m_display_names.insert_or_assign(kv, name);
}

auto bot::queue_message(message message) -> awaitable<usize> {
    const auto message_serial = m_previous_serial++;
    message.m_serial = message_serial;

    spdlog::debug("a message from \"{}\" addressed to \"{}\" is being queued and got assigned the serial {}", message.m_from, message.m_to, message_serial);

    const auto sent_immediately = m_message_channel.try_send(boost::system::error_code{}, std::move(message));

    if (sent_immediately) {
        spdlog::trace("queued message with serial {} immediately", message_serial);
        co_return message_serial;
    }

    auto res = co_await m_message_channel.async_send(boost::system::error_code{}, std::move(message));

    if (!res) {
        spdlog::error("failed to queue a message: {}", res.error().what());
    } else {
        spdlog::trace("queued message with serial {} asynchronously", message_serial);
    }

    co_return message_serial;
}

auto bot::queue_a_reply(message const& reply_to, std::string_view from_id, message_payload payload) -> awaitable<void> {
    co_await queue_message(message{
      .m_from = std::string{from_id},
      .m_to = reply_to.m_from,

      .m_serial = 0uz,
      .m_reply_serial = reply_to.m_serial,

      .m_payload = std::move(payload),
    });
}

auto bot::handle_message(message msg) -> awaitable<void> {
    spdlog::debug("new message with serial {} from \"{}\" addressed to \"{}\"", msg.m_serial, msg.m_from, msg.m_to);

    auto wrapper = [](thing& thing, message const& msg) -> awaitable<void> {
        auto res = co_await thing.handle(msg);
        if (!res) {
            spdlog::error(
              "the `thing` \"{}\" returned an error while handling a message with serial {} (from \"{}\"): {}",  //
              thing.get_id(), msg.m_serial, msg.m_from, static_cast<error const&>(res.error())
            );
        }
        co_return;
    };

    auto internal_wrapper = [this](message msg) -> awaitable<void> {
        auto res = co_await handle_message_internal(std::move(msg));
        if (!res) {
            spdlog::error("error while handling message internally: {}", static_cast<error const&>(res.error()));
        }

        co_return;
    };

    if (msg.m_to == "") {
        using operation_t = decltype(asio::co_spawn(m_executor, std::declval<awaitable<void>>()));
        auto workers = std::vector<operation_t>{};
        for (auto& [id, thing] : m_things) {
            workers.push_back(asio::co_spawn(m_executor, wrapper(*thing, msg)));
        }

        // parallel groups go wack if theres nothing to wait on
        // truly experimental
        workers.push_back(asio::co_spawn(m_executor, ([]() -> awaitable<void> { co_return; })()));

        co_await asio::experimental::make_parallel_group(std::move(workers)).async_wait(asio::experimental::wait_for_all(), asio::use_awaitable);

        // handle the message internally last so that workers have a chance to quit
        co_await internal_wrapper(std::move(msg));
    } else if (msg.m_to == "bot") {
        co_await internal_wrapper(std::move(msg));
    } else {
        auto it = m_things.find(msg.m_to);

        if (it == m_things.end()) {
            spdlog::warn(
              "a message from \"{}\" with serial {} is addressed to \"{}\" but no such `thing` has been registered",  //
              msg.m_from, msg.m_serial, msg.m_to
            );
        } else {
            co_await wrapper(*(it->second), msg);
        }
    }

    co_return;
}

template<typename Payload>
auto bot::handle_message_internal(message&& msg, [[maybe_unused]] Payload&& payload) -> awaitable<result<void>> {
    if (msg.m_to == "bot") {
        spdlog::warn("message (from \"{}\", serial: {}) addressed to be internally handled has no handler", msg.m_from, msg.m_serial);
    }
    co_return result<void>{};
}

template<>
auto bot::handle_message_internal(message&& msg, [[maybe_unused]] payloads::exit&& payload) -> awaitable<result<void>> {
    m_message_channel.close();
    co_return result<void>{};
}

template<>
auto bot::handle_message_internal(message&& msg, payloads::add_thing&& payload) -> awaitable<result<void>> {
    auto key = std::string{payload.m_thing->get_id()};
    spdlog::info("adding a new `thing` with the id \"{}\"", key);

    auto [it, emplaced] = m_things.try_emplace(key, std::move(payload.m_thing));
    if (!emplaced) {
        spdlog::warn("tried to add a `thing` with an id \"{}\" but it appears to already be registered", key);
        co_return result<void>{};
    }

    asio::co_spawn(
      m_executor,
      [this, &thing = *(it->second)] -> awaitable<void> {
          spdlog::debug("the worker for the `thing` with the id \"{}\" has started", thing.get_id());

          auto res = co_await thing.worker(*this);
          if (!res) {
              spdlog::debug("the worker for the `thing` with the id \"{}\" has exited with error: {}", thing.get_id(), static_cast<error const&>(res.error()));
          } else {
              spdlog::debug("the worker for the `thing` with the id \"{}\" has exited cleanly", thing.get_id());
          }

          co_await m_completion_channel.async_send({}, thing.get_id());

          co_return;
      },
      asio::detached
    );

    co_return result<void>{};
}

auto bot::handle_message_internal(message msg) -> awaitable<result<void>> {
    auto visitor = [this, &msg]<typename Payload>(Payload&& payload) -> awaitable<result<void>> {
        return handle_message_internal<Payload>(std::move(msg), std::forward<Payload>(payload));
    };

    return std::visit(visitor, std::move(msg.m_payload));
}

}  // namespace john
