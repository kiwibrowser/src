// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_BASE_SCREEN_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_BASE_SCREEN_DELEGATE_H_

#include <string>

#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

// Interface that handles notifications received from any of login wizard
// screens.
class MockBaseScreenDelegate : public BaseScreenDelegate {
 public:
  MockBaseScreenDelegate();
  virtual ~MockBaseScreenDelegate();

  MOCK_METHOD3(OnExit,
               void(BaseScreen&,
                    ScreenExitCode,
                    const ::login::ScreenContext*));
  MOCK_METHOD0(ShowCurrentScreen, void());
  MOCK_METHOD0(GetErrorScreen, ErrorScreen*());
  MOCK_METHOD0(ShowErrorScreen, void());
  MOCK_METHOD1(HideErrorScreen, void(BaseScreen*));
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_BASE_SCREEN_DELEGATE_H_
