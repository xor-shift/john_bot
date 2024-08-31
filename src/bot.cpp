#include <bot.hpp>

#include <stuff/core/visitor.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace asio = boost::asio;
using asio::awaitable;

#define ARST "\033[0m"
#define ABLD "\033[1m"

static void print_message(john::incoming_message const& message) {
    static auto kv_printer = [](std::pair<std::string_view, std::string_view> const& kv) { spdlog::debug("  " ABLD "{}" ARST ": {}", kv.first, kv.second); };

    spdlog::debug(ABLD "thing" ARST ": {}", message.m_thing_id);
    spdlog::debug("sender identifier kv list:");
    std::ranges::for_each(message.m_sender_identifier, kv_printer);
    spdlog::debug("return-to-sender kv list:");
    std::ranges::for_each(message.m_return_to_sender, kv_printer);
    spdlog::debug(ABLD "content" ARST ": {}", message.m_content);
}

namespace john {

auto bot::run() -> boost::asio::awaitable<anyhow::result<void>> {
    auto executor = co_await boost::asio::this_coro::executor;

    using operation_t = decltype(boost::asio::co_spawn(executor, std::declval<thing*>()->worker(*this)));
    auto workers = std::vector<operation_t>{};
    for (auto& thing : m_things) {
        workers.push_back(boost::asio::co_spawn(executor, thing->worker(*this)));
    }
    co_await boost::asio::experimental::make_parallel_group(std::move(workers)).async_wait(boost::asio::experimental::wait_for_all(), boost::asio::use_awaitable);

    co_return anyhow::result<void>{};
}

auto bot::new_message(internal_message message) -> boost::asio::awaitable<void> {
    spdlog::debug("new internal message");
    const auto visitor = stf::multi_visitor{
      [&](john::incoming_message const& message) -> boost::asio::awaitable<void> {
          spdlog::debug("new message:");
          print_message(message);

          co_return;
      },
      [](auto const&) -> boost::asio::awaitable<void> { co_return; }
    };

    co_await std::visit(visitor, message);

    for (auto& thing : m_things) {
        asio::co_spawn(co_await asio::this_coro::executor, thing->handle(message), asio::detached);
    }

    co_return;
}

}  // namespace john
