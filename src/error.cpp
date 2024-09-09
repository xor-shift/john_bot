#include <error.hpp>

#include <cpptrace/cpptrace.hpp>

namespace john {

error::error()
    : m_trace(std::move(cpptrace::raw_trace::current().frames)) {}

}  // namespace john

auto fmt::formatter<john::error>::format(john::error const& error, fmt::format_context& ctx) -> fmt::format_context::iterator {
    auto it = ctx.out();

    it = fmt::format_to(it, "error with description: {}\n", error.description());

    // should be pointer interconvertible
    auto const& backtrace = reinterpret_cast<cpptrace::raw_trace const&>(error.backtrace());
    it = fmt::format_to(it, "{}", backtrace.resolve().to_string(true));

    const auto* cause = error.source();
    while (cause != nullptr) {
        it = fmt::format_to(it, "\ncaused by error: {}\n", cause->description());

        auto const& backtrace = reinterpret_cast<cpptrace::raw_trace const&>(cause->backtrace());
        it = fmt::format_to(it, "{}", backtrace.resolve().to_string(true));

        cause = cause->source();
    }

    return it;
}
