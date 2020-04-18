// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_AVATAR_MENU_ACTIONS_H_
#define CHROME_BROWSER_PROFILES_AVATAR_MENU_ACTIONS_H_

#include "chrome/browser/profiles/profile_metrics.h"

class Browser;
class Profile;

// This interface controls the behavior of avatar menu actions.
// Only implemented by AvatarMenuActionsDesktop, although a Chrome OS version
// used to exist as AvatarMenuActionsChromeOS.
class AvatarMenuActions {
 public:
  virtual ~AvatarMenuActions() {}

  static AvatarMenuActions* Create();

  // Allows the user to create a new profile.
  virtual void AddNewProfile(ProfileMetrics::ProfileAdd type) = 0;

  // Allows the user to edit the profile at the given index in the cache.
  virtual void EditProfile(Profile* profile) = 0;

  // Returns true if the add profile link should be shown.
  virtual bool ShouldShowAddNewProfileLink() const = 0;

  // Returns true if the edit profile link should be shown.
  virtual bool ShouldShowEditProfileLink() const = 0;

  // Updates the browser.
  // TODO: Delegate browser actions to remove dependency on Browser class.
  virtual void ActiveBrowserChanged(Browser* browser) = 0;
};

#endif  // CHROME_BROWSER_PROFILES_AVATAR_MENU_ACTIONS_H_
