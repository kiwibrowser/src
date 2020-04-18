// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/mock_demo_setup_screen.h"

using ::testing::AtLeast;
using ::testing::NotNull;

namespace chromeos {

MockDemoSetupScreen::MockDemoSetupScreen(
    BaseScreenDelegate* base_screen_delegate,
    DemoSetupScreenView* view)
    : DemoSetupScreen(base_screen_delegate, view) {}

MockDemoSetupScreen::~MockDemoSetupScreen() = default;

MockDemoSetupScreenView::MockDemoSetupScreenView() {
  EXPECT_CALL(*this, MockBind(NotNull())).Times(AtLeast(1));
}

MockDemoSetupScreenView::~MockDemoSetupScreenView() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void MockDemoSetupScreenView::Bind(DemoSetupScreen* screen) {
  screen_ = screen;
  MockBind(screen);
}

}  // namespace chromeos
