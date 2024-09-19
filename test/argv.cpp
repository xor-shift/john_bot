#include <argv.hpp>

#include <gtest/gtest.h>

TEST(argv, plain) {
    ASSERT_EQ(john::make_argv("a b \t  c"), (std::vector<std::string>{"a", "b", "c"}));
    ASSERT_EQ(john::make_argv("a'b\" \t  c\"d'"), (std::vector<std::string>{"a'b\"", "c\"d'"}));
    ASSERT_EQ(john::make_argv("\\a \\t \\n"), (std::vector<std::string>{"a", "\t", "\n"}));
}

TEST(argv, quotes) {
    ASSERT_EQ(john::make_argv("\"a b c\""), (std::vector<std::string>{"a b c"}));
    ASSERT_EQ(john::make_argv("\"a b c \"d"), (std::vector<std::string>{"a b c d"}));
    ASSERT_EQ(john::make_argv("\"a b c \"d\\ e"), (std::vector<std::string>{"a b c d e"}));
    ASSERT_EQ(john::make_argv("\"a b c \"d\\ e \"a b c \"d\\ e"), (std::vector<std::string>{"a b c d e", "a b c d e"}));
}

TEST(argv, edge_cases) {
    ASSERT_EQ(john::make_argv("\""), (std::vector<std::string>{""}));
    ASSERT_EQ(john::make_argv("\\"), (std::vector<std::string>{""}));
    ASSERT_EQ(john::make_argv(""), (std::vector<std::string>{}));
}
