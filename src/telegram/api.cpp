#include <telegram/api.hpp>

#include <try.hpp>

#include <spdlog/spdlog.h>
#include <boost/json.hpp>

#include <span>

namespace asio = boost::asio;
namespace json = boost::json;

using asio::awaitable;

namespace john::telegram {

namespace detail {

template<typename T>
struct field_names;

template<>
struct field_names<api::types::user> {
    inline static constexpr const char* names[] { "id", "is_bot", "" };
};

}

}  // namespace john::telegram

