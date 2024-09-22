#include <telegram/api.hpp>

#include <stuff/core/try.hpp>
#include <stuff/intro.hpp>
#include <stuff/intro/aggregate.hpp>

#include <spdlog/spdlog.h>
#include <boost/json.hpp>

#include <ranges>
#include <span>

namespace asio = boost::asio;
namespace json = boost::json;

using asio::awaitable;

namespace john::telegram::api {

namespace detail {

template<typename T>
struct field_names;

template<>
struct field_names<types::user> {
    inline static constexpr const char* m_names[]{
      "id",
      "is_bot",
      "first_name",
      "last_name",
      "username",
      "language_code",
      "is_premium",
      "added_to_attachment_menu",
      "can_join_groups",
      "can_read_all_group_messages",
      "supports_inline_queries",
      "can_connect_to_business",
      "has_main_web_app"
    };
};

template<>
struct field_names<types::chat> {
    inline static constexpr const char* m_names[]{
      "id", "type", "title", "username", "first_name", "last_name", "is_forum",
    };
};

template<>
struct field_names<types::message_entity> {
    inline static constexpr const char* m_names[]{
      "type", "offset", "length", "url", "user", "language", "custom_emoji_id",
    };
};

template<>
struct field_names<types::message> {
    inline static constexpr const char* m_names[]{
      "message_id", "message_thread_id", "from", "chat", "text", "entities",
    };
};

template<>
struct field_names<types::edited_message> : field_names<types::message> {};

template<>
struct field_names<types::update> {
    inline static constexpr const char* m_names[]{
      "update_id",
      "message;edited_message",
    };
};

template<typename T, template<typename...> typename Type>
struct is_specialisation_of : std::false_type {};

template<template<typename...> typename T, typename... Args>
struct is_specialisation_of<T<Args...>, T> : std::true_type {};

// TODO:
// change the try_as_type functions to be regular functions instead of member function pointers
// add support for true_type (requires the change above so that additional checks can be done)
template<typename T>
struct read_info;

template<>
struct read_info<bool> {
    static constexpr const char* m_type_name = "bool";
    static constexpr boost::system::result<bool> (json::value::*const try_as_type)() const = &json::value::try_as_bool;
};

template<>
struct read_info<i64> {
    static constexpr const char* m_type_name = "i64";
    static constexpr boost::system::result<i64> (json::value::*const try_as_type)() const = &json::value::try_as_int64;
};

template<>
struct read_info<std::string> {
    static constexpr const char* m_type_name = "std::string";
    static constexpr boost::system::result<json::string const&> (json::value::*const try_as_type)() const = &json::value::try_as_string;
};

template<typename T>
struct remove_optional : std::type_identity<T> {};

template<typename T>
struct remove_optional<std::optional<T>> : std::type_identity<T> {};

// static_assert(is_specialisation_of<std::optional<std::string>, std::optional>::value);

template<typename T>
struct reader;

template<typename T>
    requires requires { read_info<T>::m_type_name; }
struct reader<T> {
    static auto read(json::value const& value) -> anyhow::result<T> {
        const auto* const type_name = read_info<T>::m_type_name;
        const auto try_as_type = read_info<T>::try_as_type;

        auto res = (value.*try_as_type)();
        if (!res) {
            return _anyhow_fmt("value was expected to be a {}, it is: {}", type_name, serialize(value));
        }

        if constexpr (std::is_same_v<json::string, std::remove_cvref_t<decltype(*res)>>) {  // special case
            return std::string(*res);
        } else if constexpr (!std::is_const_v<std::remove_reference_t<decltype(*res)>>) {
            return std::move(*res);
        } else {
            return *res;
        }
    }
};

template<typename T>
struct reader<std::vector<T>> {
    static auto read(json::value const& value) -> anyhow::result<std::vector<T>> {
        auto ret = std::vector<T>{};

        const auto array = value.try_as_array();
        if (!array) {
            return _anyhow_fmt("value was expected to be an array, it is: {}", serialize(value));
        }

        for (auto i = 0uz; auto const& value : *array) {
            auto res = reader<T>::read(value);

            if (!res) {
                auto error = anyhow::string_error(fmt::format("failed to read an array element (#{})", i), std::make_unique<anyhow::error>(std::move(res.error())));
                return std::unexpected{std::move(error)};
            }

            ret.emplace_back(std::move(*res));
            ++i;
        }

        return std::move(ret);
    }
};

template<typename T>
    requires requires { field_names<T>::m_names; }
struct reader<T> {
    static auto read(json::value const& value) -> anyhow::result<T> {
        const auto object = value.try_as_object();
        if (!object) {
            return _anyhow_fmt("expected an object, got: {}", serialize(value));
        }

        auto ret = T{};

        auto first_error = std::optional<anyhow::error>{};

        stf::intro::introspector_t<T>::iterate(ret, [&]<typename Field, usize I>(std::integral_constant<usize, I>, Field&& field) {
            if (first_error) {
                return;
            }

            using ActualField = std::remove_cvref_t<Field>;
            if (auto res = read_field<I, ActualField>(*object); !res) {
                first_error.emplace(std::move(res.error()));
                return;
            } else {
                field = std::move(*res);
            }
        });

        if (first_error) {
            return std::unexpected{*first_error};
        } else {
            return ret;
        }
    }

private:
    template<usize I, typename Field>
    static auto read_field(json::object const& from_within) -> anyhow::result<Field> {
        using U = remove_optional<std::remove_cvref_t<Field>>::type;
        if constexpr (!is_specialisation_of<U, std::variant>::value) {
            return read_regular_field<I, Field>(from_within);
        } else {
            return read_variant_field<I>(from_within, std::type_identity<U>{});
        }
    }

