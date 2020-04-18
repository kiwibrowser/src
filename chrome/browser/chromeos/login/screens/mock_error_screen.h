// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_ERROR_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_ERROR_SCREEN_H_

#include "chrome/browser/chromeos/login/screens/error_screen.h"
#include "chrome/browser/chromeos/login/screens/network_error.h"
#include "chrome/browser/chromeos/login/screens/network_error_view.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class MockErrorScreen : public ErrorScreen {
 public:
  MockErrorScreen(BaseScreenDelegate* base_screen_delegate,
                  NetworkErrorView* view);
  ~MockErrorScreen() override;

  void FixCaptivePortal() override;
  void SetUIState(NetworkError::UIState ui_state) override;
  void SetErrorState(NetworkError::ErrorState error_state,
                     const std::string& network) override;

  MOCK_METHOD0(MockFixCaptivePortal, void());
  MOCK_METHOD1(MockSetUIState, void(NetworkError::UIState ui_state));
  MOCK_METHOD2(MockSetErrorState,
               void(NetworkError::ErrorState error_state,
                    const std::string& network));
};

class MockNetworkErrorView : public NetworkErrorView {
 public:
  MockNetworkErrorView();
  virtual ~MockNetworkErrorView();

  void Bind(ErrorScreen* screen) override;
  void Unbind() override;

  MOCK_METHOD0(Show, void());
  MOCK_METHOD0(Hide, void());
  MOCK_METHOD1(MockBind, void(ErrorScreen* screen));
  MOCK_METHOD0(MockUnbind, void());
  MOCK_METHOD1(ShowOobeScreen, void(OobeScreen screen));

 private:
  ErrorScreen* screen_ = nullptr;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_ERROR_SCREEN_H_
