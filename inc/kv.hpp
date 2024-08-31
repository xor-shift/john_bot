#pragma once

#include <memory>
#include <ranges>
#include <string_view>

#include <common.hpp>

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

    // complexity: linear
    constexpr auto operator[](usize n) const -> std::pair<std::string_view, std::string_view> {
        auto it = begin();
        for (auto i = 0uz; i < n; i++) {
            ++it;
        }

        return *it;
    }

    // complexity: linear
    constexpr auto contains(std::string_view key) -> bool { return (*this)[key]; }

private:
    usize m_size_cache = 0;
    std::vector<char, Allocator> m_data;
};

using mini_kv = basic_mini_kv<>;

}  // namespace john
