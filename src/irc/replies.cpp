#include <irc/replies.hpp>

#include <algorithm>
#include <ranges>
#include <span>

namespace john::irc {

namespace detail {

#include <irc/replies/reply_list.ipp>

}

auto get_reply(std::string_view raw_reply) -> reply {
    if (raw_reply.size() == 3 && std::ranges::all_of(raw_reply, [](const auto v) { return v >= '0' && v <= '9'; })) {
        const auto numeric_value = (raw_reply[0] - '0') * 100 + (raw_reply[1] - '0') * 10 + (raw_reply[2] - '0');

        auto keys_view = std::span(detail::valid_replies) | std::views::keys;
        const auto it = std::ranges::lower_bound(keys_view, numeric_value);
        if (it != keys_view.end() && *it == numeric_value) {
            return static_cast<numeric_reply>(numeric_value);
        }

        return numeric_value;
    }

    return raw_reply;
}

}  // namespace john::irc

auto fmt::formatter<john::irc::reply>::format(john::irc::reply const& reply, fmt::format_context& ctx) -> fmt::format_context::iterator {
    const auto visitor = [it = ctx.out()]<typename T>(T const& v) constexpr {
        if constexpr (std::is_same_v<T, int>) {
            return fmt::format_to(it, "{} (unknown)", v);
        }

        if constexpr (std::is_same_v<T, std::string_view>) {
            return fmt::format_to(it, "\"{}\"", v);
        }

        if constexpr (std::is_same_v<T, john::irc::numeric_reply>) {
            auto const& valid_replies = john::irc::detail::valid_replies;

            auto keys_view = std::span(valid_replies) | std::views::keys;
            const auto key_it = std::ranges::lower_bound(keys_view, static_cast<int>(v));

            const auto* const name = valid_replies[std::distance(keys_view.begin(), key_it)].second;

            return fmt::format_to(it, "{} ({})", static_cast<int>(v), name);
        }
    };

    return std::visit(visitor, reply);
}
