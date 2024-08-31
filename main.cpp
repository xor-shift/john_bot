#include <irc/client.hpp>

#include <alloc.hpp>
#include <assio/as_expected.hpp>
#include <error.hpp>
#include <telegram/client.hpp>

#include <stuff/core/integers.hpp>
#include <stuff/core/try.hpp>
#include <stuff/core/visitor.hpp>

#include <spdlog/spdlog.h>
#include <magic_enum.hpp>

#include <boost/asio.hpp>

struct echoer final : john::thing {
    ~echoer() override = default;

    auto worker(john::bot& bot) -> boost::asio::awaitable<void> override {
        m_bot = &bot;
        co_return;
    }

    auto handle(john::internal_message const& message) -> boost::asio::awaitable<void> override {
        const auto visitor = stf::multi_visitor{
          [&](john::incoming_message const& message) -> boost::asio::awaitable<void> {
              co_await m_bot->new_message(john::outgoing_message{
                .m_target = message.m_return_to_sender,
                .m_content = message.m_content,
              });

              co_return;
          },
          [](auto&&) -> boost::asio::awaitable<void> { co_return; }
        };

        co_await std::visit(visitor, message);
    }

private:
    john::bot* m_bot = nullptr;
};

auto async_main() -> boost::asio::awaitable<void> {
    auto executor = co_await boost::asio::this_coro::executor;

    auto bot = john::bot{};
    bot.add_thing<john::irc::irc_client>(executor);
    bot.add_thing<john::telegram::client>(executor);
    bot.add_thing<echoer>();

    auto res = co_await bot.run();

    if (!res) {
        spdlog::error("{}", res.error().description());
    }
}

auto main() -> int {
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Hello, world!");

    auto context = boost::asio::io_context{1};

    boost::asio::co_spawn(context, async_main(), boost::asio::detached);

    context.run();
}
