#include <alloc.hpp>
#include <assio/as_expected.hpp>
#include <bot.hpp>
#include <error.hpp>
#include <irc/client.hpp>
#include <sqlite/aggregate.hpp>
#include <sqlite/sqlite.hpp>
#include <telegram/client.hpp>
#include <things/dummy.hpp>
#include <things/relay.hpp>

#include <stuff/core/integers.hpp>
#include <stuff/core/try.hpp>
#include <stuff/core/visitor.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <magic_enum.hpp>

namespace asio = boost::asio;
using anyhow::result;
using asio::awaitable;

struct irc_entry {
    i32 m_id;
    std::string m_server;
    i32 m_port;
    std::string m_username;
    std::string m_realname;
};

template<typename Thing, typename... Args>
auto add_thing(john::bot& bot, Args&&... args) -> boost::asio::awaitable<void> {
    const auto _ = co_await bot.queue_message(john::message{
      .m_from = "",
      .m_to = "bot",

      .m_serial = 0uz,
      .m_reply_serial = std::nullopt,

      .m_payload =
        john::payloads::add_thing{
          .m_thing = std::make_unique<Thing>(std::forward<Args>(args)...),
        },
    });

    co_return;
}

auto setup_single_irc(john::bot& bot, sqlite3& db, irc_entry entry) -> awaitable<result<void>> {
    auto nicks = TRYC(sqlite::query<std::string>(db, "select nick from irc_nick_choices where irc_id = ?", entry.m_id));
    auto channels = TRYC(sqlite::query<std::string>(db, "select channel from irc_channels where irc_id = ?", entry.m_id));

    if (nicks.empty()) {
        co_return _anyhow("no nicks given");
    }

    co_await add_thing<john::irc::irc_client>(
      bot, bot.get_executor(),
      john::irc::configuration{
        .m_identifier = fmt::format("irc_{}", entry.m_id),

        .m_server = std::move(entry.m_server),
        .m_port = static_cast<u16>(entry.m_port),
        .m_use_ssl = false,

        .m_password = std::nullopt,
        .m_nicks = std::move(nicks),
        .m_user = std::move(entry.m_username),
        .m_realname = std::move(entry.m_realname),
        .m_channels = std::move(channels),
      }
    );

    co_return result<void>{};
}

auto setup_irc(john::bot& bot, sqlite3& db) -> awaitable<result<void>> {
    for (auto&& entry : TRYC(sqlite::query<irc_entry>(db, "select rowid, * from clients_irc"))) {
        auto id = entry.m_id;

        auto res = co_await setup_single_irc(bot, db, std::move(entry));

        if (!res) {
            spdlog::warn("failed to set an irc client up (id: \"{}\"): {}", id, static_cast<john::error const&>(res.error()));
        }
    }

    co_return result<void>{};
}

auto setup_telegram(john::bot& bot, sqlite3& db) -> awaitable<result<void>> {
    for (auto const& [id, token] : TRYC(sqlite::query<i64, std::string>(db, "select * from clients_telegram"))) {
        co_await add_thing<john::telegram::client>(
          bot, bot.get_executor(),
          john::telegram::configuration{
            .m_identifier = fmt::format("telegram_{}", id),
            .m_token = fmt::format("{}:{}", id, token),
            .m_host = "api.telegram.org",
          }
        );
    }

    co_return result<void>{};
}

auto setup_bot(john::bot& bot, sqlite3& db) -> awaitable<void> {
    if (auto res = co_await setup_irc(bot, db); !res) {
        spdlog::warn("failed to set up any irc clients");
    }

    if (auto res = co_await setup_telegram(bot, db); !res) {
        spdlog::warn("failed to set up any telegram clients");
    }

    co_await add_thing<john::things::dummy>(bot);

    co_await add_thing<john::things::relay>(bot);

    co_return;
}

auto launch_bot() -> awaitable<result<void>> {
    namespace asio = boost::asio;
    using boost::asio::ip::tcp;

    auto db = TRYC(sqlite::open("db.sqlite"));

    auto executor = co_await asio::this_coro::executor;

    auto bot = john::bot(db, executor);

    boost::asio::co_spawn(
      executor,
      [&]() -> awaitable<void> {
          co_await boost::asio::deadline_timer(co_await boost::asio::this_coro::executor, boost::posix_time::seconds(1)).async_wait(boost::asio::use_awaitable);

          co_await setup_bot(bot, *db);

          co_return;
      },
      boost::asio::detached
    );

    auto res = co_await bot.run();

    if (!res) {
        spdlog::error("{}", res.error().description());
    }

    co_return result<void>{};
}

auto async_main() -> awaitable<void> {
    auto executor = co_await boost::asio::this_coro::executor;

    if (auto res = co_await launch_bot(); !res) {
        spdlog::error("{}", static_cast<john::error const&>(res.error()));
    }

    co_return;
}

auto main() -> int {
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Hello, world!");

    auto context = boost::asio::io_context{1};
    // auto guard = boost::asio::make_work_guard(context);

    boost::asio::co_spawn(context, async_main(), boost::asio::detached);

    context.run();
}
