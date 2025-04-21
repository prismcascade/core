// A minimal GoogleTest + RapidCheck smoke test
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

// plain GoogleTest — 確実に 1 つは通る
TEST(Smoke, AlwaysPasses) { EXPECT_EQ(1, 1); }

// property‑based example (rapidcheck)
RC_GTEST_PROP(SmokeProperty, AdditionIsCommutative, (int a, int b)) { RC_ASSERT(a + b == b + a); }
