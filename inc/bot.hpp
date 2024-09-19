#pragma once

#include <assio/as_expected.hpp>
#include <error.hpp>
#include <kv.hpp>
#include <sqlite/database.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>

namespace john {

struct bot;
struct thing;

namespace payloads {

struct incoming_message {
    std::string_view m_thing_id;
    mini_kv m_sender_identifier;
    mini_kv m_return_to_sender;
    std::string m_content;
};

struct command {
    std::string_view m_thing_id;
    mini_kv m_sender_identifier;
    mini_kv m_return_to_sender;

    std::vector<std::string> m_argv;
};

struct outgoing_message {
    mini_kv m_target;
    std::string m_content;
};

struct add_thing {
    std::unique_ptr<thing> m_thing;
};

struct remove_thing {
    std::string m_thing_identifier;
};

struct exit {};

struct other {
    std::span<const u8> m_data;
};

}  // namespace payloads

using message_payload = std::variant<
  std::monostate,
  payloads::incoming_message,
  payloads::command,
  payloads::outgoing_message,
  payloads::add_thing,
  payloads::remove_thing,
  payloads::exit,
  payloads::other
  /**/>;

struct message {
    std::string m_from;  // TODO: refactor to string_view
    std::string m_to;

    usize m_serial;
    std::optional<usize> m_reply_serial;

    message_payload m_payload;
};

struct thing {
    virtual ~thing() = default;

    virtual auto get_id() const -> std::string_view = 0;

    // pins bot
    virtual auto worker(bot& bot) -> boost::asio::awaitable<anyhow::result<void>> = 0;

    virtual auto handle(message const&) -> boost::asio::awaitable<anyhow::result<void>> = 0;
};

// john bot
struct bot {
    bot(sqlite::database database, boost::asio::any_io_executor& executor)
        : m_database(std::move(database))
        , m_executor(executor)
        , m_message_channel(executor, 32uz)  // TODO: this is terrible
        , m_completion_channel(executor) {}

    auto run() -> boost::asio::awaitable<anyhow::result<void>>;

    // - meant to be called by "thing"s.
    // - the message serial will be overwritten.
    auto queue_message(message msg) -> boost::asio::awaitable<usize>;

    auto queue_a_reply(message const& reply_to, std::string_view from_id, message_payload payload) -> boost::asio::awaitable<void>;

    // TODO: restrict update and insert on const when the db is being used through <sqlite/*.hpp>
    auto get_db() const -> sqlite3& { return *m_database; }

    auto display_name(john::mini_kv const& kv) const -> std::optional<std::string>;

    void set_display_name(john::mini_kv const& kv, std::string name);

    auto get_executor() -> boost::asio::any_io_executor& { return m_executor; }

private:
    sqlite::database m_database;
    boost::asio::any_io_executor& m_executor;

    std::unordered_map<std::string, std::unique_ptr<thing>> m_things;

    mutable std::mutex m_display_names_mutex{};
    std::unordered_map<john::mini_kv, std::string> m_display_names{};

    std::atomic<usize> m_previous_serial{1uz};

    assify<boost::asio::experimental::concurrent_channel<void(boost::system::error_code, message)>> m_message_channel;

    assify<boost::asio::experimental::concurrent_channel<void(boost::system::error_code, std::string_view)>> m_completion_channel;

    auto handle_message(message msg) -> boost::asio::awaitable<void>;

    template<typename Payload>
    auto handle_message_internal(message&& msg, Payload&& payload) -> boost::asio::awaitable<anyhow::result<void>>;

    auto handle_message_internal(message msg) -> boost::asio::awaitable<anyhow::result<void>>;
};

}  // namespace john
