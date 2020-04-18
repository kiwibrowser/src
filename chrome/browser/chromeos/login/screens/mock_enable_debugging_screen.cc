// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/mock_enable_debugging_screen.h"

using ::testing::AtLeast;
using ::testing::NotNull;

namespace chromeos {

MockEnableDebuggingScreen::MockEnableDebuggingScreen(
    BaseScreenDelegate* base_screen_delegate,
    EnableDebuggingScreenView* view)
    : EnableDebuggingScreen(base_screen_delegate, view) {}

MockEnableDebuggingScreen::~MockEnableDebuggingScreen() {}

MockEnableDebuggingScreenView::MockEnableDebuggingScreenView() {
  EXPECT_CALL(*this, MockSetDelegate(NotNull())).Times(AtLeast(1));
}

MockEnableDebuggingScreenView::~MockEnableDebuggingScreenView() {
  if (delegate_)
    delegate_->OnViewDestroyed(this);
}

void MockEnableDebuggingScreenView::SetDelegate(
    EnableDebuggingScreenView::Delegate* delegate) {
  delegate_ = delegate;
  MockSetDelegate(delegate);
}

}  // namespace chromeos
