#include <irc/message.hpp>

#include <gtest/gtest.h>

using john::irc::message_view;

void assert_messages_equal(message_view got, message_view expected) {
    ASSERT_EQ(got.m_prefix_name, expected.m_prefix_name);
    ASSERT_EQ(got.m_prefix_user, expected.m_prefix_user);
    ASSERT_EQ(got.m_prefix_host, expected.m_prefix_host);

    ASSERT_EQ(got.m_command, expected.m_command);
    ASSERT_EQ(got.m_trailing, expected.m_trailing);

    auto got_params_vec = std::vector(std::from_range, got.params());
    auto expected_params_vec = std::vector(std::from_range, expected.params());

    std::ranges::copy(expected_params_vec, std::ostream_iterator<std::string_view>(std::cout, ", "));
    std::cout << '\n';

    ASSERT_EQ(got_params_vec, expected_params_vec);
}

void assert_valid_message(std::string_view chars, message_view expected) {
    if (const auto message = message_view::from_chars(chars); !message) {
        ASSERT_TRUE(message) << message.error().description();
    } else {
        assert_messages_equal(*message, std::move(expected));
    }
}

TEST(message, message) {
    using john::irc::numeric_reply;
    using john::irc::reply;

    assert_valid_message(
      "NICK amy\r\n",
      {
        .m_command = "NICK",
        .m_params = "amy",
      }
    );

    assert_valid_message(
      "USER amy * * :Amy Pond\r\n",
      {
        .m_command = "USER",
        .m_params = "amy  *    *",
        .m_trailing = "Amy Pond",
      }
    );

    assert_valid_message(
      ":bar.example.com 001 amy :Welcome to the blah blah\r\n",
      {
        .m_prefix_name = "bar.example.com",
        .m_command = 1,
        .m_params = "  amy ",
        .m_trailing = "Welcome to the blah blah",
      }
    );

    assert_valid_message(
      ":bar.example.com 433 * amy :Nickname is already in use.\r\n",
      {
        .m_prefix_name = "bar.example.com",
        .m_command = numeric_reply::ERR_NICKNAMEINUSE,
        .m_params = "*  amy ",
        .m_trailing = "Nickname is already in use.",
      }
    );

    assert_valid_message(
      "PRIVMSG rory :Hey Rory...\r\n",
      {
        .m_command = "PRIVMSG",
        .m_params = "rory",
        .m_trailing = "Hey Rory...",
      }
    );

    assert_valid_message(
      ":amy!amy@foo.example.com PRIVMSG rory :Hey Rory...\r\n",
      {
        .m_prefix_name = "amy",
        .m_prefix_user = "amy",
        .m_prefix_host = "foo.example.com",
        .m_command = "PRIVMSG",
        .m_params = "rory",
        .m_trailing = "Hey Rory...",
      }
    );

    /*

    assert_valid_message(
      "\r\n",
      {
        .m_original_message = "",
      }
    );
    */
}
