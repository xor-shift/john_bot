#pragma once

#include <bot.hpp>
#include <telegram/api.hpp>
#include <telegram/connection.hpp>

namespace john::telegram {

struct client final : thing {
    client(boost::asio::any_io_executor& executor, configuration const& config)
        : m_executor(executor)
        , m_connection(executor, config)
        , m_update_connection(executor, config)
        , m_config(config) {}

    client(client const&) = delete;
    client(client&&) = delete;

    auto worker(bot& bot) -> boost::asio::awaitable<void> override;

    auto handle(internal_message const& message) -> boost::asio::awaitable<void> override;

private:
    boost::asio::any_io_executor& m_executor;
    connection m_connection;
    connection m_update_connection;

    configuration m_config;

    auto worker_inner(bot& bot) -> boost::asio::awaitable<anyhow::result<void>>;
};

}  // namespace john::telegram
