#pragma once

#include <spdlog/fmt/fmt.h>
#include <cpptrace/cpptrace.hpp>

#include <expected>
#include <optional>

namespace john {

struct error {
    error() {}

    constexpr virtual ~error() = default;

    constexpr virtual auto source() & -> error* { return nullptr; }
    constexpr virtual auto source() const& -> const error* { return nullptr; }

    virtual auto source() && -> std::shared_ptr<error> { return nullptr; }

    constexpr virtual auto description() const -> std::string_view = 0;

    constexpr auto backtrace() -> cpptrace::raw_trace& { return m_trace; }

private:
    cpptrace::raw_trace m_trace;
};

}  // namespace john

namespace boost::system {
class error_code;
}

namespace anyhow {

struct string_error final : john::error {
    string_error(std::string description, std::unique_ptr<error> source = nullptr)
        : m_description(std::move(description))
        , m_source(std::move(source)) {}

    constexpr auto source() & -> error* { return m_source.get(); }
    constexpr auto source() const& -> const error* { return m_source.get(); }

    auto source() && -> std::shared_ptr<error> { return std::move(m_source); }

    constexpr auto description() const -> std::string_view { return m_description; }

private:
    std::string m_description;
    std::shared_ptr<error> m_source;
};

struct error final : john::error {
    template<typename T>
        requires(std::is_base_of_v<john::error, std::remove_cvref_t<T>>) && (!std::is_same_v<std::remove_cvref_t<T>, error>)
    error(T&& v)
        : m_deleter(&error::delete_t<T>)
        , m_copier(&error::copy_t<T>)
        , m_original(static_cast<john::error*>(new std::remove_cvref_t<T>(std::forward<T>(v)))) {}

    error(std::string_view message)
        : error(string_error(std::string{message})) {}

    template<typename T>
        requires std::is_same_v<T, boost::system::error_code> &&  // make sure that we have to instantiate the tempalte
                 std::is_object_v<T>                              // make sure that the type is defined
    error(T ec)
        : error(string_error(ec.what())) {}

    error(error const& other)
        : m_deleter(other.m_deleter)
        , m_copier(other.m_copier)
        , m_original(m_copier(other.m_original)) {}

    error(error&& other) noexcept
        : m_deleter(other.m_deleter)
        , m_copier(other.m_copier)
        , m_original(other.m_original) {
        other.m_original = nullptr;
    }

    ~error() override {
        if (m_original) {
            m_deleter(m_original);
            m_original = nullptr;
        }
    }

    auto source() & -> john::error* override { return m_original->source(); }
    auto source() const& -> const john::error* override { return m_original->source(); }

    auto source() && -> std::shared_ptr<john::error> override { return std::move(*m_original).source(); }

    auto description() const -> std::string_view override { return m_original->description(); }

private:
    void (*m_deleter)(john::error*) = nullptr;
    john::error* (*m_copier)(john::error*) = nullptr;

    john::error* m_original = nullptr;

    template<typename T, typename U = std::remove_cvref_t<T>>
    static void delete_t(john::error* error) {
        delete reinterpret_cast<U*>(error);
    }

    template<typename T, typename U = std::remove_cvref_t<T>>
        requires std::copy_constructible<U>
    static auto copy_t(john::error* error) -> john::error* {
        const auto* const original = reinterpret_cast<const U*>(error);
        return new U(*original);
    }
};

template<typename T>
using result = std::expected<T, error>;

}  // namespace anyhow

#define _anyhow(message) \
    std::unexpected { ::anyhow::error(::anyhow::string_error(std::string(message))) }

#define _anyhow_fmt(_format, ...) \
    std::unexpected { ::anyhow::error(::anyhow::string_error(fmt::format(_format, __VA_ARGS__))) }

template<>
struct fmt::formatter<john::error> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> fmt::format_parse_context::iterator { return ctx.begin(); }

    auto format(john::error const& error, fmt::format_context& ctx) -> fmt::format_context::iterator {
        auto it = ctx.out();

        it = fmt::format_to(it, "error with description: {}", error.description());

        const auto* cause = error.source();
        while (cause != nullptr) {
            it = fmt::format_to(it, "\ncaused by error: {}", cause->description());
            cause = cause->source();
        }

        return it;
    }
};
