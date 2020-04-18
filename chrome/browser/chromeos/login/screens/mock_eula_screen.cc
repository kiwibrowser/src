// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/mock_eula_screen.h"

namespace chromeos {

using ::testing::AtLeast;
using ::testing::_;

MockEulaScreen::MockEulaScreen(BaseScreenDelegate* base_screen_delegate,
                               Delegate* delegate,
                               EulaView* view)
    : EulaScreen(base_screen_delegate, delegate, view) {}

MockEulaScreen::~MockEulaScreen() {}

MockEulaView::MockEulaView() {
  EXPECT_CALL(*this, MockBind(_)).Times(AtLeast(1));
}

MockEulaView::~MockEulaView() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void MockEulaView::Bind(EulaScreen* screen) {
  screen_ = screen;
  MockBind(screen);
}

void MockEulaView::Unbind() {
  screen_ = nullptr;
  MockUnbind();
}

}  // namespace chromeos
