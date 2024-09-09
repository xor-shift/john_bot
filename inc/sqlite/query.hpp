#pragma once

#include <sqlite/detail/query.hpp>
#include <sqlite/error.hpp>

#include <sqlite3.h>

#include <stuff/core/integers.hpp>

namespace sqlite {

// exec_return
namespace detail {

template<typename... Ts>
struct exec_return : std::type_identity<std::tuple<Ts...>> {};

template<typename T>
struct exec_return<T> : std::type_identity<T> {};

template<typename... Ts>
using exec_return_t = typename exec_return<Ts...>::type;

}  // namespace detail

// reader
namespace detail {

template<typename... Ts>
struct reader<std::tuple<Ts...>> {
    static auto read(sqlite3_stmt* statement) -> std::expected<std::tuple<Ts...>, error> {
        return read_impl(statement, std::make_index_sequence<sizeof...(Ts)>());  //
    }

private:
    template<usize... Indices>
    static auto read_impl(sqlite3_stmt* statement, std::index_sequence<Indices...>) -> std::expected<std::tuple<Ts...>, error> {
        return std::tuple<Ts...>{
          std::invoke(column_reader_t<std::tuple_element_t<Indices, std::tuple<Ts...>>>::reader, statement, Indices)...,
        };
    }
};

template<column_readable T>
struct reader<T> {
    static auto read(sqlite3_stmt* statement) -> std::expected<T, error> {
        return static_cast<T>(std::invoke(column_reader_t<T>::reader, statement, 0));  //
    }
};

}  // namespace detail

template<typename... Ts>
inline auto query(sqlite3& db, const char* sql, auto&&... placeholder_values) -> std::expected<std::vector<detail::exec_return_t<Ts...>>, error> {
    using U = detail::exec_return_t<Ts...>;
    auto ret = std::vector<U>{};

    TRY(detail::query_impl(
      db, sql,
      [&ret](sqlite3_stmt* statement) -> std::expected<void, error> {
          ret.emplace_back(TRY(detail::reader<U>::read(statement)));  //
          return {};
      },
      std::forward<decltype(placeholder_values)>(placeholder_values)...
    ));

    return ret;
}

}  // namespace sqlite
