// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "gpu/config/gpu_control_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

namespace {

constexpr auto kNumerical = GpuControlList::kVersionStyleNumerical;
constexpr auto kLexical = GpuControlList::kVersionStyleLexical;

constexpr auto kBetween = GpuControlList::kBetween;
constexpr auto kEQ = GpuControlList::kEQ;
constexpr auto kLT = GpuControlList::kLT;
constexpr auto kLE = GpuControlList::kLE;
constexpr auto kGT = GpuControlList::kGT;
constexpr auto kGE = GpuControlList::kGE;
constexpr auto kAny = GpuControlList::kAny;

}  // namespace anonymous

class VersionTest : public testing::Test {
 public:
  VersionTest() = default;
  ~VersionTest() override = default;

  typedef GpuControlList::Version Version;
};

TEST_F(VersionTest, VersionComparison) {
  {
    Version info = {kAny, kNumerical, nullptr, nullptr};
    EXPECT_TRUE(info.Contains("0"));
    EXPECT_TRUE(info.Contains("8.9"));
    EXPECT_TRUE(info.Contains("100"));
    EXPECT_TRUE(info.Contains("1.9.alpha"));
  }
  {
    Version info = {kGT, kNumerical, "8.9", nullptr};
    EXPECT_FALSE(info.Contains("7"));
    EXPECT_FALSE(info.Contains("8.9"));
    EXPECT_FALSE(info.Contains("8.9.hs762"));
    EXPECT_FALSE(info.Contains("8.9.1"));
    EXPECT_TRUE(info.Contains("9"));
    EXPECT_TRUE(info.Contains("9.hs762"));
  }
  {
    Version info = {kGE, kNumerical, "8.9", nullptr};
    EXPECT_FALSE(info.Contains("7"));
    EXPECT_FALSE(info.Contains("7.07hdy"));
    EXPECT_TRUE(info.Contains("8.9"));
    EXPECT_TRUE(info.Contains("8.9.1"));
    EXPECT_TRUE(info.Contains("8.9.1beta0"));
    EXPECT_TRUE(info.Contains("9"));
    EXPECT_TRUE(info.Contains("9.0rel"));
  }
  {
    Version info = {kEQ, kNumerical, "8.9", nullptr};
    EXPECT_FALSE(info.Contains("7"));
    EXPECT_TRUE(info.Contains("8"));
    EXPECT_TRUE(info.Contains("8.1uhdy"));
    EXPECT_TRUE(info.Contains("8.9"));
    EXPECT_TRUE(info.Contains("8.9.8alp9"));
    EXPECT_TRUE(info.Contains("8.9.1"));
    EXPECT_FALSE(info.Contains("9"));
  }
  {
    Version info = {kLT, kNumerical, "8.9", nullptr};
    EXPECT_TRUE(info.Contains("7"));
    EXPECT_TRUE(info.Contains("7.txt"));
    EXPECT_TRUE(info.Contains("8.8"));
    EXPECT_TRUE(info.Contains("8.8.test"));
    EXPECT_FALSE(info.Contains("8"));
    EXPECT_FALSE(info.Contains("8.9"));
    EXPECT_FALSE(info.Contains("8.9.1"));
    EXPECT_FALSE(info.Contains("8.9.duck"));
    EXPECT_FALSE(info.Contains("9"));
  }
  {
    Version info = {kLE, kNumerical, "8.9", nullptr};
    EXPECT_TRUE(info.Contains("7"));
    EXPECT_TRUE(info.Contains("8.8"));
    EXPECT_TRUE(info.Contains("8"));
    EXPECT_TRUE(info.Contains("8.9"));
    EXPECT_TRUE(info.Contains("8.9.chicken"));
    EXPECT_TRUE(info.Contains("8.9.1"));
    EXPECT_FALSE(info.Contains("9"));
    EXPECT_FALSE(info.Contains("9.pork"));
  }
  {
    Version info = {kBetween, kNumerical, "8.9", "9.1"};
    EXPECT_FALSE(info.Contains("7"));
    EXPECT_FALSE(info.Contains("8.8"));
    EXPECT_TRUE(info.Contains("8"));
    EXPECT_TRUE(info.Contains("8.9"));
    EXPECT_TRUE(info.Contains("8.9.1"));
    EXPECT_TRUE(info.Contains("9"));
    EXPECT_TRUE(info.Contains("9.1"));
    EXPECT_TRUE(info.Contains("9.1.9"));
    EXPECT_FALSE(info.Contains("9.2"));
    EXPECT_FALSE(info.Contains("10"));
  }
}

