#pragma once

#include <expected>
#include <type_traits>

#include <boost/asio.hpp>

using boost::asio::constraint_t;
using std::conditional_t;
using std::decay_t;
using std::false_type;
using std::is_convertible;
using std::is_same;

namespace john {

template<typename CompletionToken>
class as_expected_t {
public:
    struct default_constructor_tag {};

    constexpr as_expected_t(default_constructor_tag = {}, CompletionToken token = {})
        : m_token(static_cast<CompletionToken&&>(token)) {}

    template<typename T>
    constexpr explicit as_expected_t(T&& completion_token)
        : m_token(static_cast<T&&>(completion_token)) {}

    template<typename InnerExecutor>
    struct executor_with_default : InnerExecutor {
        typedef as_expected_t default_completion_token_type;

        template<typename InnerExecutor1>
            requires(!std::is_same_v<InnerExecutor1, executor_with_default>) && (std::is_convertible_v<InnerExecutor1, InnerExecutor>)
        executor_with_default(const InnerExecutor1& ex) noexcept
            : InnerExecutor(ex) {}
    };

    template<typename T>
    using as_default_on_t = typename T::template rebind_executor<executor_with_default<typename T::executor_type>>::other;

    template<typename T>
    static auto as_default_on(T&& object) -> typename decay_t<T>::template rebind_executor<executor_with_default<typename decay_t<T>::executor_type>>::other {
        return static_cast<T&&>(object);
    }

    // private:
    CompletionToken m_token;
};

struct partial_as_expected {
    constexpr partial_as_expected() {}

    template<typename CompletionToken>
    [[nodiscard]] inline constexpr auto operator()(CompletionToken&& completion_token) const -> as_expected_t<decay_t<CompletionToken>> {
        return as_expected_t<decay_t<CompletionToken>>(static_cast<CompletionToken&&>(completion_token));
    }
};

inline constexpr partial_as_expected as_expected;

}  // namespace john

namespace john::detail {

template<typename... Ts>
struct expected_type : std::type_identity<std::expected<std::tuple<std::decay_t<Ts>...>, boost::system::error_code>> {};

template<>
struct expected_type<> : std::type_identity<std::expected<void, boost::system::error_code>> {};

template<typename T>
struct expected_type<T> : std::type_identity<std::expected<std::decay_t<T>, boost::system::error_code>> {};

template<typename... Ts>
using expected_type_t = typename expected_type<Ts...>::type;

template<typename Handler>
class as_expected_handler {
public:
    typedef void result_type;

    template<typename CompletionToken>
    as_expected_handler(as_expected_t<CompletionToken> e)
        : m_handler(static_cast<CompletionToken&&>(e.m_token)) {}

    template<typename RedirectedHandler>
    as_expected_handler(RedirectedHandler&& h)
        : m_handler(static_cast<RedirectedHandler&&>(h)) {}

    template<typename... Args>
    void operator()(boost::system::error_code ec, Args&&... args) {
        if (ec != boost::system::error_code{}) {
            static_cast<Handler&&>(m_handler)(expected_type_t<Args...>(std::unexpect, ec));
        } else {
            static_cast<Handler&&>(m_handler)(expected_type_t<Args...>(std::forward<Args>(args)...));
        }
    }

    // private:
    Handler m_handler;
};

template<typename Handler>
inline auto asio_handler_is_continuation(as_expected_handler<Handler>* this_handler) -> bool {
    return boost_asio_handler_cont_helpers::is_continuation(this_handler->m_handler);
}

template<typename Signature>
struct as_expected_signature;

template<typename R, typename... Args>
struct as_expected_signature<R(boost::system::error_code, Args...)> {
    typedef R type(expected_type_t<Args...>);
};

template<typename R, typename... Args>
struct as_expected_signature<R(boost::system::error_code, Args...)&> {
    typedef R type(expected_type_t<Args...>) &;
};

template<typename R, typename... Args>
struct as_expected_signature<R(boost::system::error_code, Args...) &&> {
    typedef R type(expected_type_t<Args...>) &&;
};

template<typename R, typename... Args>
struct as_expected_signature<R(boost::system::error_code, Args...) noexcept> {
    typedef R type(expected_type_t<Args...>) noexcept;
};

template<typename R, typename... Args>
struct as_expected_signature<R(boost::system::error_code, Args...) & noexcept> {
    typedef R type(expected_type_t<Args...>) & noexcept;
};

template<typename R, typename... Args>
struct as_expected_signature<R(boost::system::error_code, Args...) && noexcept> {
    typedef R type(expected_type_t<Args...>) && noexcept;
};

}  // namespace john::detail

