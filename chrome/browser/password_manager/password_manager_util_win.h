// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_UTIL_WIN_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_UTIL_WIN_H_

#include "chrome/browser/password_manager/reauth_purpose.h"
#include "ui/gfx/native_widget_types.h"

namespace password_manager_util_win {

// Attempts to (re-)authenticate the user of the OS account. Returns true if
// the user was successfully authenticated, or if authentication was not
// possible.
bool AuthenticateUser(gfx::NativeWindow window,
                      password_manager::ReauthPurpose purpose);

// Query the system to determine whether the current logged on user has a
// password set on their OS account, and log the result with UMA. It should be
// called on UI thread. The query is executed with a time delay to avoid calling
// a few Windows APIs on startup.
void DelayReportOsPassword();

}  // namespace password_manager_util_win

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_UTIL_WIN_H_
