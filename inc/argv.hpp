#pragma once

#include <string_view>
#include <vector>

namespace john {

auto make_argv(std::string_view) -> std::vector<std::string>;

}
