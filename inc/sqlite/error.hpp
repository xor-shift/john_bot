#pragma once

#include <error.hpp>

namespace sqlite {

struct error final : john::error {
    // can't have this be an aggregate as there's a virtual base anyway
    error(int code, std::string description);

    ~error() override = default;

    constexpr auto description() const -> std::string_view override { return m_formatted_desc; }

private:
    int m_code;
    std::string m_description;

    std::string m_formatted_desc;
};

}
