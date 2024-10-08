#pragma once

#include <stuff/core/integers.hpp>
#include <stuff/hash/combine.hpp>

#include <algorithm>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

namespace john {

namespace detail {

template<typename StoredType, typename Sized>
    requires requires(Sized const& v) { v.size(); }
static constexpr auto get_size_arr(Sized const& v) -> std::array<char, sizeof(StoredType)> {
    using array_type = std::array<char, sizeof(StoredType)>;

    const auto size = static_cast<StoredType>(v.size());
    return std::bit_cast<array_type>(size);
}

template<typename StoredType>
static constexpr auto from_size_ptr(const char* ptr) -> usize {
    auto arr = std::array<char, sizeof(StoredType)>{};
    std::copy_n(ptr, arr.size(), arr.begin());
    const auto stored_size = std::bit_cast<StoredType>(arr);

    return static_cast<usize>(stored_size);
}

struct mini_kv_iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<std::string_view, std::string_view>;

    constexpr auto operator++() -> mini_kv_iterator& {
        const auto [key, value] = **this;
        const auto segment_size = 2 * sizeof(u16) + key.size() + value.size();
        m_data = m_data.subspan(segment_size);

        return *this;
    }

    constexpr auto operator++(int) -> mini_kv_iterator {
        auto ret = *this;

        ++(*this);

        return ret;
    }

    constexpr auto operator*() const -> std::pair<std::string_view, std::string_view> {
        const auto key_size = from_size_ptr<u16>(m_data.data());
        const auto key = std::string_view{m_data.data() + sizeof(u16), key_size};

        const auto value_size = from_size_ptr<u16>(m_data.data() + sizeof(u16) + key_size);
        const auto value = std::string_view{m_data.data() + 2 * sizeof(u16) + key_size, value_size};

        return {key, value};
    }

    constexpr auto operator==(mini_kv_iterator const& other) const noexcept { return m_data.data() == other.m_data.data(); }

    // private:
    std::span<const char> m_data;
};

}  // namespace detail

// you aren't supposed to store a lot of elements in this
//
// you may store duplicate keys if you are ok with foregoing the key indexing
// and index only with integers. (iteration, too, works just fine with
// duplicate keys) (string accesses will return the first result)
template<typename Allocator = std::allocator<char>>
struct basic_mini_kv {
    using key_type = std::string_view;
    using mapped_type = std::string_view;
    using value_type = std::pair<key_type, mapped_type>;

    constexpr basic_mini_kv(std::initializer_list<std::pair<std::string_view, std::string_view>> il, Allocator allocator = {})
        : basic_mini_kv(allocator) {
        for (auto const& [k, v] : il) {
            push_back(k, v);
        }
    }

    template<typename Range>
    constexpr basic_mini_kv(std::from_range_t, Range&& range, Allocator allocator = {})
        : basic_mini_kv(allocator) {
        for (auto const& [k, v] : range) {
            push_back(k, v);
        }
    }

    constexpr basic_mini_kv(Allocator allocator = {})
        : m_data(allocator) {}

    // complexity: constant
    constexpr void push_back(std::string_view key, std::string_view value) {
        const auto size_needed = key.size() + value.size() + sizeof(u16) * 2;
        m_data.reserve(m_data.size() + size_needed);

        std::ranges::copy(detail::get_size_arr<u16>(key), back_inserter(m_data));
        std::ranges::copy(key, back_inserter(m_data));

        std::ranges::copy(detail::get_size_arr<u16>(value), back_inserter(m_data));
        std::ranges::copy(value, back_inserter(m_data));

        ++m_size_cache;
    }

    constexpr auto size() const -> usize { return m_size_cache; }
    constexpr auto empty() const -> bool { return m_data.empty(); }

    constexpr auto begin() const -> detail::mini_kv_iterator { return {m_data}; }
    constexpr auto end() const -> detail::mini_kv_iterator { return {std::span{m_data}.subspan(m_data.size())}; }

    // complexity: linear
    constexpr auto operator[](std::string_view key) const -> std::optional<std::string_view> {
        for (auto const& [k, v] : *this) {
            if (k == key) {
                return v;
            }
        }

        return std::nullopt;
    }

    /*constexpr auto operator[](const char* str) const -> std::pair<std::string_view, std::string_view> {
        return operator[](std::string_view{str});
    }*/

    // complexity: linear
    constexpr auto operator[](usize n) const -> std::pair<std::string_view, std::string_view> {
        auto it = begin();
        for (auto i = 0uz; i < n; i++) {
            ++it;
        }

        return *it;
    }

