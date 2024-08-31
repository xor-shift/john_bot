#include <telegram/client.hpp>

#include <stuff/core/try.hpp>

#include <spdlog/spdlog.h>

namespace asio = boost::asio;

namespace john::telegram {

auto client::worker(bot& bot) -> asio::awaitable<void> {
    auto res = co_await worker_inner(bot);
    if (!res) {
        spdlog::error("telegram worker returned error: {}", res.error().description());
    }

    co_return;
}

auto client::handle(internal_message const& message) -> boost::asio::awaitable<void> {
    co_return;  //
}

auto client::worker_inner(bot& bot) -> boost::asio::awaitable<anyhow::result<void>> {
    using namespace std::string_literals;

    if (auto res = co_await m_connection.init_or_reinit(); !res) {
        spdlog::error("failed to initialize a connection to telegram: {}", res.error().description());
    }

    auto body = boost::json::object{
      {"allowed_updates", {"message"}},
      {"timeout", "300"},
    };
    auto res = co_await m_connection.make_request("getUpdates", body);

    if (res) {
        auto ser = serialize(*res);
        spdlog::debug("tg response: {}", ser);
    } else {
        spdlog::error("failed to make request: {}", res.error().description());
    }

    co_return anyhow::result<void>{};
}

}  // namespace john::telegram
