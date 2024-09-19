#pragma once

#include <bot.hpp>

namespace john::things {

struct logger final : thing {
    ~logger() override = default;

    auto get_id() const -> std::string_view override { return "logger"; }

    auto worker(john::bot& bot) -> boost::asio::awaitable<anyhow::result<void>> override;

    auto handle(john::message const& msg) -> boost::asio::awaitable<anyhow::result<void>> override;
};

}  // namespace john::things
