#pragma once

#include <stuff/core/integers.hpp>

#include <error.hpp>
#include <telegram/connection.hpp>

#include <memory>

namespace john::telegram::api {

namespace types {

struct user {
    i64 m_id;
    bool m_is_bot;

    std::string m_first_name;
    std::string m_last_name;
    std::string m_username;

    std::string m_language_code;

    bool m_is_premium;
    bool m_added_to_attachment_menu;

    // only in getMe:

    bool m_can_join_groups;
    bool m_can_read_all_group_messages;
    bool m_supports_inline_queries;
    bool m_can_connect_to_business;
    bool m_has_main_web_app;
};

}

auto get_me(connection& conn) -> boost::asio::awaitable<anyhow::result<types::user>>;

}  // namespace john::telegram
