#pragma once

#include <alloc.hpp>
#include <assio/as_expected.hpp>
#include <bot.hpp>
#include <irc/message.hpp>

#include <boost/asio/ip/tcp.hpp>

namespace john::irc {

struct configuration {
    std::string m_identifier;

    std::string m_server;
    uint16_t m_port;
    bool m_use_ssl;

    std::optional<std::string> m_password;
    std::vector<std::string> m_nicks;
    std::string m_user;
    std::string m_realname;

    std::vector<std::string> m_channels;
};

namespace state {

struct disconnected {};
struct disconnecting {};

struct connected {
    usize m_nick_try;
};

struct registered {
    std::string_view m_nick;
};

struct failure_connection {};
struct failure_registration {};

}  // namespace state

using state_t = std::variant<state::disconnected, state::disconnecting, state::connected, state::registered, state::failure_connection, state::failure_registration>;

struct irc_client final : thing {
    irc_client(boost::asio::any_io_executor& executor, configuration config, usize max_message_length = 512)
        : m_config(std::move(config))
        , m_executor(executor)
        , m_socket(m_executor)
        , m_incoming_buffer(std::allocator<char>{}.allocate(max_message_length), max_message_length) {}

    ~irc_client() {
        std::allocator<char>{}.deallocate(m_incoming_buffer.data(), m_incoming_buffer.size());

        m_incoming_buffer = {};
    }

    irc_client(irc_client const&) = delete;
    irc_client(irc_client&&) = delete;

    auto get_id() const -> std::string_view override { return m_config.m_identifier; }

    auto worker(bot& bot) -> boost::asio::awaitable<anyhow::result<void>> override;

    auto handle(john::message const& message) -> boost::asio::awaitable<anyhow::result<void>> override;

private:
    configuration m_config;
    boost::asio::any_io_executor& m_executor;
    bot* m_bot = nullptr;

    //assio::mutex m_state_mutex{};
    state_t m_state = state::disconnected{};

    assify<boost::asio::ip::tcp::socket> m_socket;

    bool m_error_cleanup = false;
    std::span<char> m_incoming_buffer;
    usize m_incoming_buffer_usage = 0uz;

    auto run_inner() -> boost::asio::awaitable<std::expected<void, boost::system::error_code>>;

    auto message_handler(message_view const& msg) -> boost::asio::awaitable<void>;

    auto state_change(state_t new_state) -> boost::asio::awaitable<void>;

    auto send_message(message const& msg) -> boost::asio::awaitable<void>;

    auto identify_sender(message const& msg) const -> std::string;

    template<typename Payload>
    auto bot_message_handler(john::message const& msg, Payload const& payload) -> boost::asio::awaitable<anyhow::result<void>>;
};

}  // namespace john::irc
