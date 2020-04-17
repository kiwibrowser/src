// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/scoped_pipe.h"

#include <vector>

#include "platform/api/logging.h"
#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace {

using ::testing::ElementsAre;

std::vector<int>* g_freed_values = nullptr;

struct IntTraits {
  using PipeType = int;
  static constexpr int kInvalidValue = -1;

  static void Close(int fd) { g_freed_values->push_back(fd); }
};

constexpr int IntTraits::kInvalidValue;

class ScopedPipeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    OSP_DCHECK(!g_freed_values);
    g_freed_values = new std::vector<int>();
  }

  void TearDown() override {
    delete g_freed_values;
    g_freed_values = nullptr;
  }
};

}  // namespace

TEST_F(ScopedPipeTest, Close) {
  { ScopedPipe<IntTraits> x; }
  ASSERT_TRUE(g_freed_values->empty());

  { ScopedPipe<IntTraits> x(3); }
  EXPECT_THAT(*g_freed_values, ElementsAre(3));
  g_freed_values->clear();

  {
    ScopedPipe<IntTraits> x(3);
    EXPECT_EQ(3, x.release());

    ScopedPipe<IntTraits> y;
    EXPECT_EQ(IntTraits::kInvalidValue, y.release());
  }
  ASSERT_TRUE(g_freed_values->empty());

  {
    ScopedPipe<IntTraits> x(3);
    ScopedPipe<IntTraits> y(std::move(x));
    EXPECT_EQ(IntTraits::kInvalidValue, x.get());
    EXPECT_EQ(3, y.get());
    EXPECT_TRUE(g_freed_values->empty());
  }
  EXPECT_THAT(*g_freed_values, ElementsAre(3));
  g_freed_values->clear();

  {
    ScopedPipe<IntTraits> x(3);
    ScopedPipe<IntTraits> y(4);
    y = std::move(x);
    EXPECT_EQ(IntTraits::kInvalidValue, x.get());
    EXPECT_EQ(3, y.get());
    EXPECT_EQ(1u, g_freed_values->size());
    EXPECT_EQ(4, g_freed_values->front());
  }
  EXPECT_THAT(*g_freed_values, ElementsAre(4, 3));
  g_freed_values->clear();
}

TEST_F(ScopedPipeTest, Comparisons) {
  std::vector<int> g_freed_values;
  ScopedPipe<IntTraits> x;
  ScopedPipe<IntTraits> y;
  EXPECT_FALSE(x);
  EXPECT_EQ(x, y);

  x = ScopedPipe<IntTraits>(3);
  EXPECT_TRUE(x);
  EXPECT_NE(x, y);

  y = ScopedPipe<IntTraits>(4);
  EXPECT_TRUE(y);
  EXPECT_NE(x, y);

  y = ScopedPipe<IntTraits>(3);
  EXPECT_EQ(x, y);
}

}  // namespace openscreen
