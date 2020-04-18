// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/applescript/bookmark_applescript_utils_test.h"
#import "chrome/browser/ui/cocoa/applescript/bookmark_item_applescript.h"
#import "chrome/browser/ui/cocoa/applescript/error_applescript.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

using BookmarkItemAppleScriptTest = BookmarkAppleScriptTest;

namespace {

// Set and get title.
IN_PROC_BROWSER_TEST_F(BookmarkItemAppleScriptTest, GetAndSetTitle) {
  NSArray* bookmarkItems = [bookmarkBar_.get() bookmarkItems];
  BookmarkItemAppleScript* item1 = [bookmarkItems objectAtIndex:0];
  [item1 setTitle:@"Foo"];
  EXPECT_NSEQ(@"Foo", [item1 title]);
}

// Set and get URL.
IN_PROC_BROWSER_TEST_F(BookmarkItemAppleScriptTest, GetAndSetURL) {
  NSArray* bookmarkItems = [bookmarkBar_.get() bookmarkItems];
  BookmarkItemAppleScript* item1 = [bookmarkItems objectAtIndex:0];
  [item1 setURL:@"http://foo-bar.org"];
  EXPECT_EQ(GURL("http://foo-bar.org"),
            GURL(base::SysNSStringToUTF8([item1 URL])));

  // If scripter enters invalid URL.
  base::scoped_nsobject<FakeScriptCommand> fakeScriptCommand(
      [[FakeScriptCommand alloc] init]);
  [item1 setURL:@"invalid-url.org"];
  EXPECT_EQ((int)AppleScript::errInvalidURL,
            [fakeScriptCommand.get() scriptErrorNumber]);
}

// Tests setting  a Javascript URL when the "Allow Javascript in Apple
// Events" feature is enabled. An error should be returned if preference
// value is set to false.
IN_PROC_BROWSER_TEST_F(BookmarkItemAppleScriptTest,
                       GetAndSetJavascriptURLFeatureEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      features::kAppleScriptExecuteJavaScriptMenuItem);

  PrefService* prefs = profile()->GetPrefs();
  prefs->SetBoolean(prefs::kAllowJavascriptAppleEvents, false);
  NSArray* bookmarkItems = [bookmarkBar_.get() bookmarkItems];
  BookmarkItemAppleScript* item1 = [bookmarkItems objectAtIndex:0];

  base::scoped_nsobject<FakeScriptCommand> fakeScriptCommand(
      [[FakeScriptCommand alloc] init]);
  [item1 setURL:@"javascript:alert('hi');"];
  EXPECT_EQ(AppleScript::ErrorCode::errJavaScriptUnsupported,
            [fakeScriptCommand.get() scriptErrorNumber]);

  prefs->SetBoolean(prefs::kAllowJavascriptAppleEvents, true);
  [item1 setURL:@"javascript:alert('hi');"];
  EXPECT_EQ(GURL("javascript:alert('hi');"),
            GURL(base::SysNSStringToUTF8([item1 URL])));
}

// Tests setting a Javascript URL when the "Allow Javascript in Apple
// Events" feature is enabled. Doing this should should succeed regardless
// of the preference value.
IN_PROC_BROWSER_TEST_F(BookmarkItemAppleScriptTest,
                       GetAndSetJavascriptURLFeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      features::kAppleScriptExecuteJavaScriptMenuItem);

  PrefService* prefs = profile()->GetPrefs();
  prefs->SetBoolean(prefs::kAllowJavascriptAppleEvents, false);
  NSArray* bookmarkItems = [bookmarkBar_.get() bookmarkItems];
  BookmarkItemAppleScript* item1 = [bookmarkItems objectAtIndex:0];

  base::scoped_nsobject<FakeScriptCommand> fakeScriptCommand(
      [[FakeScriptCommand alloc] init]);
  [item1 setURL:@"javascript:alert('hi');"];
  EXPECT_EQ(GURL("javascript:alert('hi');"),
            GURL(base::SysNSStringToUTF8([item1 URL])));

  prefs->SetBoolean(prefs::kAllowJavascriptAppleEvents, true);
  [item1 setURL:@"javascript:alert('hi');"];
  EXPECT_EQ(GURL("javascript:alert('hi');"),
            GURL(base::SysNSStringToUTF8([item1 URL])));
}

}  // namespace
