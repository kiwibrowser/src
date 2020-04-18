// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_EARL_GREY_CHROME_EARL_GREY_UI_H_
#define IOS_CHROME_TEST_EARL_GREY_CHROME_EARL_GREY_UI_H_

#import <EarlGrey/EarlGrey.h>
#import <Foundation/Foundation.h>

// Test methods that perform actions on Chrome. These methods only affect Chrome
// using the UI with Earl Grey.
@interface ChromeEarlGreyUI : NSObject

// Makes the toolbar visible by swiping downward, if necessary. Then taps on
// the Tools menu button. At least one tab needs to be open and visible when
// calling this method.
+ (void)openToolsMenu;

// Opens the settings menu by opening the tools menu, and then tapping the
// Settings button. There will be a GREYAssert if the tools menu is open when
// calling this method.
+ (void)openSettingsMenu;

// Scrolls to find the button in the Tools menu with the corresponding
// |buttonMatcher|, and then taps it. If |buttonMatcher| is not found, or
// the Tools menu is not open when this is called there will be a GREYAssert.
+ (void)tapToolsMenuButton:(id<GREYMatcher>)buttonMatcher;

// Scrolls to find the button in the Settings menu with the corresponding
// |buttonMatcher|, and then taps it. If |buttonMatcher| is not found, or
// the Settings menu is not open when this is called there will be a GREYAssert.
+ (void)tapSettingsMenuButton:(id<GREYMatcher>)buttonMatcher;

// Scrolls to find the button in the Privacy menu with the corresponding
// |buttonMatcher|, and then taps it. If |buttonMatcher| is not found, or
// the Privacy menu is not open when this is called there will be a GREYAssert.
+ (void)tapPrivacyMenuButton:(id<GREYMatcher>)buttonMatcher;

// Scrolls to find the button in the Clear Browsing Data menu with the
// corresponding |buttonMatcher|, and then taps it. If |buttonMatcher| is
// not found, or the Clear Browsing Data menu is not open when this is called
// there will be a GREYAssert.
+ (void)tapClearBrowsingDataMenuButton:(id<GREYMatcher>)buttonMatcher;

// Scrolls to find the button in the accounts menu with the corresponding
// |buttonMatcher|, and then taps it. If |buttonMatcher| is not found, or the
// accounts menu is not open when this is called there will be a GREYAssert.
+ (void)tapAccountsMenuButton:(id<GREYMatcher>)buttonMatcher;

// Open a new tab via the tools menu.
+ (void)openNewTab;

// Open a new incognito tab via the tools menu.
+ (void)openNewIncognitoTab;

// Reloads the page via the reload button, and does not wait for the page to
// finish loading.
+ (void)reload;

// Opens the share menu via the share button.
// This method requires that there is at least one tab open.
+ (void)openShareMenu;

// Waits for toolbar to become visible if |isVisible| is YES, otherwise waits
// for it to disappear. If the condition is not met within a timeout, a
// GREYAssert is induced.
+ (void)waitForToolbarVisible:(BOOL)isVisible;

// Signs in the identity for the specific |userEmail|. This must be called from
// the NTP, and it doesn't dismiss the sign in confirmation page.
+ (void)signInToIdentityByEmail:(NSString*)userEmail;

// Confirms the sign in confirmation page, scrolls first to make the OK button
// visible on short devices (e.g. iPhone 5s).
+ (void)confirmSigninConfirmationDialog;

@end

#endif  // IOS_CHROME_TEST_EARL_GREY_CHROME_EARL_GREY_UI_H_
