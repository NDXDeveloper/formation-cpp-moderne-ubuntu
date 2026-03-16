/* ============================================================================
   Section 26.3.2 : FetchContent
   Description : Tests unitaires avec GoogleTest téléchargé via FetchContent
   Fichier source : 03.2-fetchcontent.md
   ============================================================================ */

#include <gtest/gtest.h>
#include <string>

TEST(BasicTest, Addition) {
    EXPECT_EQ(2 + 2, 4);
}

TEST(BasicTest, StringSize) {
    std::string hello = "hello";
    EXPECT_EQ(hello.size(), 5);
}

TEST(BasicTest, StringConcat) {
    std::string result = std::string("CMake") + " rocks";
    EXPECT_EQ(result, "CMake rocks");
}
