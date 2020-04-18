// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/lock_window.h"

#include "ash/login/ui/lock_contents_view.h"
#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_big_user_view.h"
#include "ash/login/ui/login_keyboard_test_base.h"
#include "ash/login/ui/login_test_utils.h"

namespace ash {

using LockWindowVirtualKeyboardTest = LoginKeyboardTestBase;

TEST_F(LockWindowVirtualKeyboardTest, VirtualKeyboardDoesNotCoverAuthView) {
  ASSERT_NO_FATAL_FAILURE(ShowLockScreen());
  LockContentsView* lock_contents =
      LockScreen::TestApi(LockScreen::Get()).contents_view();
  ASSERT_NE(nullptr, lock_contents);

  LoadUsers(1);

  LoginBigUserView* auth_view =
      MakeLockContentsViewTestApi(lock_contents).primary_big_view();
  ASSERT_NE(nullptr, auth_view);

  ASSERT_NO_FATAL_FAILURE(ShowKeyboard());
  EXPECT_FALSE(
      auth_view->GetBoundsInScreen().Intersects(GetKeyboardBoundsInScreen()));
}

}  // namespace ash
