#pragma once

#include <spdlog/fmt/fmt.h>
#include <magic_enum.hpp>

#include <string_view>
#include <variant>

namespace john::irc {

#include "./replies/replies.ipp"

using reply = std::variant<int, numeric_reply, std::string_view>;

auto get_reply(std::string_view raw_reply) -> reply;

}  // namespace john::irc

template<>
struct fmt::formatter<john::irc::reply> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> fmt::format_parse_context::iterator { return ctx.begin(); }

    auto format(john::irc::reply const& reply, fmt::format_context& ctx) -> fmt::format_context::iterator;
};
