#include <things/tcp.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace asio = boost::asio;
namespace ip = asio::ip;

using anyhow::result;
using asio::awaitable;
using ip::tcp;

namespace john::things {

struct tcp_thing::pimple {
    pimple(asio::any_io_executor const& executor)
        : m_acceptor(executor, tcp::endpoint(ip::address_v4(), 1337)) {}

    assify<tcp::acceptor> m_acceptor;
};

tcp_thing::tcp_thing(asio::any_io_executor executor)
    : m_pimpl(std::make_unique<pimple>(executor)) {}

tcp_thing::~tcp_thing() = default;

auto tcp_thing::worker(john::bot& bot) -> awaitable<result<void>> {
    for (;;) {
        auto res = co_await m_pimpl->m_acceptor.async_accept();

        if (!res) {
            spdlog::warn("failed to accept a client (how?) (TODO)");
            continue;
        }

        auto socket = auto(std::move(*res));

        asio::co_spawn(bot.get_executor(), client_worker(std::move(socket)), asio::detached);
    }
    co_return result<void>{};  //
}

auto tcp_thing::client_worker(tcp::socket socket) -> awaitable<result<void>> {
    auto endpoint = socket.remote_endpoint();
    const auto client_internal_id = fmt::format("tcp_thing_{}_{}", endpoint.address().to_string(), endpoint.port());

    spdlog::debug("client_worker for {} has started", client_internal_id);

    auto cl = std::make_shared<client>(client{
      .m_sock = std::move(socket),
      .m_chan{co_await asio::this_coro::executor, 16},
    });

    const auto client_id = ++m_last_client_id;

    {
        const auto _ = std::unique_lock{m_clients_mutex};
        m_clients.emplace(client_id, cl);
    }

    for (;;) {
        auto res = co_await cl->m_chan.async_receive();
        if (!res) {
            spdlog::warn("client_worker for {} has received an error through the message channel {}", client_internal_id, res.error().what());
            break;
        }

        co_await cl->m_sock.async_send(asio::buffer(std::bit_cast<std::array<u8, 4>>(static_cast<u32>(res->size()))));
        co_await cl->m_sock.async_send(asio::buffer(*res));
    }

    /*auto dummy = std::array<u8, 1>{};
    const auto _ = co_await cl->m_sock.async_receive(asio::mutable_buffer{dummy.data(), dummy.size()});*/

    spdlog::debug("client_worker for {} has exited", client_internal_id);

    co_return result<void>{};
}

auto tcp_thing::handle(john::message const& msg) -> awaitable<result<void>> {
    if (const auto* const ptr = std::get_if<payloads::outgoing_message>(&msg.m_payload); ptr != nullptr) {
        const auto _ = std::unique_lock{m_clients_mutex};
        for (auto& [id, cl] : m_clients) {
            cl->m_chan.try_send(boost::system::error_code{}, ptr->m_content);
        }
    }

    co_return result<void>{};  //
}

}  // namespace john::things
