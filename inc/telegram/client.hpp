#pragma once

#include <bot.hpp>
#include <telegram/api.hpp>
#include <telegram/connection.hpp>

namespace john::telegram {

inline static configuration g_configuration = {
    .m_token = "OMITTED",
    .m_host = "api.telegram.org",
};

struct client final : thing {
    client(boost::asio::any_io_executor& executor)
        : m_executor(executor)
        , m_connection(executor, g_configuration) {}

    client(client const&) = delete;
    client(client&&) = delete;

    auto worker(bot& bot) -> boost::asio::awaitable<void> override;

    auto handle(internal_message const& message) -> boost::asio::awaitable<void> override;

private:
    boost::asio::any_io_executor& m_executor;
    connection m_connection;

    auto worker_inner(bot& bot) -> boost::asio::awaitable<anyhow::result<void>>;
};

}  // namespace john::telegram
