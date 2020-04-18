// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/crw_navigation_item_storage.h"

#import <Foundation/Foundation.h>
#include <stdint.h>

#include <utility>

#include "base/strings/sys_string_conversions.h"
#import "ios/web/navigation/navigation_item_impl.h"
#import "ios/web/navigation/navigation_item_storage_test_util.h"
#include "ios/web/public/referrer.h"
#import "net/base/mac/url_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/gtest_support.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class CRWNavigationItemStorageTest : public PlatformTest {
 protected:
  CRWNavigationItemStorageTest()
      : item_storage_([[CRWNavigationItemStorage alloc] init]) {
    // Set up |item_storage_|.
    [item_storage_ setVirtualURL:GURL("http://init.test")];
    [item_storage_ setReferrer:web::Referrer(GURL("http://referrer.url"),
                                             web::ReferrerPolicyDefault)];
    [item_storage_ setTimestamp:base::Time::Now()];
    [item_storage_ setTitle:base::SysNSStringToUTF16(@"Title")];
    [item_storage_
        setDisplayState:web::PageDisplayState(0.0, 0.0, 0.0, 0.0, 0.0)];
    [item_storage_
        setPOSTData:[@"Test data" dataUsingEncoding:NSUTF8StringEncoding]];
    [item_storage_ setHTTPRequestHeaders:@{ @"HeaderKey" : @"HeaderValue" }];
    [item_storage_ setUserAgentType:web::UserAgentType::DESKTOP];
  }

  // Convenience getter to facilitate dot notation in tests.
  CRWNavigationItemStorage* item_storage() { return item_storage_; }

 protected:
  CRWNavigationItemStorage* item_storage_;
};

// Tests initializing with the legacy keys.
TEST_F(CRWNavigationItemStorageTest, InitWithCoderLegacy) {
  NSURL* virtualURL = net::NSURLWithGURL(item_storage().virtualURL);
  NSURL* referrerURL = net::NSURLWithGURL(item_storage().referrer.url);
  NSString* title = base::SysUTF16ToNSString(item_storage().title);
  // Legacy NavigationItems don't persist timestamp.
  item_storage().timestamp = base::Time::FromCFAbsoluteTime(0);

  // Set up archiver and unarchiver.
  NSMutableData* data = [[NSMutableData alloc] init];
  NSKeyedArchiver* archiver =
      [[NSKeyedArchiver alloc] initForWritingWithMutableData:data];
  [archiver encodeObject:virtualURL
                  forKey:web::kNavigationItemStorageURLDeperecatedKey];
  [archiver encodeObject:referrerURL
                  forKey:web::kNavigationItemStorageReferrerURLDeprecatedKey];
  [archiver encodeObject:title forKey:web::kNavigationItemStorageTitleKey];
  NSDictionary* display_state_dict =
      item_storage().displayState.GetSerialization();
  [archiver encodeObject:display_state_dict
                  forKey:web::kNavigationItemStoragePageDisplayStateKey];
  [archiver
      encodeBool:YES
          forKey:web::kNavigationItemStorageUseDesktopUserAgentDeprecatedKey];
  NSDictionary* request_headers = item_storage().HTTPRequestHeaders;
  [archiver encodeObject:request_headers
                  forKey:web::kNavigationItemStorageHTTPRequestHeadersKey];
  [archiver encodeObject:item_storage().POSTData
                  forKey:web::kNavigationItemStoragePOSTDataKey];
  BOOL skip_repost_form_confirmation =
      item_storage().shouldSkipRepostFormConfirmation;
  [archiver
      encodeBool:skip_repost_form_confirmation
          forKey:web::kNavigationItemStorageSkipRepostFormConfirmationKey];
  [archiver finishEncoding];
  NSKeyedUnarchiver* unarchiver =
      [[NSKeyedUnarchiver alloc] initForReadingWithData:data];

  // Create a CRWNavigationItemStorage and verify that it is equivalent.
  CRWNavigationItemStorage* new_storage =
      [[CRWNavigationItemStorage alloc] initWithCoder:unarchiver];
  EXPECT_TRUE(web::ItemStoragesAreEqual(item_storage(), new_storage));
}

// Tests that unarchiving CRWNavigationItemStorage data results in an equivalent
// storage.
TEST_F(CRWNavigationItemStorageTest, EncodeDecode) {
  NSData* data = [NSKeyedArchiver archivedDataWithRootObject:item_storage()];
  id decoded = [NSKeyedUnarchiver unarchiveObjectWithData:data];
  EXPECT_TRUE(web::ItemStoragesAreEqual(item_storage(), decoded));
}