    template<usize I, typename Field>
    static auto read_regular_field(json::object const& from_within) -> anyhow::result<Field> {
        using U = remove_optional<std::remove_cvref_t<Field>>::type;

        const auto* const field_name = detail::field_names<T>::m_names[I];
        const auto* const field_ptr = from_within.find(field_name);

        if (field_ptr == from_within.end()) {
            if constexpr (is_specialisation_of<std::remove_cvref_t<Field>, std::optional>::value) {
                return std::nullopt;
            } else {
                return _anyhow_fmt("field #{} (\"{}\") is not optional, yet missing", I, field_name);
            }
        }

        auto const& value = field_ptr->value();

        if (auto res = reader<U>::read(value); !res) {
            auto error =
              anyhow::string_error(fmt::format("failed to read a non-optional field \"{}\" (#{})", field_name, I), std::make_unique<anyhow::error>(std::move(res.error())));
            return std::unexpected{std::move(error)};
        } else {
            return std::move(*res);
        }
    }

    template<usize I, typename... Ts>
    static auto read_variant_field(json::object const& from_within, std::type_identity<std::variant<Ts...>>) -> anyhow::result<std::variant<Ts...>> {
        using namespace std::string_view_literals;

        auto variant_names_str = std::string_view(detail::field_names<T>::m_names[I]);
        auto variant_names = variant_names_str | std::views::split(";"sv);

        auto res = variant_reader<std::variant<Ts...>, Ts...>::try_read(from_within, std::ranges::begin(variant_names));
        if (!res) {
            auto err =
              anyhow::string_error("failed to read a variant (field might exist but an error was produced regardless)", std::make_unique<anyhow::error>(std::move(res.error())));
            return std::unexpected{std::move(err)};
        }

        auto opt = std::move(*res);
        if (!opt) {
            return _anyhow("no suitable key for a variant was found");
        }

        return std::move(*opt);
    }

    template<typename Variant, typename... Vs>
    struct variant_reader;

    template<typename Variant>
    struct variant_reader<Variant, std::monostate> {
        static auto try_read([[maybe_unused]] json::object const& from_within, [[maybe_unused]] auto name_it) -> anyhow::result<std::optional<Variant>> { return std::nullopt; }
    };

    template<typename Variant>
    struct variant_reader<Variant> {
        static auto try_read([[maybe_unused]] json::object const& from_within, [[maybe_unused]] auto name_it) -> anyhow::result<std::optional<Variant>> {
            return _anyhow("no key suitable for the variant exists");
        }
    };

