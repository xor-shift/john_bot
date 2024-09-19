#pragma once

#include <bot.hpp>

namespace john::things {

struct dummy final : thing {
    ~dummy() override = default;

    auto get_id() const -> std::string_view override { return "dummy"; }

    auto worker(john::bot& bot) -> boost::asio::awaitable<anyhow::result<void>> override { co_return anyhow::result<void>{}; }

    auto handle(john::message const& msg) -> boost::asio::awaitable<anyhow::result<void>> override { co_return anyhow::result<void>{}; }
};

}  // namespace john::things
