#pragma once

#include <stuff/core/integers.hpp>

#include <error.hpp>
#include <telegram/connection.hpp>

#include <memory>

namespace john::telegram::api {

namespace types {

struct user {
    inline static constexpr usize _stf_arity = 13uz;

    i64 m_id;
    bool m_is_bot;

    std::optional<std::string> m_first_name;
    std::optional<std::string> m_last_name;
    std::optional<std::string> m_username;

    std::optional<std::string> m_language_code;

    std::optional<bool> m_is_premium;
    std::optional<bool> m_added_to_attachment_menu;

    // only in getMe:

    std::optional<bool> m_can_join_groups;
    std::optional<bool> m_can_read_all_group_messages;
    std::optional<bool> m_supports_inline_queries;
    std::optional<bool> m_can_connect_to_business;
    std::optional<bool> m_has_main_web_app;
};

struct chat {
    inline static constexpr usize _stf_arity = 7uz;

    i64 m_id;
    std::string m_type;

    std::optional<std::string> m_title;
    std::optional<std::string> m_username;
    std::optional<std::string> m_first_name;
    std::optional<std::string> m_last_name;

    std::optional<bool> m_is_forum;
};

struct message {
    inline static constexpr usize _stf_arity = 5uz;

    i64 m_id;
    std::optional<i64> m_thread_id;

    std::optional<user> m_from;
    std::optional<chat> m_chat;

    std::optional<std::string> m_text;
    // std::optional<std::vector<message_entity>> m_entities;
};

struct edited_message : message {
    inline static constexpr usize _stf_arity = message::_stf_arity;
};

struct update {
    inline static constexpr usize _stf_arity = 2uz;

    i64 m_update_id;
    std::variant<message, edited_message> m_message;
};

// clang-format off
enum class update_type : u32 {
    none                      = 0,
    message                   = 1 << 0,
    edited_message            = 1 << 1,
    channel_post              = 1 << 2,
    edited_channel_post       = 1 << 3,
    business_connection       = 1 << 4,
    business_message          = 1 << 5,
    edited_business_message   = 1 << 6,
    deleted_business_messages = 1 << 7,
    message_reaction          = 1 << 8,
    message_reaction_count    = 1 << 9,
    inline_query              = 1 << 10,
    chosen_inline_result      = 1 << 11,
    callback_query            = 1 << 12,
    shipping_query            = 1 << 13,
    pre_checkout_query        = 1 << 14,
    poll                      = 1 << 15,
    poll_answer               = 1 << 16,
    my_chat_member            = 1 << 17,
    chat_member               = 1 << 18,
    chat_join_request         = 1 << 19,
    chat_boost                = 1 << 20,
    removed_chat_boost        = 1 << 21,

    all = 0x1FFFFFull,
};
// clang-format on

constexpr auto operator|(update_type lhs, update_type rhs) -> update_type { return static_cast<update_type>(static_cast<u32>(lhs) | static_cast<u32>(rhs)); }

constexpr auto operator&(update_type lhs, update_type rhs) -> update_type { return static_cast<update_type>(static_cast<u32>(lhs) & static_cast<u32>(rhs)); }

}  // namespace types

auto get_me(connection& conn) -> boost::asio::awaitable<anyhow::result<types::user>>;

/// @param type
///   The type of updates to be returned. Can be a combination of update_type enumerations.
///
/// @param timeout_seconds
///   The timeout for the request, enforced by telegram. Use 0 for short polling.
///
/// @param offset
///   The first update returned will have an id greater than or equal to this value.
auto get_updates(connection& conn, types::update_type type = types::update_type::all, u64 timeout_seconds = 300, u64 offset = 0) -> boost::asio::awaitable<anyhow::result<std::vector<types::update>>>;

auto send_message(connection& conn, std::variant<i64, std::string_view> chat_id, std::string_view text) -> boost::asio::awaitable<anyhow::result<types::message>>;

}  // namespace john::telegram::api
