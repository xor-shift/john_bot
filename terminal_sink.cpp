#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <sys/ioctl.h>
#include <termios.h>

#include <cstdio>
#include <ranges>

namespace asio = boost::asio;
namespace ip = asio::ip;

using asio::awaitable;
using error_code = boost::system::error_code;
using ip::tcp;

template<typename Signature>
using chan = boost::asio::experimental::concurrent_channel<Signature>;

namespace events {

struct resize {
    size_t m_rows;
    size_t m_cols;
};

}  // namespace events

using event = std::variant<events::resize>;

#define ESC "\x1B"
#define CSI ESC "["

// shorthands
#define CLR CSI "2J"
#define SHOW_CURSOR CSI "?25h"
#define HIDE_CURSOR CSI "?25l"
#define SGR(n) "\x1B[" #n "m"

struct terminal {
    terminal() {
        std::printf(HIDE_CURSOR SGR(102) SGR(30));

        auto t = termios{};
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        const auto _ = std::fflush(stdout);
    }

    ~terminal() {
        std::printf(SGR(0) CLR SHOW_CURSOR);

        auto t = termios{};
        tcgetattr(0, &t);
        t.c_lflag |= ECHO;
        tcsetattr(0, TCSANOW, &t);

        const auto _ = std::fflush(stdout);
    }

    void resize(size_t rows, size_t cols) {
        m_rows = rows;
        m_cols = cols;
        redraw();
    }

    void resize() {
        auto win_size = winsize{};
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &win_size);

        return resize(win_size.ws_row, win_size.ws_col);
    }

    void clear() {
        m_lines.clear();
        redraw();
    }

    template<typename... Ts>
        requires(sizeof...(Ts) != 0)
    void line(std::format_string<Ts...> m_format, Ts&&... ts) {
        return line(std::format(std::move(m_format), std::forward<Ts>(ts)...));
    }

    void line(std::string str) {
        m_lines.emplace_back(std::move(str));
        redraw();
    }

private:
    size_t m_rows = 0;
    size_t m_cols = 0;

    std::vector<std::string> m_lines;

    void redraw() {
        std::printf(CLR);

        for (auto printed_rows = 0uz; auto line : m_lines | std::views::reverse | std::views::transform([](auto const& str) -> std::string_view { return str; })) {
            auto line_rows = std::vector<std::string_view>{};

            while (!line.empty()) {
                const auto chars_to_consume = std::min(m_cols, std::min(line.size(), line.find('\n')));

                const auto cur = line.substr(0, chars_to_consume);
                line = line.substr(chars_to_consume);

                line_rows.emplace_back(cur);
            }

            for (auto row : line_rows | std::views::reverse) {
                std::printf(CSI "%zu;%zuH", (m_rows - printed_rows), 0uz);
                std::printf("%.*s", static_cast<int>(row.size()), row.data());

                if (++printed_rows == m_rows) {
                    break;
                }
            }

            if (printed_rows == m_rows) {
                break;
            }
        }

        const auto _ = std::fflush(stdout);
    }
};

template<typename Executor>
struct state {
    Executor& m_executor;
    chan<void(error_code, event)> m_ch;
    terminal m_term;
    tcp::socket m_sock;

    void close() {
        m_ch.close();
        m_sock.close();
    }
};

template<typename Executor>
auto signal_handler_task(state<Executor>& st) -> awaitable<void> {
    auto signal_set = asio::signal_set{st.m_executor, SIGINT, SIGWINCH};

    for (;;) {
        const auto [ec, signum] = co_await signal_set.async_wait(asio::as_tuple(asio::use_awaitable));

        if (signum == SIGINT) {
            st.close();
            break;
        }

        if (ec) {
            break;
        }

        if (signum == SIGWINCH) {
            auto win_size = winsize{};
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &win_size);

            co_await st.m_ch.async_send(
              {},
              events::resize{
                .m_rows = win_size.ws_row,
                .m_cols = win_size.ws_col,
              }
            );
        }
    }

    co_return;
}

template<typename Executor>
auto tcp_worker(state<Executor>& st) -> awaitable<void> {
    for (;;) {
        st.m_term.line("connecting...");
        const auto [ec_connect] = co_await st.m_sock.async_connect(tcp::endpoint(ip::address_v4({127, 0, 0, 1}), 1337), asio::as_tuple(asio::use_awaitable));

        if (ec_connect != error_code{}) {
            if (!st.m_ch.is_open()) {
                st.m_term.line("tcp_worker exiting (after async_connect) as the channel is closed");
                break;
            }

            st.m_term.line("failed to connect, reconnecting in 3 seconds. error: {}", ec_connect.what());
            co_await asio::deadline_timer{st.m_executor, boost::posix_time::seconds(3)}.async_wait();
            continue;
        }

        st.m_term.clear();
        st.m_term.line("connected!");

        for (;;) {
            auto header_buffer = std::array<uint8_t, 4>{};
            // const auto [ec_read_header, read_header_bytes] = co_await st.m_sock.async_read_some(header_buffer, asio::as_tuple(asio::use_awaitable));
            const auto [ec_header_read, read_header_bytes] =
              co_await st.m_sock.async_receive(asio::mutable_buffer{header_buffer.data(), header_buffer.size()}, asio::as_tuple(asio::use_awaitable));
            if (ec_header_read != error_code{}) {
                st.m_term.line("error while reading header: {}", ec_header_read.what());
                break;
            }

            const auto size = std::bit_cast<uint32_t>(header_buffer);
            auto str = std::string(size, 'A');
            const auto [ec_str_read, read_str_bytes] = co_await st.m_sock.async_receive(asio::buffer(str), asio::as_tuple(asio::use_awaitable));
            if (ec_str_read != error_code{}) {
                st.m_term.line("error while reading content: {}", ec_header_read.what());
                break;
            }

            st.m_term.line(std::move(str));
        }

        if (st.m_ch.is_open()) {
            continue;
        }

        st.m_term.line("disconnected!");
        st.m_term.line("retrying in a bit");
        co_await asio::deadline_timer{st.m_executor, boost::posix_time::seconds(3)}.async_wait();

        break;
    }

    st.m_term.line("exiting the tcp worker");

    co_return;
}

template<typename Executor>
auto async_main(state<Executor>& st) -> awaitable<void> {
    for (;;) {
        const auto [ec, event] = co_await st.m_ch.async_receive(asio::as_tuple(asio::use_awaitable));

        if (ec != error_code{}) {
            break;
        }
    }
}

auto main(int argc, char** argv) -> int {
    auto executor = asio::io_context{};

    auto st = state<asio::io_context>{
      .m_executor = executor,
      .m_ch = chan<void(error_code, event)>{executor},
      .m_term = terminal{},
      .m_sock = tcp::socket{executor.get_executor()},
    };

    st.m_term.resize();

    asio::co_spawn(executor, signal_handler_task(st), asio::detached);
    asio::co_spawn(executor, tcp_worker(st), asio::detached);
    asio::co_spawn(executor, async_main(st), asio::detached);

    executor.run();
}
