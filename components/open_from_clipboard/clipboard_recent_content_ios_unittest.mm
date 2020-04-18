// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/open_from_clipboard/clipboard_recent_content_ios.h"

#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>

#include <memory>

#include "base/strings/sys_string_conversions.h"
#import "components/open_from_clipboard/clipboard_recent_content_impl_ios.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

UIImage* TestUIImage() {
  CGRect frame = CGRectMake(0, 0, 1.0, 1.0);
  UIGraphicsBeginImageContext(frame.size);

  CGContextRef context = UIGraphicsGetCurrentContext();
  CGContextSetFillColorWithColor(context, [UIColor redColor].CGColor);
  CGContextFillRect(context, frame);

  UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();

  return image;
}

void SetPasteboardImage(UIImage* image) {
  [[UIPasteboard generalPasteboard] setImage:image];
}

void SetPasteboardContent(const char* data) {
  [[UIPasteboard generalPasteboard]
               setValue:[NSString stringWithUTF8String:data]
      forPasteboardType:@"public.plain-text"];
}
const char kUnrecognizedURL[] = "bad://foo/";
const char kRecognizedURL[] = "good://bar/";
const char kRecognizedURL2[] = "good://bar/2";
const char kAppSpecificURL[] = "test://qux/";
const char kAppSpecificScheme[] = "test";
const char kRecognizedScheme[] = "good";
NSTimeInterval kLongerThanMaxAge = 60 * 60 * 7;
NSTimeInterval kMaxAge = 60 * 60 * 1;
}  // namespace

@interface ClipboardRecentContentImplIOSWithFakeUptime
    : ClipboardRecentContentImplIOS
@property(nonatomic) NSTimeInterval fakeUptime;

- (instancetype)initWithMaxAge:(NSTimeInterval)maxAge
             authorizedSchemes:(NSArray*)authorizedSchemes
                  userDefaults:(NSUserDefaults*)groupUserDefaults
                        uptime:(NSTimeInterval)uptime;

@end

@implementation ClipboardRecentContentImplIOSWithFakeUptime

@synthesize fakeUptime = _fakeUptime;

- (instancetype)initWithMaxAge:(NSTimeInterval)maxAge
             authorizedSchemes:(NSSet*)authorizedSchemes
                  userDefaults:(NSUserDefaults*)groupUserDefaults
                        uptime:(NSTimeInterval)uptime {
  self = [super initWithMaxAge:maxAge
             authorizedSchemes:authorizedSchemes
                  userDefaults:groupUserDefaults
                      delegate:nil];
  if (self) {
    _fakeUptime = uptime;
  }
  return self;
}

- (NSTimeInterval)uptime {
  return self.fakeUptime;
}

@end

class ClipboardRecentContentIOSWithFakeUptime
    : public ClipboardRecentContentIOS {
 public:
  ClipboardRecentContentIOSWithFakeUptime(
      ClipboardRecentContentImplIOS* implementation)
      : ClipboardRecentContentIOS(implementation) {}
};

class ClipboardRecentContentIOSTest : public ::testing::Test {
 protected:
  ClipboardRecentContentIOSTest() {
    // By default, set that the device booted 10 days ago.
    ResetClipboardRecentContent(kAppSpecificScheme,
                                base::TimeDelta::FromDays(10));
  }

  void SimulateDeviceRestart() {
    ResetClipboardRecentContent(kAppSpecificScheme,
                                base::TimeDelta::FromSeconds(0));
  }

  void ResetClipboardRecentContent(const std::string& application_scheme,
                                   base::TimeDelta time_delta) {
    clipboard_content_implementation_ =
        [[ClipboardRecentContentImplIOSWithFakeUptime alloc]
               initWithMaxAge:kMaxAge
            authorizedSchemes:@[
              base::SysUTF8ToNSString(kRecognizedScheme),
              base::SysUTF8ToNSString(application_scheme)
            ]
                 userDefaults:[NSUserDefaults standardUserDefaults]
                       uptime:time_delta.InSecondsF()];

    clipboard_content_ =
        std::make_unique<ClipboardRecentContentIOSWithFakeUptime>(
            clipboard_content_implementation_);
  }

  void SetStoredPasteboardChangeDate(NSDate* change_date) {
    clipboard_content_implementation_.lastPasteboardChangeDate =
        [change_date copy];
    [clipboard_content_implementation_ saveToUserDefaults];
  }

