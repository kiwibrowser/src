// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_UPDATE_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_UPDATE_SCREEN_H_

#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/update_screen.h"
#include "chrome/browser/chromeos/login/screens/update_view.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class MockUpdateScreen : public UpdateScreen {
 public:
  MockUpdateScreen(BaseScreenDelegate* base_screen_delegate, UpdateView* view);
  virtual ~MockUpdateScreen();

  MOCK_METHOD0(StartNetworkCheck, void());
};

class MockUpdateView : public UpdateView {
 public:
  MockUpdateView();
  virtual ~MockUpdateView();

  void Bind(UpdateScreen* screen) override;
  void Unbind() override;

  MOCK_METHOD0(Show, void());
  MOCK_METHOD0(Hide, void());
  MOCK_METHOD1(MockBind, void(UpdateScreen* screen));
  MOCK_METHOD0(MockUnbind, void());

 private:
  UpdateScreen* screen_ = nullptr;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_UPDATE_SCREEN_H_
