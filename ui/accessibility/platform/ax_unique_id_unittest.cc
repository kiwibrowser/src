// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/platform/ax_unique_id.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

class AXPlatformUniqueIdTest : public testing::Test {
 public:
  AXPlatformUniqueIdTest() {}
  ~AXPlatformUniqueIdTest() override {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(AXPlatformUniqueIdTest, TestIdsUnique) {
  AXUniqueId id1, id2;
  EXPECT_FALSE(id1 == id2);
  EXPECT_GT(id2.Get(), id1.Get());
}

static const int32_t kMaxId = 100;

class AXTestSmallBankUniqueId : public AXUniqueId {
 public:
  AXTestSmallBankUniqueId();

  friend class AXUniqueId;
};

AXTestSmallBankUniqueId::AXTestSmallBankUniqueId() : AXUniqueId(kMaxId) {}

TEST_F(AXPlatformUniqueIdTest, TestIdsNotReused) {
  // Create a bank of ids that uses up all available ids.
  // Then remove an id and replace with a new one. Since it's the only
  // slot available, the id will end up having the same value, rather than
  // starting over at 1.
  AXTestSmallBankUniqueId* ids[kMaxId];

  for (int i = 0; i < kMaxId; i++) {
    ids[i] = new AXTestSmallBankUniqueId();
  }

  static int kIdToReplace = 10;

  // IDs are 1-based.
  EXPECT_EQ(ids[kIdToReplace]->Get(), kIdToReplace + 1);

  // Delete one of the ids and replace with a new one.
  delete ids[kIdToReplace];
  ids[kIdToReplace] = new AXTestSmallBankUniqueId();

  // IDs are 1-based.
  EXPECT_EQ(ids[kIdToReplace]->Get(), kIdToReplace + 1);

  // Clean up.
  for (int i = 0; i < kMaxId; i++) {
    delete ids[i];
  }
}

}  // namespace ui
