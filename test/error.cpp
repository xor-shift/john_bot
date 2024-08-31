#include <error.hpp>

#include <gtest/gtest.h>

struct mock_error final : john::error {
    ~mock_error() override = default;

    auto source() & -> john::error* override { return nullptr; }
    auto source() const& -> const john::error* override { return nullptr; }

    auto source() && -> std::shared_ptr<john::error> override { return nullptr; }

    auto description() const -> std::string_view override { return ""; }
};

TEST(error, no_error_code) {
    auto err_0 = anyhow::error(anyhow::string_error("asd"));
    auto err_1 = anyhow::error(mock_error{});
}

#include <boost/system/error_code.hpp>

TEST(error, error_code) {
    auto err_0 = anyhow::error(boost::system::error_code{});  //
}
