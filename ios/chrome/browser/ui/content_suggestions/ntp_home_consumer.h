// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_NTP_HOME_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_NTP_HOME_CONSUMER_H_

#import <Foundation/Foundation.h>

@protocol LogoVendor;

// Handles NTP Home update notifications.
@protocol NTPHomeConsumer<NSObject>

// Whether the Google logo or doodle is being shown.
- (void)setLogoIsShowing:(BOOL)logoIsShowing;

// Exposes view and methods to drive the doodle.
- (void)setLogoVendor:(id<LogoVendor>)logoVendor;

// The number of tabs to show in the NTP fake toolbar.
- (void)setTabCount:(int)tabCount;

// |YES| if the NTP toolbar can show the forward arrow.
- (void)setCanGoForward:(BOOL)canGoForward;

// |YES| if the NTP toolbar can show the back arrow.
- (void)setCanGoBack:(BOOL)canGoBack;

// The location bar has lost focus.
- (void)locationBarResignsFirstResponder;

// Tell location bar has taken focus.
- (void)locationBarBecomesFirstResponder;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_NTP_HOME_CONSUMER_H_
