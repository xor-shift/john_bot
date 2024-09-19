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

    auto get_id() const -> std::string_view override { return m_config.m_identifier; }

    auto worker(bot& bot) -> boost::asio::awaitable<anyhow::result<void>> override;

    auto handle(message const& msg) -> boost::asio::awaitable<anyhow::result<void>> override;

private:
    boost::asio::any_io_executor& m_executor;
    connection m_connection;
    connection m_update_connection;

    configuration m_config;

    bot* m_bot = nullptr;

    auto worker_inner() -> boost::asio::awaitable<anyhow::result<void>>;

    template<typename T>
    auto handle_update(T const& update) -> boost::asio::awaitable<anyhow::result<void>>;

    template<typename Payload>
    auto bot_message_handler(john::message const& msg, Payload const& payload) -> boost::asio::awaitable<anyhow::result<void>>;
};

}  // namespace john::telegram
