// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/endsession_watcher_window_win.h"

#include <windows.h>
#include <stddef.h>
#include <vector>

#include "base/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace browser_watcher {

namespace {

class EndSessionWatcherWindowTest : public testing::Test {
 public:
  EndSessionWatcherWindowTest()
      : num_callbacks_(0), last_message_(0), last_lparam_(0) {
  }

  void OnEndSessionMessage(UINT message, LPARAM lparam) {
    ++num_callbacks_;
    last_message_ = message;
    last_lparam_ = lparam;
  }

  size_t num_callbacks_;
  UINT last_message_;
  LPARAM last_lparam_;
};

}  // namespace browser_watcher

TEST_F(EndSessionWatcherWindowTest, NoCallbackOnDestruction) {
  {
    EndSessionWatcherWindow watcher_window(
        base::Bind(&EndSessionWatcherWindowTest::OnEndSessionMessage,
                   base::Unretained(this)));
  }

  EXPECT_EQ(0u, num_callbacks_);
  EXPECT_EQ(0, last_lparam_);
}

TEST_F(EndSessionWatcherWindowTest, IssuesCallbackOnMessage) {
  EndSessionWatcherWindow watcher_window(
      base::Bind(&EndSessionWatcherWindowTest::OnEndSessionMessage,
                 base::Unretained(this)));

  ::SendMessage(watcher_window.window(), WM_QUERYENDSESSION, TRUE, 0xBEEF);
  EXPECT_EQ(1u, num_callbacks_);
  EXPECT_EQ(static_cast<DWORD>(WM_QUERYENDSESSION), last_message_);
  EXPECT_EQ(0xBEEF, last_lparam_);

  ::SendMessage(watcher_window.window(), WM_ENDSESSION, TRUE, 0xCAFE);
  EXPECT_EQ(2u, num_callbacks_);
  EXPECT_EQ(static_cast<UINT>(WM_ENDSESSION), last_message_);
  EXPECT_EQ(0xCAFE, last_lparam_);

  // Verify that other messages don't pass through.
  ::SendMessage(watcher_window.window(), WM_CLOSE, TRUE, 0xCAFE);
  EXPECT_EQ(2u, num_callbacks_);
}

}  // namespace browser_watcher
