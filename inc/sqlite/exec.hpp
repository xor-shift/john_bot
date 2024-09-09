#pragma once

#include <sqlite/detail/query.hpp>
#include <sqlite/error.hpp>

#include <sqlite3.h>

#include <stuff/core/integers.hpp>

namespace sqlite {

inline auto exec(sqlite3& db, const char* sql, auto&&... placeholder_values) -> std::expected<void, error> {
    sqlite3_exec;

    TRY(detail::query_impl(
      db, sql,
      [](sqlite3_stmt* statement) -> std::expected<void, error> {
          return {};  //
      },
      std::forward<decltype(placeholder_values)>(placeholder_values)...
    ));

    return {};
}

}  // namespace sqlite
