// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_ENABLE_DEBUGGING_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_ENABLE_DEBUGGING_SCREEN_H_

#include "chrome/browser/chromeos/login/screens/enable_debugging_screen.h"
#include "chrome/browser/chromeos/login/screens/enable_debugging_screen_view.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class MockEnableDebuggingScreen : public EnableDebuggingScreen {
 public:
  MockEnableDebuggingScreen(BaseScreenDelegate* base_screen_delegate,
                            EnableDebuggingScreenView* view);
  ~MockEnableDebuggingScreen() override;
};

class MockEnableDebuggingScreenView : public EnableDebuggingScreenView {
 public:
  MockEnableDebuggingScreenView();
  ~MockEnableDebuggingScreenView() override;

  MOCK_METHOD0(Show, void());
  MOCK_METHOD0(Hide, void());
  MOCK_METHOD1(MockSetDelegate, void(Delegate* delegate));

  void SetDelegate(EnableDebuggingScreenView::Delegate* delegate) override;

 private:
  Delegate* delegate_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_ENABLE_DEBUGGING_SCREEN_H_
