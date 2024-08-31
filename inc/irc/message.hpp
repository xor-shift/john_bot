#pragma once

#include <error.hpp>
#include <irc/replies.hpp>
#include <stuff/core/try.hpp>

#include <ranges>

namespace john::irc {

// received message
struct message_view {
    std::string m_original_message;

    std::optional<std::string_view> m_prefix_name;
    std::optional<std::string_view> m_prefix_user;
    std::optional<std::string_view> m_prefix_host;

    reply m_command;
    std::string_view m_params;
    std::optional<std::string_view> m_trailing;

    static auto from_chars(std::string_view message) -> anyhow::result<message_view> {
        if (!message.ends_with("\r\n")) {
            return _anyhow("no trailing CRLF?");
        }
        message.remove_suffix(2);

        auto ret = message_view{std::string{message}};

        auto after_prefix = TRY(ret.init_prefix(message));
        auto after_command = TRY(ret.init_command(after_prefix));
        auto after_params = TRY(ret.init_params(after_command));
        auto after_trailing = TRY(ret.init_trailing(after_params));
        static_cast<void>(after_trailing);

        return ret;
    }

    auto params() const -> std::ranges::range auto {
        using namespace std::string_view_literals;
        return m_params |                                                      //
               std::views::split(" "sv) |                                      //
               std::views::filter([](auto const& v) { return !v.empty(); }) |  //
               std::views::transform([](auto const& v) { return std::string_view(v); });
    }

private:
    auto init_prefix(std::string_view message) -> anyhow::result<std::string_view> {
        if (!message.starts_with(':')) {
            return message;
        }

        const auto prefix_0 = message.substr(0, std::distance(message.begin(), std::ranges::find(message, ' ')));
        const auto prefix_name = prefix_0.substr(0, std::min(prefix_0.find('!'), prefix_0.find('@')));

        const auto prefix_1 = prefix_0.substr(prefix_name.size());
        const auto prefix_user = prefix_1.substr(0, prefix_1.find('@'));

        const auto prefix_host = prefix_1.substr(prefix_user.size());

        m_prefix_name = prefix_name.substr(1);

        if (!prefix_user.empty()) {
            m_prefix_user = prefix_user.substr(1);
        }

        if (!prefix_host.empty()) {
            m_prefix_host = prefix_host.substr(1);
        }

        const auto the_rest = message.substr(prefix_0.size());
        if (the_rest.empty()) {
            return the_rest;
        }

        return the_rest.substr(the_rest.find_first_not_of(' '));
    }

    auto init_command(std::string_view after_prefix) -> anyhow::result<std::string_view> {
        if (after_prefix.empty()) {
            return _anyhow("no command in message");
        }

        const auto command_str = after_prefix.substr(0, after_prefix.find(' '));
        const auto after_command = after_prefix.substr(command_str.size());

        m_command = get_reply(command_str);
        return after_command;
    }

    auto init_params(std::string_view after_command) -> anyhow::result<std::string_view> {
        const auto params_str = after_command.substr(0, after_command.find(':'));
        const auto after_params = after_command.substr(params_str.size());

        m_params = params_str;
        return after_params;
    }

    auto init_trailing(std::string_view after_params) -> anyhow::result<std::string_view> {
        if (after_params.empty()) {
            return after_params;
        }

        if (!after_params.starts_with(':')) {
            return _anyhow("trailing doesn't start with :");
        }

        m_trailing = after_params.substr(1);

        return "";
    }
};

// message to be sent
struct message {
    std::optional<std::string> m_prefix_name;
    std::optional<std::string> m_prefix_user;
    std::optional<std::string> m_prefix_host;

    std::string_view m_command;
    std::vector<std::string> m_params;
    std::optional<std::string> m_trailing;

    // interface compat with message_view
    auto params() const -> std::ranges::range auto { return std::ranges::ref_view(m_params); }

    static auto bare(std::string_view command) -> message { return {.m_command = command}; }

    auto with_name(std::string name, std::optional<std::string> user = std::nullopt, std::optional<std::string> host = std::nullopt) && -> message {
        m_prefix_name = std::move(name);
        m_prefix_user = std::move(user);
        m_prefix_host = std::move(host);
        return std::move(*this);
    }

    auto with_param(std::string param) && -> message {
        m_params.push_back(param);
        return std::move(*this);
    }

    auto with_trailing(std::string trailing) && -> message {
        m_trailing = trailing;
        return std::move(*this);
    }

    auto encode() const -> std::vector<std::string> {
        auto before_trailing = std::string{};
        auto bt_it = std::back_inserter(before_trailing);

        if (m_prefix_name) {
            *bt_it++ = ':';
            bt_it = std::copy(m_prefix_name->begin(), m_prefix_name->end(), bt_it);

            if (m_prefix_user) {
                *bt_it++ = '!';
                bt_it = std::copy(m_prefix_user->begin(), m_prefix_user->end(), bt_it);
            }

            if (m_prefix_host) {
                *bt_it++ = '@';
                bt_it = std::copy(m_prefix_host->begin(), m_prefix_host->end(), bt_it);
            }

            *bt_it++ = ' ';
        }

        bt_it = std::copy(m_command.begin(), m_command.end(), bt_it);

        for (auto const& param : m_params) {
            *bt_it++ = ' ';
            bt_it = std::copy(param.begin(), param.end(), bt_it);
        }

        // TODO: split messages on trailing
        if (m_trailing) {
            *bt_it++ = ' ';
            bt_it = std::copy(m_trailing->begin(), m_trailing->end(), bt_it);
        }

        *bt_it++ = '\r';
        *bt_it++ = '\n';

        return {before_trailing};
    }
};

}  // namespace john::irc
