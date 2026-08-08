#include <memorial/graph.hpp>

#include <gtest/gtest.h>

TEST(Version, ReportsProjectVersion) { EXPECT_EQ(memorial::version(), "0.1.0"); }
