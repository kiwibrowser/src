// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_TEST_OOBE_SCREEN_WAITER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_TEST_OOBE_SCREEN_WAITER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"

namespace content {
class MessageLoopRunner;
}

namespace chromeos {

// A waiter that blocks until the expected oobe screen is reached.
class OobeScreenWaiter : public OobeUI::Observer {
 public:
  explicit OobeScreenWaiter(OobeScreen expected_screen);
  ~OobeScreenWaiter() override;

  // Run message loop to wait for the expected_screen to become current screen.
  void Wait();

  // Run message loop to wait for the expected_screen to be fully initialized.
  void WaitForInitialization();

  // Similar to Wait() but does not assert the current screen is
  // expected_screen on exit. Use this when there are multiple screen changes
  // during the wait and the screen to be waited is not the final one.
  void WaitNoAssertCurrentScreen();

  // OobeUI::Observer implementation:
  void OnCurrentScreenChanged(OobeScreen current_screen,
                              OobeScreen new_screen) override;
  void OnScreenInitialized(OobeScreen screen) override;

 private:
  OobeUI* GetOobeUI();

  bool waiting_for_screen_init_ = false;
  bool waiting_for_screen_ = false;
  OobeScreen expected_screen_ = OobeScreen::SCREEN_UNKNOWN;
  scoped_refptr<content::MessageLoopRunner> runner_;

  DISALLOW_COPY_AND_ASSIGN(OobeScreenWaiter);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_TEST_OOBE_SCREEN_WAITER_H_