TEST_F(VersionTest, DateComparison) {
  // When we use '-' as splitter, we assume a format of mm-dd-yyyy
  // or mm-yyyy, i.e., a date.
  {
    Version info = {kEQ, kNumerical, "1976.3.21", nullptr};
    EXPECT_TRUE(info.Contains("3-21-1976", '-'));
    EXPECT_TRUE(info.Contains("3-1976", '-'));
    EXPECT_TRUE(info.Contains("03-1976", '-'));
    EXPECT_FALSE(info.Contains("21-3-1976", '-'));
  }
  {
    Version info = {kGT, kNumerical, "1976.3.21", nullptr};
    EXPECT_TRUE(info.Contains("3-22-1976", '-'));
    EXPECT_TRUE(info.Contains("4-1976", '-'));
    EXPECT_TRUE(info.Contains("04-1976", '-'));
    EXPECT_FALSE(info.Contains("3-1976", '-'));
    EXPECT_FALSE(info.Contains("2-1976", '-'));
  }
  {
    Version info = {kBetween, kNumerical, "1976.3.21", "2012.12.25"};
    EXPECT_FALSE(info.Contains("3-20-1976", '-'));
    EXPECT_TRUE(info.Contains("3-21-1976", '-'));
    EXPECT_TRUE(info.Contains("3-22-1976", '-'));
    EXPECT_TRUE(info.Contains("3-1976", '-'));
    EXPECT_TRUE(info.Contains("4-1976", '-'));
    EXPECT_TRUE(info.Contains("1-1-2000", '-'));
    EXPECT_TRUE(info.Contains("1-2000", '-'));
    EXPECT_TRUE(info.Contains("2000", '-'));
    EXPECT_TRUE(info.Contains("11-2012", '-'));
    EXPECT_TRUE(info.Contains("12-2012", '-'));
    EXPECT_TRUE(info.Contains("12-24-2012", '-'));
    EXPECT_TRUE(info.Contains("12-25-2012", '-'));
    EXPECT_FALSE(info.Contains("12-26-2012", '-'));
    EXPECT_FALSE(info.Contains("1-2013", '-'));
    EXPECT_FALSE(info.Contains("2013", '-'));
  }
}

TEST_F(VersionTest, LexicalComparison) {
  // When we use lexical style, we assume a format major.minor.*.
  // We apply numerical comparison to major, lexical comparison to others.
  {
    Version info = {kLT, kLexical, "8.201", nullptr};
    EXPECT_TRUE(info.Contains("8.001.100"));
    EXPECT_TRUE(info.Contains("8.109"));
    EXPECT_TRUE(info.Contains("8.10900"));
    EXPECT_TRUE(info.Contains("8.109.100"));
    EXPECT_TRUE(info.Contains("8.2"));
    EXPECT_TRUE(info.Contains("8.20"));
    EXPECT_TRUE(info.Contains("8.200"));
    EXPECT_TRUE(info.Contains("8.20.100"));
    EXPECT_FALSE(info.Contains("8.201"));
    EXPECT_FALSE(info.Contains("8.2010"));
    EXPECT_FALSE(info.Contains("8.21"));
    EXPECT_FALSE(info.Contains("8.21.100"));
    EXPECT_FALSE(info.Contains("9.002"));
    EXPECT_FALSE(info.Contains("9.201"));
    EXPECT_FALSE(info.Contains("12"));
    EXPECT_FALSE(info.Contains("12.201"));
  }
  {
    Version info = {kLT, kLexical, "9.002", nullptr};
    EXPECT_TRUE(info.Contains("8.001.100"));
    EXPECT_TRUE(info.Contains("8.109"));
    EXPECT_TRUE(info.Contains("8.10900"));
    EXPECT_TRUE(info.Contains("8.109.100"));
    EXPECT_TRUE(info.Contains("8.2"));
    EXPECT_TRUE(info.Contains("8.20"));
    EXPECT_TRUE(info.Contains("8.200"));
    EXPECT_TRUE(info.Contains("8.20.100"));
    EXPECT_TRUE(info.Contains("8.201"));
    EXPECT_TRUE(info.Contains("8.2010"));
    EXPECT_TRUE(info.Contains("8.21"));
    EXPECT_TRUE(info.Contains("8.21.100"));
    EXPECT_FALSE(info.Contains("9.002"));
    EXPECT_FALSE(info.Contains("9.201"));
    EXPECT_FALSE(info.Contains("12"));
    EXPECT_FALSE(info.Contains("12.201"));
  }
}

}  // namespace gpu
