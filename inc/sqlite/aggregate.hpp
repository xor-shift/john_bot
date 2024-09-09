#pragma once

#include <sqlite/error.hpp>
#include <sqlite/detail/query.hpp>

#include <stuff/intro/aggregate.hpp>

#include <sqlite3.h>

namespace sqlite {

namespace detail {

template<typename T>
struct reader;

template<typename T>
    requires std::is_aggregate_v<T>
struct reader<T> {
    static auto read(sqlite3_stmt* statement) -> std::expected<T, error> {
        using TupleType = decltype(stf::intro::tie_aggregate(std::declval<T>()));
        auto sequence = std::make_index_sequence<stf::intro::arity<T>>();
        return read_impl(statement, std::type_identity<TupleType>{}, sequence);
    }

private:
    template<typename... Ts, usize... Indices>
    static auto read_impl(sqlite3_stmt* statement, std::type_identity<std::tuple<Ts...>>, std::index_sequence<Indices...>) -> std::expected<T, error> {
        return T{
          std::invoke(column_reader_t<std::tuple_element_t<Indices, std::tuple<Ts...>>>::reader, statement, Indices)...,
        };
    }
};

}  // namespace detail

template<typename T>
inline auto query_aggregate(sqlite3& db, const char* sql, auto&&... placeholder_values) -> std::expected<std::vector<T>, error> {
    auto ret = std::vector<T>{};

    TRY(detail::query_impl(
      db, sql,
      [&ret](sqlite3_stmt* statement) -> std::expected<void, error> {
          ret.emplace_back(TRY(detail::reader<T>::read(statement)));
          return {};  //
      },
      std::forward<decltype(placeholder_values)>(placeholder_values)...
    ));

    return ret;
}

}  // namespace sqlite
