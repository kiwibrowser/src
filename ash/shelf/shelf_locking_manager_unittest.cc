// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/shelf_locking_manager.h"

#include "ash/shelf/shelf.h"
#include "ash/test/ash_test_base.h"

namespace ash {
namespace {

// Tests the shelf behavior when the screen or session is locked.
class ShelfLockingManagerTest : public AshTestBase {
 public:
  ShelfLockingManagerTest() = default;

  ShelfLockingManager* GetShelfLockingManager() {
    return GetPrimaryShelf()->GetShelfLockingManagerForTesting();
  }

  void SetScreenLocked(bool locked) {
    GetShelfLockingManager()->OnLockStateChanged(locked);
  }

  void SetSessionState(session_manager::SessionState state) {
    GetShelfLockingManager()->OnSessionStateChanged(state);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ShelfLockingManagerTest);
};

// Makes sure shelf alignment is correct for lock screen.
TEST_F(ShelfLockingManagerTest, AlignmentLockedWhileScreenLocked) {
  Shelf* shelf = GetPrimaryShelf();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());

  shelf->SetAlignment(SHELF_ALIGNMENT_LEFT);
  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());

  SetScreenLocked(true);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED, shelf->alignment());
  SetScreenLocked(false);
  EXPECT_EQ(SHELF_ALIGNMENT_LEFT, shelf->alignment());
}

// Makes sure shelf alignment is correct for login and add user screens.
TEST_F(ShelfLockingManagerTest, AlignmentLockedWhileSessionLocked) {
  Shelf* shelf = GetPrimaryShelf();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());

  shelf->SetAlignment(SHELF_ALIGNMENT_RIGHT);
  EXPECT_EQ(SHELF_ALIGNMENT_RIGHT, shelf->alignment());

  SetSessionState(session_manager::SessionState::LOGIN_PRIMARY);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED, shelf->alignment());
  SetSessionState(session_manager::SessionState::ACTIVE);
  EXPECT_EQ(SHELF_ALIGNMENT_RIGHT, shelf->alignment());

  SetSessionState(session_manager::SessionState::LOGIN_SECONDARY);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED, shelf->alignment());
  SetSessionState(session_manager::SessionState::ACTIVE);
  EXPECT_EQ(SHELF_ALIGNMENT_RIGHT, shelf->alignment());
}

// Makes sure shelf alignment changes are stored, not set, while locked.
TEST_F(ShelfLockingManagerTest, AlignmentChangesDeferredWhileLocked) {
  Shelf* shelf = GetPrimaryShelf();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());

  SetScreenLocked(true);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED, shelf->alignment());
  shelf->SetAlignment(SHELF_ALIGNMENT_RIGHT);
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM_LOCKED, shelf->alignment());
  SetScreenLocked(false);
  EXPECT_EQ(SHELF_ALIGNMENT_RIGHT, shelf->alignment());
}

}  // namespace
}  // namespace ash
