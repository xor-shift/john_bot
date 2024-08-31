#include <kv.hpp>

#include <gtest/gtest.h>

TEST(kv, kv) {
    auto kv_0 = john::mini_kv{};
    ASSERT_EQ(kv_0.size(), 0uz);
    ASSERT_TRUE(kv_0.empty());

    kv_0.push_back("a", "b");
    ASSERT_EQ(kv_0.size(), 1uz);
    ASSERT_FALSE(kv_0.empty());

    ASSERT_EQ(kv_0["a"], "b");
    ASSERT_EQ(kv_0[0], (std::pair{"a", "b"}));

    kv_0.push_back("Hello,", "world!");
    ASSERT_EQ(kv_0.size(), 2uz);

    ASSERT_EQ(kv_0["Hello,"], "world!");
    ASSERT_EQ(kv_0[0], (std::pair{"a", "b"}));
    ASSERT_EQ(kv_0[1], (std::pair{"Hello,", "world!"}));
}
