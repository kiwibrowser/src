// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_APP_SIGNIN_TEST_UTIL_H_
#define IOS_CHROME_TEST_APP_SIGNIN_TEST_UTIL_H_

namespace chrome_test_util {

// Sets up mock authentication that will bypass the real ChromeIdentityService
// and install a fake one.
void SetUpMockAuthentication();

// Tears down the fake ChromeIdentityService and restores the real one.
void TearDownMockAuthentication();

// Sets up a mock AccountReconcilor that will always succeed and won't use the
// network.
void SetUpMockAccountReconcilor();

// Tears down the mock AccountReconcilor if it was previously set up.
void TearDownMockAccountReconcilor();

// Signs the user out and clears the known accounts entirely. Returns whether
// the accounts were correctly removed from the keychain.
bool SignOutAndClearAccounts();

// Resets mock authentication.
void ResetMockAuthentication();

// Resets Sign-in promo impression preferences for bookmarks and settings view,
// and resets kIosBookmarkPromoAlreadySeen flag for bookmarks.
void ResetSigninPromoPreferences();

}  // namespace chrome_test_util

#endif  // IOS_CHROME_TEST_APP_SIGNIN_TEST_UTIL_H_
