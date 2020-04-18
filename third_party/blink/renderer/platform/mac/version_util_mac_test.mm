// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "third_party/blink/renderer/platform/mac/version_util_mac.h"

#include <AppKit/AppKit.h>
#include <gtest/gtest.h>

#ifndef NSAppKitVersionNumber10_10
#define NSAppKitVersionNumber10_10 1343
#endif

// This number was determined by writing a tiny Cocoa App on 10.10.4.
#define NSAppKitVersionNumber10_10Max 1348

// This number was measured on OSX 10.11 Beta 15A234d. The 10.11
// AppKit.framework does not provide an NSAppKitVersionNumber preprocessor
// definition for OSX 10.11.
#define NSAppKitVersionNumber10_11Max 1389

// AppKit version is loosely correlated to OSX version. It's still useful as a
// sanity check in unit tests, though we don't want to rely on it in production
// code.
TEST(VersionUtilMac, AppKitVersions) {
  if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_10Max &&
      floor(NSAppKitVersionNumber) >= NSAppKitVersionNumber10_10) {
    EXPECT_TRUE(blink::IsOS10_10());
    EXPECT_FALSE(blink::IsOS10_11());
    return;
  }

  if (floor(NSAppKitVersionNumber) == NSAppKitVersionNumber10_11Max) {
    EXPECT_FALSE(blink::IsOS10_10());
    EXPECT_TRUE(blink::IsOS10_11());
    return;
  }
}