 protected:
  std::unique_ptr<ClipboardRecentContentIOSWithFakeUptime> clipboard_content_;
  ClipboardRecentContentImplIOSWithFakeUptime*
      clipboard_content_implementation_;
};

TEST_F(ClipboardRecentContentIOSTest, SchemeFiltering) {
  GURL gurl;

  // Test unrecognized URL.
  SetPasteboardContent(kUnrecognizedURL);
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));

  // Test recognized URL.
  SetPasteboardContent(kRecognizedURL);
  EXPECT_TRUE(clipboard_content_->GetRecentURLFromClipboard(&gurl));
  EXPECT_STREQ(kRecognizedURL, gurl.spec().c_str());

  // Test URL with app specific scheme.
  SetPasteboardContent(kAppSpecificURL);
  EXPECT_TRUE(clipboard_content_->GetRecentURLFromClipboard(&gurl));
  EXPECT_STREQ(kAppSpecificURL, gurl.spec().c_str());

  // Test URL without app specific scheme.
  ResetClipboardRecentContent(std::string(), base::TimeDelta::FromDays(10));

  SetPasteboardContent(kAppSpecificURL);
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));
}

TEST_F(ClipboardRecentContentIOSTest, PasteboardURLObsolescence) {
  GURL gurl;
  SetPasteboardContent(kRecognizedURL);

  // Test that recent pasteboard data is provided.
  EXPECT_TRUE(clipboard_content_->GetRecentURLFromClipboard(&gurl));
  EXPECT_STREQ(kRecognizedURL, gurl.spec().c_str());

  // Test that old pasteboard data is not provided.
  SetStoredPasteboardChangeDate(
      [NSDate dateWithTimeIntervalSinceNow:-kLongerThanMaxAge]);
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));

  // Tests that if chrome is relaunched, old pasteboard data is still
  // not provided.
  ResetClipboardRecentContent(kAppSpecificScheme,
                              base::TimeDelta::FromDays(10));
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));

  SimulateDeviceRestart();
  // Tests that if the device is restarted, old pasteboard data is still
  // not provided.
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));
}

TEST_F(ClipboardRecentContentIOSTest, SupressedPasteboard) {
  GURL gurl;
  SetPasteboardContent(kRecognizedURL);

  // Test that recent pasteboard data is provided.
  EXPECT_TRUE(clipboard_content_->GetRecentURLFromClipboard(&gurl));

  // Suppress the content of the pasteboard.
  clipboard_content_->SuppressClipboardContent();

  // Check that the pasteboard content is suppressed.
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));

  // Create a new clipboard content to test persistence.
  ResetClipboardRecentContent(kAppSpecificScheme,
                              base::TimeDelta::FromDays(10));

  // Check that the pasteboard content is still suppressed.
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));

  // Check that even if the device is restarted, pasteboard content is
  // still suppressed.
  SimulateDeviceRestart();
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));

  // Check that if the pasteboard changes, the new content is not
  // supressed anymore.
  SetPasteboardContent(kRecognizedURL2);
  EXPECT_TRUE(clipboard_content_->GetRecentURLFromClipboard(&gurl));
}

// Checks that if user copies something other than a string we don't cache the
// string in pasteboard.
TEST_F(ClipboardRecentContentIOSTest, AddingNonStringRemovesCachedString) {
  GURL gurl;
  SetPasteboardContent(kRecognizedURL);

  // Test that recent pasteboard data is provided.
  EXPECT_TRUE(clipboard_content_->GetRecentURLFromClipboard(&gurl));
  EXPECT_STREQ(kRecognizedURL, gurl.spec().c_str());

  // Overwrite pasteboard with an image.
  base::scoped_nsobject<UIImage> image([[UIImage alloc] init]);
  SetPasteboardImage(TestUIImage());

  // Pasteboard should appear empty.
  EXPECT_FALSE(clipboard_content_->GetRecentURLFromClipboard(&gurl));

  // Tests that if URL is added again, pasteboard provides it normally.
  SetPasteboardContent(kRecognizedURL);
  EXPECT_TRUE(clipboard_content_->GetRecentURLFromClipboard(&gurl));
  EXPECT_STREQ(kRecognizedURL, gurl.spec().c_str());
}
