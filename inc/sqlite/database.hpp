#pragma once

#include <sqlite/error.hpp>

#include <sqlite3.h>

#include <memory>

namespace sqlite {

using database = std::shared_ptr<sqlite3>;

extern auto open(const char* filename) -> std::expected<database, error>;

}

