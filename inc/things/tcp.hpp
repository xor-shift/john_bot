#pragma once

#include <bot.hpp>

#include <boost/asio/ip/tcp.hpp>

namespace john::things {

struct tcp_thing final : thing {
    struct pimple;

    tcp_thing(boost::asio::any_io_executor executor);

    ~tcp_thing() override;

    auto get_id() const -> std::string_view override { return "tcp_thing"; }

    auto worker(john::bot& bot) -> boost::asio::awaitable<anyhow::result<void>> override;

    auto handle(john::message const& msg) -> boost::asio::awaitable<anyhow::result<void>> override;

private:
    std::unique_ptr<pimple> m_pimpl;

    struct client {
        boost::asio::ip::tcp::socket m_sock;
        assify<boost::asio::experimental::concurrent_channel<void(boost::system::error_code, std::string)>> m_chan;
    };

    std::atomic<usize> m_last_client_id{0uz};
    std::mutex m_clients_mutex;
    std::unordered_map<usize, std::shared_ptr<client>> m_clients;

    auto client_worker(boost::asio::ip::tcp::socket socket) -> boost::asio::awaitable<anyhow::result<void>>;
};

}  // namespace john::things
