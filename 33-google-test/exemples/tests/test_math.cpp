/* ============================================================================
   Section 33.2.1 : TEST — Tests simples
   Description : Tests unitaires des fonctions mathématiques avec TEST
   Fichier source : 02.1-test-simple.md
   ============================================================================ */

#include <gtest/gtest.h>
#include "mp/math.hpp"

TEST(Add, ReturnsSumOfTwoPositiveIntegers) { EXPECT_EQ(mp::add(2, 3), 5); }
TEST(Add, HandlesNegativeNumbers) { EXPECT_EQ(mp::add(-1, -1), -2); EXPECT_EQ(mp::add(-5, 3), -2); }
TEST(Add, HandlesZero) { EXPECT_EQ(mp::add(0, 0), 0); EXPECT_EQ(mp::add(42, 0), 42); }

TEST(Divide, ReturnsIntegerQuotient) { EXPECT_EQ(mp::divide(10, 3), 3); }
TEST(Divide, ThrowsOnZeroDivisor) { EXPECT_THROW(mp::divide(10, 0), std::invalid_argument); }
TEST(Divide, HandlesNegativeDividend) { EXPECT_EQ(mp::divide(-10, 3), -3); }

TEST(Factorial, ReturnsOneForZero) { EXPECT_EQ(mp::factorial(0), 1); }
TEST(Factorial, ComputesCorrectly) { EXPECT_EQ(mp::factorial(5), 120); }
TEST(Factorial, ThrowsOnNegativeInput) { EXPECT_THROW(mp::factorial(-1), std::invalid_argument); }

TEST(Average, ComputesMean) { EXPECT_DOUBLE_EQ(mp::average({1.0, 2.0, 3.0}), 2.0); }
TEST(Average, ThrowsOnEmptyVector) { EXPECT_THROW(mp::average({}), std::invalid_argument); }
