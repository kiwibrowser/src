// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PROFILES_USER_MANAGER_MAC_H_
#define CHROME_BROWSER_UI_COCOA_PROFILES_USER_MANAGER_MAC_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "components/signin/core/browser/signin_metrics.h"

@class DialogWindowController;
@class UserManagerWindowController;

// Dialog widget that contains the Desktop User Manager webui. This object
// should always be created from the UserManager::Show() method. Note that only
// one User Manager will exist at a time.
class UserManagerMac {
 public:
  // Called by the cocoa window controller when its window closes and the
  // controller destroyed itself. Deletes the instance.
  void WindowWasClosed();

  // Called from the UserManager class once the |system_profile| is ready. Will
  // construct a UserManagerMac object and show |url|.
  static void OnSystemProfileCreated(const base::Time& start_time,
                                     Profile* system_profile,
                                     const std::string& url);

  UserManagerWindowController* window_controller() {
    return window_controller_.get();
  }

  void set_user_manager_started_showing(
      const base::Time& user_manager_started_showing) {
    user_manager_started_showing_ = user_manager_started_showing;
  }

  void LogTimeToOpen();

  void ShowDialog(content::BrowserContext* browser_context,
                  const std::string& email,
                  const GURL& url);
  void CloseDialog();

  void DisplayErrorMessage();

  void SetSigninProfilePath(const base::FilePath& profile_path);
  base::FilePath GetSigninProfilePath();

 private:
  explicit UserManagerMac(Profile* profile);
  virtual ~UserManagerMac();

  // Controller of the window.
  base::scoped_nsobject<UserManagerWindowController> window_controller_;

  base::scoped_nsobject<DialogWindowController> reauth_window_;

  base::Time user_manager_started_showing_;

  base::FilePath signin_profile_path_;

  DISALLOW_COPY_AND_ASSIGN(UserManagerMac);
};

#endif  // CHROME_BROWSER_UI_COCOA_PROFILES_USER_MANAGER_MAC_H_
