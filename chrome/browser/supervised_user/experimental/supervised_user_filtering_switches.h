// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_EXPERIMENTAL_SUPERVISED_USER_FILTERING_SWITCHES_H_
#define CHROME_BROWSER_SUPERVISED_USER_EXPERIMENTAL_SUPERVISED_USER_FILTERING_SWITCHES_H_

class Profile;

namespace supervised_users {

// These functions are wrappers around switches::kSupervisedUserSafeSites that
// evaluate a field trial if no command line arguments are specified.
bool IsSafeSitesBlacklistEnabled(const Profile* profile);
bool IsSafeSitesOnlineCheckEnabled(const Profile* profile);

}  // namespace supervised_users

#endif  // CHROME_BROWSER_SUPERVISED_USER_EXPERIMENTAL_SUPERVISED_USER_FILTERING_SWITCHES_H_
