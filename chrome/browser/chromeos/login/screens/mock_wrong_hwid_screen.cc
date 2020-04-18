// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/mock_wrong_hwid_screen.h"

using testing::AtLeast;
using testing::NotNull;

namespace chromeos {

MockWrongHWIDScreen::MockWrongHWIDScreen(
    BaseScreenDelegate* base_screen_delegate,
    WrongHWIDScreenView* view)
    : WrongHWIDScreen(base_screen_delegate, view) {}

MockWrongHWIDScreen::~MockWrongHWIDScreen() {}

MockWrongHWIDScreenView::MockWrongHWIDScreenView() : delegate_(nullptr) {
  EXPECT_CALL(*this, MockSetDelegate(NotNull())).Times(AtLeast(1));
}

MockWrongHWIDScreenView::~MockWrongHWIDScreenView() {
  if (delegate_)
    delegate_->OnViewDestroyed(this);
}

void MockWrongHWIDScreenView::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
  MockSetDelegate(delegate);
}

}  // namespace chromeos
