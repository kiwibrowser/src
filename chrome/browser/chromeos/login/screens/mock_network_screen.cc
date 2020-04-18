// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/mock_network_screen.h"

namespace chromeos {

using ::testing::AtLeast;
using ::testing::_;

MockNetworkScreen::MockNetworkScreen(BaseScreenDelegate* base_screen_delegate,
                                     Delegate* delegate,
                                     NetworkView* view)
    : NetworkScreen(base_screen_delegate, delegate, view) {}

MockNetworkScreen::~MockNetworkScreen() {}

MockNetworkView::MockNetworkView() {
  EXPECT_CALL(*this, MockBind(_)).Times(AtLeast(1));
}

MockNetworkView::~MockNetworkView() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void MockNetworkView::Bind(NetworkScreen* screen) {
  screen_ = screen;
  MockBind(screen);
}

void MockNetworkView::Unbind() {
  screen_ = nullptr;
  MockUnbind();
}

}  // namespace chromeos
