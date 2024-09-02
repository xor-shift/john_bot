#pragma once

#include <error.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/json.hpp>

namespace john::telegram {

struct configuration {
    std::string m_id;
    std::string m_token;
    std::string m_host;
};

namespace detail {

struct connection_impl;

}

enum class request_verb {
    get,
    post,
};

struct connection {
    connection(boost::asio::any_io_executor& executor, configuration configuration);

    // connecton_impl is yet incomplete so the destructor has to be defined elsewhere
    ~connection();

    auto init_or_reinit() -> boost::asio::awaitable<anyhow::result<void>>;

    auto make_request(std::string_view endpoint, request_verb verb = request_verb::get) -> boost::asio::awaitable<anyhow::result<boost::json::value>>;

    auto make_request(std::string_view endpoint, boost::json::object const& body, request_verb verb = request_verb::get)
      -> boost::asio::awaitable<anyhow::result<boost::json::value>>;

private:
    std::unique_ptr<detail::connection_impl> m_impl;
};

}  // namespace john::telegram
