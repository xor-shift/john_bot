#include <things/logger.hpp>

namespace asio = boost::asio;
using anyhow::result;
using asio::awaitable;

namespace john::things {

auto logger::worker(john::bot& bot) -> awaitable<result<void>> {
    co_return result<void>{};  //
}

auto logger::handle(john::message const& msg) -> awaitable<result<void>> {
    static std::mutex logger_mutex{};  // should be fine
    const auto _ = std::unique_lock{logger_mutex};

    if (msg.m_reply_serial) {
        spdlog::debug("logger received message with serial #{} (in reply to #{}) from \"{}\" addressed to \"{}\"", msg.m_serial, *msg.m_reply_serial, msg.m_from, msg.m_to);
    } else {
        spdlog::debug("logger received message with serial #{} (not in reply to anything) from \"{}\" addressed to \"{}\"", msg.m_serial, msg.m_from, msg.m_to);
    }

    spdlog::debug("payload:");
    const auto visitor = stf::multi_visitor{
      [](payloads::incoming_message const& msg) {
          spdlog::debug("payloads::incoming_message");
          spdlog::debug("  thing_id: {}", msg.m_thing_id);
          spdlog::debug("  sender_identifier: {}", msg.m_sender_identifier.serialize());
          spdlog::debug("  return_to_sender: {}", msg.m_return_to_sender.serialize());
          spdlog::debug("  content: {}", msg.m_content);
      },
      [](payloads::command const& cmd) {
          spdlog::debug("payloads::incoming_message");
          spdlog::debug("  thing_id: {}", cmd.m_thing_id);
          spdlog::debug("  sender_identifier: {}", cmd.m_sender_identifier.serialize());
          spdlog::debug("  return_to_sender: {}", cmd.m_return_to_sender.serialize());
          if (cmd.m_argv.empty()) {
              spdlog::debug("  argv is empty");
          } else {
              spdlog::debug("  argv:");
          }

          for (auto i = 0uz; auto const& arg : cmd.m_argv) {
              spdlog::debug("    #{}: {}", i++, arg);
          }
      },
      [](auto const&) { spdlog::debug("no logger is provided for the payload, sad!"); },
    };

    std::visit(visitor, msg.m_payload);

    co_return result<void>{};  //
}

}  // namespace john::things
