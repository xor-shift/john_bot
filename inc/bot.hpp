#pragma once

#include <error.hpp>
#include <kv.hpp>
#include <sqlite/database.hpp>

#include <boost/asio/awaitable.hpp>

namespace john {

struct bot;

struct incoming_message {
    std::string_view m_thing_id;
    mini_kv m_sender_identifier;
    mini_kv m_return_to_sender;
    std::string m_content;
};

struct outgoing_message {
    mini_kv m_target;
    std::string m_content;
};

using internal_message = std::variant<incoming_message, outgoing_message>;

struct thing {
    virtual ~thing() = default;

    // pins bot
    virtual auto worker(bot& bot) -> boost::asio::awaitable<void> = 0;

    virtual auto handle(internal_message const&) -> boost::asio::awaitable<void> = 0;
};

// john bot
struct bot {
    bot(sqlite::database database, boost::asio::any_io_executor& executor)
        : m_database(std::move(database))
        , m_executor(executor) {}

    auto run() -> boost::asio::awaitable<anyhow::result<void>>;

    // meant to be called by "thing"s
    auto new_message(internal_message msg) -> boost::asio::awaitable<void>;

    template<typename B, typename... Args>
        requires(std::is_base_of_v<thing, B>)
    void add_thing(Args&&... args) {
        m_things.emplace_back(std::make_unique<B>(std::forward<Args>(args)...));
    }

    // TODO: restrict update and insert on const when the db is being used through <sqlite/*.hpp>
    auto get_db() const -> sqlite3& { return *m_database; }

private:
    sqlite::database m_database;
    boost::asio::any_io_executor& m_executor;

    std::vector<std::unique_ptr<thing>> m_things;
};

}  // namespace john
