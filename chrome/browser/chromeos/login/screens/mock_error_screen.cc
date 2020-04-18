// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/mock_error_screen.h"

using ::testing::AtLeast;
using ::testing::_;

namespace chromeos {

MockErrorScreen::MockErrorScreen(BaseScreenDelegate* base_screen_delegate,
                                 NetworkErrorView* view)
    : ErrorScreen(base_screen_delegate, view) {}

MockErrorScreen::~MockErrorScreen() {}

void MockErrorScreen::FixCaptivePortal() {
  ErrorScreen::FixCaptivePortal();
  MockFixCaptivePortal();
}

void MockErrorScreen::SetUIState(NetworkError::UIState ui_state) {
  ErrorScreen::SetUIState(ui_state);
  MockSetUIState(ui_state);
}

void MockErrorScreen::SetErrorState(NetworkError::ErrorState error_state,
                                    const std::string& network) {
  ErrorScreen::SetErrorState(error_state, network);
  MockSetErrorState(error_state, network);
}

MockNetworkErrorView::MockNetworkErrorView() {
  EXPECT_CALL(*this, MockBind(_)).Times(AtLeast(1));
  EXPECT_CALL(*this, MockUnbind()).Times(AtLeast(1));
}

MockNetworkErrorView::~MockNetworkErrorView() {
  if (screen_)
    screen_->OnViewDestroyed(this);
}

void MockNetworkErrorView::Bind(ErrorScreen* screen) {
  screen_ = screen;
  MockBind(screen);
}

void MockNetworkErrorView::Unbind() {
  screen_ = nullptr;
  MockUnbind();
}

}  // namespace chromeos
