#include <telegram/connection.hpp>

#include <assio/as_expected.hpp>

#include <stuff/core/try.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/json.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = asio::ssl;
namespace json = boost::json;

using asio::awaitable;
using asio::ip::tcp;

namespace john::telegram {

namespace detail {

struct connection_impl {
    asio::any_io_executor& m_executor;
    ssl::context m_ssl_context;
    ssl::stream<assify<beast::tcp_stream>> m_stream;

    configuration m_config;

    static auto make(asio::any_io_executor& executor, configuration config) -> std::unique_ptr<connection_impl> {
        return std::unique_ptr<connection_impl>{new connection_impl(executor, std::move(config))};
    }

    auto init_or_reinit() -> awaitable<anyhow::result<void>> {
        auto resolver = assify<tcp::resolver>(m_executor);

        if (!SSL_set_tlsext_host_name(m_stream.native_handle(), m_config.m_host.c_str())) {
            co_return _anyhow("SSL error");
        }

        auto endpoint = TRYC((co_await resolver.async_resolve(m_config.m_host, "443")));
        TRYC((co_await get_lowest_layer(m_stream).async_connect(endpoint->endpoint())));
        TRYC((co_await m_stream.async_handshake(asio::ssl::stream_base::client)));

        co_return anyhow::result<void>{};
    }

    auto make_request(std::string_view endpoint, http::verb method = http::verb::get) -> awaitable<anyhow::result<boost::json::value>> {
        return make_request_impl(endpoint, "", method);
    }

    auto make_request(std::string_view endpoint, json::object const& body, http::verb method = http::verb::get) -> awaitable<anyhow::result<boost::json::value>> {
        return make_request_impl(endpoint, serialize(body), method);
    }

private:
    connection_impl(asio::any_io_executor& executor, configuration config)
        : m_executor(executor)
        , m_ssl_context(ssl::context::sslv23)
        , m_stream(executor, m_ssl_context)
        , m_config(std::move(config)) {}

    auto make_request_impl(std::string_view endpoint, std::string body, http::verb method = http::verb::get) -> awaitable<anyhow::result<boost::json::value>> {
        auto req = http::request<http::string_body>{method, fmt::format("/bot{}/{}", m_config.m_token, endpoint), 11};
        req.set(http::field::host, m_config.m_host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");

        if (!body.empty()) {
            req.set(http::field::accept, "application/json");
            req.body() = std::move(body);
        }

        req.prepare_payload(); // lol

        TRYC((co_await http::async_write(m_stream, req)));

        auto buffer = beast::flat_buffer{};
        auto response = http::response<http::dynamic_body>{};
        TRYC((co_await http::async_read(m_stream, buffer, response)));

        /*{
            spdlog::trace("{}", (std::stringstream{} << req).str());
            spdlog::trace("{}", (std::stringstream{} << response).str());
        }*/

        if (response.result_int() != 200) {
            auto body = std::string {};

            for (auto i = 0uz; auto const& buffer : response.body().data()) {
                auto data = std::string_view(boost::asio::buffer_cast<const char*>(buffer), buffer.size());
                std::ranges::copy(data, back_inserter(body));
            }

            co_return _anyhow_fmt("call to /{} returned {} (expected 200). body: {}", endpoint, response.result_int(), body);
        }

        auto parser = json::stream_parser{};

        for (auto i = 0uz; auto const& buffer : response.body().data()) {
            auto data = std::string_view(boost::asio::buffer_cast<const char*>(buffer), buffer.size());

            // spdlog::debug("buffer #{}: {}", i++, data);

            auto ec = boost::system::error_code{};
            parser.write(data, ec);

            if (ec != boost::system::error_code{}) {
                co_return std::unexpected{ec};
            }
        }

        auto json_value = parser.release();

        co_return json_value;
    }
};

}  // namespace detail

connection::connection(boost::asio::any_io_executor& executor, configuration configuration)
    : m_impl(detail::connection_impl::make(executor, std::move(configuration))) {}

connection::~connection() = default;

auto connection::init_or_reinit() -> boost::asio::awaitable<anyhow::result<void>> { return m_impl->init_or_reinit(); }

auto connection::make_request(std::string_view endpoint, request_verb verb) -> boost::asio::awaitable<anyhow::result<boost::json::value>> {
    auto real_verb = http::verb{};

    switch (verb) {
        case request_verb::get: real_verb = http::verb::get;
        case request_verb::post: real_verb = http::verb::post;
    }

    return m_impl->make_request(endpoint, real_verb);
}

auto connection::make_request(std::string_view endpoint, boost::json::object const& body, request_verb verb) -> boost::asio::awaitable<anyhow::result<boost::json::value>> {
    auto real_verb = http::verb{};

    switch (verb) {
        case request_verb::get: real_verb = http::verb::get;
        case request_verb::post: real_verb = http::verb::post;
    }

    return m_impl->make_request(endpoint, body, real_verb);
}

}  // namespace john::telegram
