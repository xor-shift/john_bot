#pragma once

#include <alloc.hpp>
#include <assio/as_expected.hpp>
#include <bot.hpp>
#include <irc/message.hpp>

#include <boost/asio/ip/tcp.hpp>

namespace john::irc {

struct configuration {
    std::string m_identifier;
    std::vector<std::string> m_nicks;
    std::string m_user;
    std::string m_realname;

    std::vector<std::string> m_channels;
};

namespace state {

struct disconnected {};

struct connected {
    usize m_nick_try;
};

struct registered {
    std::string_view m_nick;
};

struct failure_connection {};
struct failure_registration {};

}  // namespace state

using state_t = std::variant<state::disconnected, state::connected, state::registered, state::failure_connection, state::failure_registration>;

struct irc_client final : thing {
    irc_client(boost::asio::any_io_executor& executor, usize max_message_length = 512, std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : m_executor(executor)
        , m_resource(resource)
        , m_incoming_buffer(john::create_n<char>(resource, max_message_length), max_message_length)
        , m_socket(m_executor) {}

    ~irc_client() {
        john::destroy(m_resource, m_incoming_buffer.data(), m_incoming_buffer.size());
        m_incoming_buffer = {};
    }

    irc_client(irc_client const&) = delete;
    irc_client(irc_client&&) = delete;

    auto worker(bot& bot) -> boost::asio::awaitable<void> override;

    auto handle(internal_message const& message) -> boost::asio::awaitable<void> override;

private:
    boost::asio::any_io_executor& m_executor;
    std::pmr::memory_resource* m_resource;

    state_t m_state = state::disconnected{};

    bool m_error_cleanup = false;
    std::span<char> m_incoming_buffer;
    usize m_incoming_buffer_usage = 0uz;

    assify<boost::asio::ip::tcp::socket> m_socket;

    bot* m_bot = nullptr;

    auto run_inner() -> boost::asio::awaitable<std::expected<void, boost::system::error_code>>;

    auto message_handler(message_view const& message) -> boost::asio::awaitable<void>;

    auto state_change(state_t new_state) -> boost::asio::awaitable<void>;

    auto send_message(message const& msg) -> boost::asio::awaitable<void>;

    auto identify_sender(message const& msg) const -> std::string;
};

}  // namespace john::irc
