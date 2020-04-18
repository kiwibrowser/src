// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREEN_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREEN_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"

namespace chromeos {

class WizardController;

// Class that manages creation and ownership of screens.
class ScreenManager {
 public:
  // |wizard_controller| is not owned by this class.
  explicit ScreenManager(WizardController* wizard_controller);
  ~ScreenManager();

  // Getter for screen with lazy initialization.
  BaseScreen* GetScreen(OobeScreen screen);

  bool HasScreen(OobeScreen screen);

 private:
  FRIEND_TEST_ALL_PREFIXES(EnrollmentScreenTest, TestCancel);
  FRIEND_TEST_ALL_PREFIXES(WizardControllerFlowTest, Accelerators);
  friend class WizardControllerFlowTest;
  friend class WizardControllerOobeResumeTest;
  friend class WizardInProcessBrowserTest;
  friend class WizardControllerBrokenLocalStateTest;

  // Created screens.
  std::map<OobeScreen, std::unique_ptr<BaseScreen>> screens_;

  // Used to allocate BaseScreen instances. Unowned.
  WizardController* wizard_controller_;

  DISALLOW_COPY_AND_ASSIGN(ScreenManager);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREEN_MANAGER_H_
