/* ============================================================================
   Section 33.3 : Assertions et matchers
   Description : Démonstration des assertions GTest et matchers GMock
   Fichier source : 03-assertions-matchers.md
   ============================================================================ */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <string>

TEST(Assertions, Equality) { EXPECT_EQ(42, 42); EXPECT_NE(42, 0); }
TEST(Assertions, Ordering) { EXPECT_LT(50, 100); EXPECT_LE(3, 3); EXPECT_GT(10, 0); }
TEST(Assertions, FloatingPoint) { EXPECT_NEAR(0.1 + 0.2, 0.3, 1e-10); }
TEST(Assertions, Strings) { EXPECT_STREQ("hello", "hello"); EXPECT_STRNE("a", "b"); }
TEST(Assertions, Exceptions) {
    EXPECT_THROW(throw std::runtime_error("oops"), std::runtime_error);
    EXPECT_NO_THROW(int x = 1 + 1; (void)x);
}

using namespace ::testing;
TEST(Matchers, Scalars) { EXPECT_THAT(5, AllOf(Gt(0), Lt(10))); }
TEST(Matchers, Strings) {
    std::string msg = "[ERROR] Connection timeout at 10:45:23";
    EXPECT_THAT(msg, HasSubstr("ERROR"));
    EXPECT_THAT(msg, StartsWith("[ERROR]"));
    EXPECT_THAT(msg, ContainsRegex("[0-9]{2}:[0-9]{2}:[0-9]{2}"));
}
TEST(Matchers, Containers) {
    std::vector<int> v = {1, 2, 3, 4, 5};
    EXPECT_THAT(v, Contains(3));
    EXPECT_THAT(v, Each(Gt(0)));
    EXPECT_THAT(v, SizeIs(5));
    EXPECT_THAT(v, ElementsAre(1, 2, 3, 4, 5));
}