    template<typename Variant, typename V, typename... Vs>
    struct variant_reader<Variant, V, Vs...> {
        static auto try_read(json::object const& from_within, auto name_it) -> anyhow::result<std::optional<Variant>> {
            const auto name = std::string_view(*name_it++);

            auto const& value =  //
              ({
                  const auto* const kv_pair = from_within.find(name);
                  if (kv_pair == from_within.end()) {
                      return variant_reader<Variant, Vs...>::try_read(from_within, std::move(name_it));
                  }

                  kv_pair;
              })->value();

            if (auto res = reader<V>::read(value); !res) {
                auto parent = std::make_unique<anyhow::error>(std::move(res.error()));
                auto err = anyhow::string_error(
                  fmt::format("found a variant member at key \"{}\" (with {} types to check remaining) but failed to deserialise it", name, sizeof...(Vs)), std::move(parent)
                );
                return std::unexpected{std::move(err)};
            } else {
                return Variant{std::move(*res)};
            }
        }
    };
};

}  // namespace detail

namespace types {

constexpr auto message_type_to_string(update_type type) -> std::string_view {
    const auto underlying = std::to_underlying(type);

    if (std::popcount(underlying) != 1) {
        return "";
    }

    // clang-format off
    static const char* const names[] = {
        "message",
        "edited_message",
        "channel_post",
        "edited_channel_post",
        "business_connection",
        "business_message",
        "edited_business_message",
        "deleted_business_messages",
        "message_reaction",
        "message_reaction_count",
        "inline_query",
        "chosen_inline_result",
        "callback_query",
        "shipping_query",
        "pre_checkout_query",
        "poll",
        "poll_answer",
        "my_chat_member",
        "chat_member",
        "chat_join_request",
        "chat_boost",
        "removed_chat_boost",
    };
    // clang-format on

    return names[std::countr_zero(underlying)];
}

}  // namespace types

auto get_me(connection& conn) -> boost::asio::awaitable<anyhow::result<types::user>> {
    const auto res = TRYC(co_await conn.make_request("getMe"));

    if (!res.is_object()) {
        co_return _anyhow_fmt("expected an object in the response, got: {}", serialize(res));
    }

    auto const& res_object = res.as_object();

    if (const auto* const ok = res_object.find("ok"); ok == res_object.end()) {
        co_return _anyhow_fmt("expected an 'ok' key in the response object. response: {}", serialize(res));
    } else if (ok->value() != true) {
        co_return _anyhow_fmt("'ok' is not true. response: {}", serialize(res));
    }

    if (const auto* const result = res_object.find("result"); result == res_object.end()) {
        co_return _anyhow_fmt("expected an 'result' key in the response object. response: {}", serialize(res));
    } else if (!result->value().is_object()) {
        co_return _anyhow_fmt("expected 'result' to be an object. response: {}", serialize(res));
    } else {
        co_return detail::reader<types::user>::read(result->value());
    }
}

auto get_updates(connection& conn, types::update_type type, u64 timeout_seconds, u64 offset) -> boost::asio::awaitable<anyhow::result<std::vector<types::update>>> {
    static constexpr auto no_unique_update_types = std::popcount(std::to_underlying(types::update_type::all));

    auto allowed_updates = json::array{};
    allowed_updates.reserve(no_unique_update_types);

    for (auto i = 0u; i < no_unique_update_types; i++) {
        const auto to_check = static_cast<types::update_type>(1 << i);
        const auto is_contained = (type & to_check) != types::update_type::none;

        if (!is_contained) {
            continue;
        }

        allowed_updates.emplace_back(message_type_to_string(to_check));
    }

    auto body = json::object{
      {"offset", offset},
      {"limit", 100},
      {"timeout", timeout_seconds},
      {"allowed_updates", std::move(allowed_updates)},
    };

    const auto res = TRYC(co_await conn.make_request("getUpdates", body, request_verb::post));

    if (!res.is_object()) {
        co_return _anyhow_fmt("expected an object in the response, got: {}", serialize(res));
    }

    auto const& res_object = res.as_object();

    if (const auto* const ok = res_object.find("ok"); ok == res_object.end()) {
        co_return _anyhow_fmt("expected an 'ok' key in the response object. response: {}", serialize(res));
    } else if (ok->value() != true) {
        co_return _anyhow_fmt("'ok' is not true. response: {}", serialize(res));
    }

    if (const auto* const result = res_object.find("result"); result == res_object.end()) {
        co_return _anyhow_fmt("expected an 'result' key in the response object. response: {}", serialize(res));
    } else {
        co_return detail::reader<std::vector<types::update>>::read(result->value());
    }
}

auto send_message(connection& conn, std::variant<i64, std::string_view> chat_id, std::string_view text) -> boost::asio::awaitable<anyhow::result<types::message>> {
    auto body = std::visit(
      [&text](auto chat_id) {
          return json::object{
            {"chat_id", chat_id},
            {"text", text},
          };
      },
      chat_id
    );

    const auto res = TRYC(co_await conn.make_request("sendMessage", body, request_verb::post));

    if (!res.is_object()) {
        co_return _anyhow_fmt("expected an object in the response, got: {}", serialize(res));
    }

    auto const& res_object = res.as_object();

    if (const auto* const ok = res_object.find("ok"); ok == res_object.end()) {
        co_return _anyhow_fmt("expected an 'ok' key in the response object. response: {}", serialize(res));
    } else if (ok->value() != true) {
        co_return _anyhow_fmt("'ok' is not true. response: {}", serialize(res));
    }

    if (const auto* const result = res_object.find("result"); result == res_object.end()) {
        co_return _anyhow_fmt("expected an 'result' key in the response object. response: {}", serialize(res));
    } else if (!result->value().is_object()) {
        co_return _anyhow_fmt("expected 'result' to be an object. response: {}", serialize(res));
    } else {
        co_return detail::reader<types::message>::read(result->value());
    }
}
}  // namespace john::telegram::api
