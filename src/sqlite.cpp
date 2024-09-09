#include <sqlite/sqlite.hpp>

#include <stuff/scope/scope_guard.hpp>

namespace sqlite {

namespace {

struct sqlite_destroyer {
    void operator()(sqlite3* resource) { sqlite3_close(resource); }
};

}  // namespace

error::error(int code, std::string description)
    : m_code(code)
    , m_description(std::move(description))
    , m_formatted_desc(fmt::format("sqlite error with code {} ({}), description: {}", m_code, sqlite3_errstr(m_code), m_description)) {}

auto open(const char* filename) -> std::expected<database, error> {
    auto* db = (sqlite3*)nullptr;
    auto result = sqlite3_open(filename, &db);

    auto in_case_of_throw = stf::scope_exit{[db] { sqlite3_close(db); }};
    auto resource = database(db, sqlite_destroyer{});
    in_case_of_throw.release();

    if (result != SQLITE_OK) {
        return std::unexpected{error(result, sqlite3_errmsg(db))};
    }

    return std::move(resource);
}

}  // namespace sqlite
