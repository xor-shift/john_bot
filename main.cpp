#include <alloc.hpp>
#include <assio/as_expected.hpp>
#include <error.hpp>
#include <irc/client.hpp>
#include <sqlite/aggregate.hpp>
#include <sqlite/sqlite.hpp>
#include <telegram/client.hpp>

#include <stuff/core/integers.hpp>
#include <stuff/core/try.hpp>
#include <stuff/core/visitor.hpp>

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>
#include <magic_enum.hpp>

/*auto exec_schema(sqlite3& db) -> anyhow::result<void> {
#include <generated/bootstrap.h>
#include <generated/schema.h>

    auto schema_res = sqlite::exec(db, reinterpret_cast<const char*>(schema_str));
    if (!schema_res) {
        return std::unexpected{anyhow::string_error("failed to apply schema", std::move(schema_res.error()))};
    }

    auto bootstrap_res = sqlite::exec(db, reinterpret_cast<const char*>(bootstrap_str));
    if (!bootstrap_res) {
        return std::unexpected{anyhow::string_error("failed to bootstrap", std::move(bootstrap_res.error()))};
    }

    return {};
}*/

auto launch_bot() -> boost::asio::awaitable<anyhow::result<void>> {
    namespace asio = boost::asio;
    using boost::asio::ip::tcp;

    auto db = TRYC(sqlite::open("db.sqlite"));

    auto executor = co_await asio::this_coro::executor;

    auto bot = john::bot(db, executor);

    bot.add_thing<echoer>(bot);

    for (auto const& [id, token] : TRYC(sqlite::query<i64, std::string>(*db, "select * from clients_telegram"))) {
        bot.add_thing<john::telegram::client>(
          executor,
          john::telegram::configuration{
            .m_identifier = fmt::format("telegram_{}", id),
            .m_token = fmt::format("{}:{}", id, token),
            .m_host = "api.telegram.org",
          }
        );
    }

    struct irc_entry {
        i32 m_id;
        std::string m_server;
        i32 m_port;
        std::string m_username;
        std::string m_realname;
    };

    for (auto&& entry : TRYC(sqlite::query<irc_entry>(*db, "select rowid, * from clients_irc"))) {
        auto nicks = TRYC(sqlite::query<std::string>(*db, "select nick from irc_nick_choices where irc_id = ?", entry.m_id));
        auto channels = TRYC(sqlite::query<std::string>(*db, "select channel from irc_channels where irc_id = ?", entry.m_id));

        if (nicks.empty()) {
            spdlog::error("0 nicks given for the irc configuration with the id {}", entry.m_id);
            continue;
        }

        bot.add_thing<john::irc::irc_client>(
          executor,
          john::irc::configuration{
            .m_identifier = fmt::format("irc_{}", entry.m_id),
            .m_nicks = std::move(nicks),
            .m_user = std::move(entry.m_username),
            .m_realname = std::move(entry.m_realname),
            .m_channels = std::move(channels),
          }
        );
    }

    auto res = co_await bot.run();

    if (!res) {
        spdlog::error("{}", res.error().description());
    }

    co_return anyhow::result<void>{};  //
}

auto async_main() -> boost::asio::awaitable<void> {
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

    boost::asio::co_spawn(context, async_main(), boost::asio::detached);

    context.run();
}