    // complexity: linear
    constexpr auto contains(std::string_view key) const -> bool { return (*this)[key]; }

    // complexity: quadratic
    constexpr auto operator==(basic_mini_kv const& other) const noexcept -> bool {
        for (auto const& [k, v] : *this) {
            if (auto v_other = other[k]; !v_other || *v_other != v) {
                return false;
            }
        }

        return true;
    }

    constexpr auto operator!=(basic_mini_kv const& other) const noexcept -> bool {
        return !operator==(other);
    }

    static constexpr auto deserialize(std::string_view str, Allocator allocator = {}) -> basic_mini_kv {
        using namespace std::string_view_literals;

        // clang-format off
        auto view =
            std::views::zip(
                std::views::iota(0uz),
                str
                | std::views::chunk_by([count = 0uz](const auto lhs, const auto rhs) mutable {
                    count = (count + (lhs == '\\')) * (lhs == '\\');
                    return !(lhs != '\\' && rhs == ';') || ((count % 2) == 1);
                })
            )
            | std::views::transform([](auto const& pair) {
                auto const& [idx, str] = pair;
                return idx == 0
                      ? std::string_view(str)
                      : std::string_view(str).substr(1);
            })
            | std::views::transform([](auto const& str) {
                // reversing and whatnot screws up the mutable lambda approach
                // too bad
                /*auto stuff = str
                    | std::views::chunk_by([count = 0uz](const auto lhs, const auto rhs) mutable {
                        count = (count + (lhs == '\\')) * (lhs == '\\');
                        return !(lhs != '\\' && rhs == ':') || ((count % 2) == 1);
                    })
                    | std::views::transform([](auto const& v) { return std::string_view(v); })
                    | std::views::take(2)
                    | std::views::reverse;

                auto arr = std::array<std::string_view, 2> {};
                std::ranges::copy(stuff, arr.rbegin()); // funny approach if you ask me

                return std::pair{arr[0], arr[1]};*/

                auto split = 0uz;
                for (auto escape_count = 0uz; split < str.size(); split++) {
                    const auto c = str[split];
                    const auto is_escape = c == '\\';
                    const auto is_split = c == ':';

                    if (is_split && (escape_count % 2) == 0) {
                        break;
                    }

                    escape_count = (escape_count + is_escape) * is_escape;
                }

                if (split == str.size()) {
                    return std::pair{""sv, str};
                } else {
                    const auto lhs = str.substr(0, split);
                    const auto rhs = str.substr(split + 1);

                    return std::pair{lhs, rhs};
                }
            });
        // clang-format on

        auto ret = basic_mini_kv{allocator};

        for (auto const& [k, v] : view) {
            ret.push_back(k, v);
        }

        return ret;
    }

    constexpr auto serialize() const -> std::string {
        auto required_bytes = 0uz;
        for (auto const& [k, v] : *this) {
            const auto when = [](char c) { return c == ':' || c == ';' || c == '\\'; };
            required_bytes += k.size() + std::ranges::count_if(k, when);  // key
            required_bytes += 1uz;                                        // colon
            required_bytes += v.size() + std::ranges::count_if(v, when);  // value
            required_bytes += 1uz;                                        // semicolon
        }
        required_bytes -= 1uz;  // last semicolon

        auto ret = std::string(required_bytes, ';');
        auto ret_it = ret.begin();

        const auto escaped_copy = [](std::string_view str, auto iter) {
            for (char c : str) {
                auto escapes = std::array<char, 3>{':', ';', '\\'};
                if (std::ranges::find(escapes, c) != std::ranges::end(escapes)) {
                    *iter++ = '\\';
                }
                *iter++ = c;
            }

            return iter;
        };

        for (auto first = true; auto const& [k, v] : *this) {
            if (!first) {
                *ret_it++ = ';';
            }
            first = false;

            ret_it = escaped_copy(k, ret_it);
            *ret_it++ = ':';
            ret_it = escaped_copy(v, ret_it);
        }

        return ret;
    }

private:
    usize m_size_cache = 0;
    std::vector<char, Allocator> m_data;
};

using mini_kv = basic_mini_kv<>;

}  // namespace john

template<typename Allocator>
struct std::hash<john::basic_mini_kv<Allocator>> {
    constexpr auto operator()(john::basic_mini_kv<Allocator> const& kv) const -> usize {
        auto ret = 0uz;

        for (auto const& [k, v] : kv) {
            ret = stf::hash_combine(ret, std::hash<std::string_view>{}(k));
            ret = stf::hash_combine(ret, std::hash<std::string_view>{}(k));
        }

        return ret;
    }
};
