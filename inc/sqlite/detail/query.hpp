#pragma once

#include <sqlite/error.hpp>

#include <sqlite3.h>

#include <stuff/core/integers.hpp>
#include <stuff/core/try.hpp>

namespace sqlite {

// column_reader
namespace detail {

template<typename T>
struct column_reader;

template<std::integral T>
    requires(!std::is_unsigned_v<T>)
struct column_reader<T> {
    inline static constexpr auto reader = sqlite3_column_int;
};

template<>
struct column_reader<bool> {
    static auto reader(sqlite3_stmt* statement, int column_no) -> bool {
        return sqlite3_column_int(statement, column_no);  //
    }
};

template<>
struct column_reader<i64> {
    inline static constexpr auto reader = sqlite3_column_int64;
};

template<>
struct column_reader<std::string> {
    static auto reader(sqlite3_stmt* statement, int column_no) -> std::string {
        // TODO: add support for column_reader_t<optional<T>>
        const auto* const str = reinterpret_cast<const char*>(sqlite3_column_text(statement, column_no));
        return str == nullptr ? "" : str;
    }
};

template<typename T>
using column_reader_t = column_reader<std::remove_cvref_t<T>>;

template<typename T>
concept column_readable = requires { column_reader<std::remove_cvref_t<T>>::reader; };

}  // namespace detail

// binder
namespace detail {

template<typename T>
struct binder;

template<std::signed_integral T>
struct binder<T> {
    inline static constexpr auto bind_fn = sqlite3_bind_int;
};

template<>
struct binder<i64> {
    inline static constexpr auto bind_fn = sqlite3_bind_int;
};

template<>
struct binder<std::string> {
    static auto bind_fn(sqlite3_stmt* statement, int column, std::string const& str) {
        sqlite3_bind_text(statement, column, str.c_str(), str.size(), SQLITE_TRANSIENT);  //
    }
};

template<typename T>
using binder_t = typename binder<std::remove_cvref_t<T>>::binder;

}  // namespace detail

// reader
namespace detail {

template<typename T>
struct reader;

}  // namespace detail

// query_impl
namespace detail {

template<typename Fun>
[[nodiscard]] inline auto query_impl(sqlite3& db, const char* sql, Fun&& callback, auto&&... placeholder_values) -> std::expected<void, error> {
    auto* statement = (sqlite3_stmt*)nullptr;
    if (auto res = sqlite3_prepare_v2(&db, sql, -1, &statement, nullptr); res != SQLITE_OK) {
        return std::unexpected{error(res, sqlite3_errmsg(&db))};
    }

    auto bind_column = 1;
    (detail::binder<std::remove_cvref_t<decltype(placeholder_values)>>::bind_fn(statement, bind_column++, placeholder_values), ...);

    auto res = SQLITE_OK;
    while ((res = sqlite3_step(statement)) == SQLITE_ROW) {
        TRY(std::invoke(std::forward<Fun>(callback), statement));
    }

    if (res != SQLITE_DONE) {
        return std::unexpected{error(res, sqlite3_errmsg(&db))};
    }

    sqlite3_finalize(statement);

    return {};
}

}  // namespace detail

}  // namespace sqlite
