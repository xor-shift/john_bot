#pragma once

#include <bot.hpp>

namespace john::things {

struct relay final : thing {
    ~relay() override = default;

    auto get_id() const -> std::string_view override { return "relay"; }

    auto worker(john::bot& bot) -> boost::asio::awaitable<anyhow::result<void>> override;

    auto handle(john::message const& msg) -> boost::asio::awaitable<anyhow::result<void>> override;

private:
    john::bot* m_bot = nullptr;
};

}  // namespace john::things
