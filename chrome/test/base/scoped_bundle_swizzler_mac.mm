// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/scoped_bundle_swizzler_mac.h"

#import <Foundation/Foundation.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_objc_class_swizzler.h"
#include "base/strings/sys_string_conversions.h"
#import "third_party/ocmock/OCMock/OCMock.h"

static NSBundle* g_original_main_bundle = nil;
static id g_swizzled_main_bundle = nil;

// A donor class that provides a +[NSBundle mainBundle] method that can be
// swapped with NSBundle.
@interface TestBundle : NSObject
+ (NSBundle*)mainBundle;
@end

@implementation TestBundle
+ (NSBundle*)mainBundle {
  return g_swizzled_main_bundle;
}
@end

ScopedBundleSwizzlerMac::ScopedBundleSwizzlerMac() {
  CHECK(!g_swizzled_main_bundle);
  CHECK(!g_original_main_bundle);

  g_original_main_bundle = [NSBundle mainBundle];
  g_swizzled_main_bundle =
      [[OCMockObject partialMockForObject:g_original_main_bundle] retain];
  NSString* identifier = base::SysUTF8ToNSString(base::mac::BaseBundleID());
  CHECK(identifier);
  [[[g_swizzled_main_bundle stub] andReturn:identifier] bundleIdentifier];

  class_swizzler_.reset(new base::mac::ScopedObjCClassSwizzler(
      [NSBundle class], [TestBundle class], @selector(mainBundle)));
}

ScopedBundleSwizzlerMac::~ScopedBundleSwizzlerMac() {
  [g_swizzled_main_bundle release];
  g_swizzled_main_bundle = nil;
  g_original_main_bundle = nil;
}