template<typename CompletionToken, typename... Signatures>
struct boost::asio::async_result<john::as_expected_t<CompletionToken>, Signatures...>
    : async_result<CompletionToken, typename john::detail::as_expected_signature<Signatures>::type...> {
    template<typename Initiation>
    struct init_wrapper : detail::initiation_base<Initiation> {
        using detail::initiation_base<Initiation>::initiation_base;

        template<typename Handler, typename... Args>
        void operator()(Handler&& handler, Args&&... args) && {
            static_cast<Initiation&&>(*this)(john::detail::as_expected_handler<decay_t<Handler>>(static_cast<Handler&&>(handler)), static_cast<Args&&>(args)...);
        }

        template<typename Handler, typename... Args>
        void operator()(Handler&& handler, Args&&... args) const& {
            static_cast<const Initiation&>(*this)(john::detail::as_expected_handler<decay_t<Handler>>(static_cast<Handler&&>(handler)), static_cast<Args&&>(args)...);
        }
    };

    template<typename Initiation, typename RawCompletionToken, typename... Args>
    static auto initiate(Initiation&& initiation, RawCompletionToken&& token, Args&&... args)
      -> decltype(async_initiate<
                  conditional_t<is_const<remove_reference_t<RawCompletionToken>>::value, const CompletionToken, CompletionToken>,
                  typename john::detail::as_expected_signature<Signatures>::type...>(
        init_wrapper<decay_t<Initiation>>(static_cast<Initiation&&>(initiation)),
        token.m_token,
        static_cast<Args&&>(args)...
      )) {
        return async_initiate<
          conditional_t<is_const<remove_reference_t<RawCompletionToken>>::value, const CompletionToken, CompletionToken>,
          typename john::detail::as_expected_signature<Signatures>::type...>(
          init_wrapper<decay_t<Initiation>>(static_cast<Initiation&&>(initiation)), token.m_token, static_cast<Args&&>(args)...
        );
    }
};

template<template<typename, typename> class Associator, typename Handler, typename DefaultCandidate>
struct boost::asio::associator<Associator, john::detail::as_expected_handler<Handler>, DefaultCandidate> : Associator<Handler, DefaultCandidate> {
    static auto get(const john::detail::as_expected_handler<Handler>& h) noexcept -> typename Associator<Handler, DefaultCandidate>::type {
        return Associator<Handler, DefaultCandidate>::get(h.m_handler);
    }

    static auto get(const john::detail::as_expected_handler<Handler>& h, const DefaultCandidate& c) noexcept -> decltype(Associator<Handler, DefaultCandidate>::get(h.m_handler, c)
                                                                                                             ) {
        return Associator<Handler, DefaultCandidate>::get(h.m_handler, c);
    }
};

template<typename... Signatures>
struct boost::asio::async_result<john::partial_as_expected, Signatures...> {
    template<typename Initiation, typename RawCompletionToken, typename... Args>
    static auto initiate(Initiation&& initiation, RawCompletionToken&&, Args&&... args) -> decltype(async_initiate<Signatures...>(
                                                                                          static_cast<Initiation&&>(initiation),
                                                                                          john::as_expected_t<default_completion_token_t<associated_executor_t<Initiation>>>{},
                                                                                          static_cast<Args&&>(args)...
                                                                                        )) {
        return async_initiate<Signatures...>(
          static_cast<Initiation&&>(initiation), john::as_expected_t<default_completion_token_t<associated_executor_t<Initiation>>>{}, static_cast<Args&&>(args)...
        );
    }
};
