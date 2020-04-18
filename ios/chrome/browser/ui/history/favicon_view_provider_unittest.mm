// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/favicon_view_provider.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon/core/test/mock_favicon_service.h"
#include "components/favicon_base/fallback_icon_style.h"
#include "components/favicon_base/favicon_types.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/web/public/test/test_web_thread.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "skia/ext/skia_utils_ios.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FaviconViewProvider (Testing)
@property(nonatomic, retain) UIImage* favicon;
@property(nonatomic, copy) NSString* fallbackText;
@property(nonatomic, retain) UIColor* fallbackBackgroundColor;
@property(nonatomic, retain) UIColor* fallbackTextColor;
@end

namespace {

using favicon::PostReply;
using testing::_;

// Dummy URL for the favicon case.
const char kTestFaviconURL[] = "http://test/favicon";
// Dummy URL for the fallback case.
const char kTestFallbackURL[] = "http://test/fallback";
// Dummy icon URL.
const char kTestFaviconIconURL[] = "http://test/icons/favicon";

// Size of dummy favicon image.
const CGFloat kTestFaviconSize = 57;

favicon_base::FaviconRawBitmapResult CreateTestBitmap() {
  favicon_base::FaviconRawBitmapResult result;
  result.expired = false;
  base::FilePath favicon_path;
  base::PathService::Get(ios::DIR_TEST_DATA, &favicon_path);
  favicon_path =
      favicon_path.Append(FILE_PATH_LITERAL("favicon/test_favicon.png"));
  NSData* favicon_data = [NSData
      dataWithContentsOfFile:base::SysUTF8ToNSString(favicon_path.value())];
  scoped_refptr<base::RefCountedBytes> data(new base::RefCountedBytes(
      static_cast<const unsigned char*>([favicon_data bytes]),
      [favicon_data length]));

  result.bitmap_data = data;
  CGFloat scaled_size = [UIScreen mainScreen].scale * kTestFaviconSize;
  result.pixel_size = gfx::Size(scaled_size, scaled_size);
  result.icon_url = GURL(kTestFaviconIconURL);
  result.icon_type = favicon_base::IconType::kTouchIcon;
  CHECK(result.is_valid());
  return result;
}

class FaviconViewProviderTest : public PlatformTest {
 protected:
  void SetUp() override {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    PlatformTest::SetUp();
    large_icon_service_.reset(new favicon::LargeIconService(
        &mock_favicon_service_, /*image_fetcher=*/nullptr));

    EXPECT_CALL(mock_favicon_service_, GetLargestRawFaviconForPageURL(
                                           GURL(kTestFaviconURL), _, _, _, _))
        .WillRepeatedly(PostReply<5>(CreateTestBitmap()));
  }

  web::TestWebThreadBundle thread_bundle_;
  testing::StrictMock<favicon::MockFaviconService> mock_favicon_service_;
  std::unique_ptr<favicon::LargeIconService> large_icon_service_;
  base::CancelableTaskTracker cancelable_task_tracker_;
};

// Tests that image is set when a favicon is returned from LargeIconService.
TEST_F(FaviconViewProviderTest, Favicon) {
  id mock_delegate =
      [OCMockObject mockForProtocol:@protocol(FaviconViewProviderDelegate)];
  FaviconViewProvider* viewProvider =
      [[FaviconViewProvider alloc] initWithURL:GURL(kTestFaviconURL)
                                   faviconSize:kTestFaviconSize
                                minFaviconSize:kTestFaviconSize
                              largeIconService:large_icon_service_.get()
                                      delegate:mock_delegate];
  void (^confirmationBlock)(NSInvocation*) = ^(NSInvocation* invocation) {
    __unsafe_unretained FaviconViewProvider* viewProvider;
    [invocation getArgument:&viewProvider atIndex:2];
    EXPECT_NSNE(nil, viewProvider.favicon);
  };
  [[[mock_delegate stub] andDo:confirmationBlock]
      faviconViewProviderFaviconDidLoad:viewProvider];
  EXPECT_OCMOCK_VERIFY(mock_delegate);
}

// Tests that fallback data is set when no favicon is returned from
// LargeIconService.
TEST_F(FaviconViewProviderTest, FallbackIcon) {
  EXPECT_CALL(mock_favicon_service_, GetLargestRawFaviconForPageURL(
                                         GURL(kTestFallbackURL), _, _, _, _))
      .WillRepeatedly(PostReply<5>(favicon_base::FaviconRawBitmapResult()));

  id mock_delegate =
      [OCMockObject mockForProtocol:@protocol(FaviconViewProviderDelegate)];
  FaviconViewProvider* item =
      [[FaviconViewProvider alloc] initWithURL:GURL(kTestFallbackURL)
                                   faviconSize:kTestFaviconSize
                                minFaviconSize:kTestFaviconSize
                              largeIconService:large_icon_service_.get()
                                      delegate:mock_delegate];

  // Confirm that fallback text and color have been set before delegate call.
  void (^confirmationBlock)(NSInvocation*) = ^(NSInvocation* invocation) {
    __unsafe_unretained FaviconViewProvider* viewProvider;
    [invocation getArgument:&viewProvider atIndex:2];
    // Fallback text is the first letter of the URL.
    NSString* defaultText = @"T";
    // Default colors are defined in
    // components/favicon_base/fallback_icon_style.h.
    UIColor* defaultTextColor = skia::UIColorFromSkColor(SK_ColorWHITE);
    UIColor* defaultBackgroundColor =
        skia::UIColorFromSkColor(SkColorSetRGB(0x78, 0x78, 0x78));
    EXPECT_NSEQ(defaultText, viewProvider.fallbackText);
    EXPECT_NSEQ(defaultTextColor, viewProvider.fallbackTextColor);
    EXPECT_NSEQ(defaultBackgroundColor, viewProvider.fallbackBackgroundColor);
  };
  [[[mock_delegate stub] andDo:confirmationBlock]
      faviconViewProviderFaviconDidLoad:item];
  EXPECT_OCMOCK_VERIFY(mock_delegate);
}

}  // namespace
