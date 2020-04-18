// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_DEMO_SETUP_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_DEMO_SETUP_SCREEN_H_

#include "chrome/browser/chromeos/login/screens/demo_setup_screen.h"
#include "chrome/browser/chromeos/login/screens/demo_setup_screen_view.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class MockDemoSetupScreen : public DemoSetupScreen {
 public:
  MockDemoSetupScreen(BaseScreenDelegate* base_screen_delegate,
                      DemoSetupScreenView* view);
  ~MockDemoSetupScreen() override;
};

class MockDemoSetupScreenView : public DemoSetupScreenView {
 public:
  MockDemoSetupScreenView();
  ~MockDemoSetupScreenView() override;

  MOCK_METHOD0(Show, void());
  MOCK_METHOD0(Hide, void());
  MOCK_METHOD1(MockBind, void(DemoSetupScreen* screen));

  void Bind(DemoSetupScreen* screen) override;

 private:
  DemoSetupScreen* screen_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_DEMO_SETUP_SCREEN_H_
