// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/safe_mode/safe_mode_view_controller.h"
#import "ios/chrome/browser/crash_report/breakpad_helper.h"
#import "ios/chrome/test/base/scoped_block_swizzler.h"
#import "ios/chrome/test/ocmock/OCMockObject+BreakpadControllerTesting.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/breakpad/breakpad/src/client/ios/BreakpadController.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const int kCrashReportCount = 2;

class SafeModeViewControllerTest : public PlatformTest {
 public:
  void SetUp() override {
    PlatformTest::SetUp();

    mock_breakpad_controller_ =
        [OCMockObject mockForClass:[BreakpadController class]];

    // Swizzle +[BreakpadController sharedInstance] to return
    // |mock_breakpad_controller_| instead of the normal singleton instance.
    id implementation_block = ^BreakpadController*(id self) {
      return mock_breakpad_controller_;
    };
    breakpad_controller_shared_instance_swizzler_.reset(new ScopedBlockSwizzler(
        [BreakpadController class], @selector(sharedInstance),
        implementation_block));
  }

  void TearDown() override {
    [[mock_breakpad_controller_ stub] stop];
    breakpad_helper::SetEnabled(false);

    PlatformTest::TearDown();
  }

 protected:
  id mock_breakpad_controller_;
  std::unique_ptr<ScopedBlockSwizzler>
      breakpad_controller_shared_instance_swizzler_;
};

// Verify that +[SafeModeViewController hasSuggestions] returns YES if and only
// if crash reporter and crash report uploading are enabled and there are
// multiple crash reports to upload.
TEST_F(SafeModeViewControllerTest, HasSuggestions) {
  // Test when crash reporter is disabled.
  breakpad_helper::SetEnabled(false);
  EXPECT_OCMOCK_VERIFY(mock_breakpad_controller_);
  EXPECT_FALSE([SafeModeViewController hasSuggestions]);

  breakpad_helper::SetUploadingEnabled(false);
  EXPECT_OCMOCK_VERIFY(mock_breakpad_controller_);
  EXPECT_FALSE([SafeModeViewController hasSuggestions]);

  breakpad_helper::SetUploadingEnabled(true);
  EXPECT_OCMOCK_VERIFY(mock_breakpad_controller_);
  EXPECT_FALSE([SafeModeViewController hasSuggestions]);

  // Test when crash reporter is enabled.
  [[mock_breakpad_controller_ expect] start:NO];
  breakpad_helper::SetEnabled(true);
  EXPECT_OCMOCK_VERIFY(mock_breakpad_controller_);
  EXPECT_FALSE([SafeModeViewController hasSuggestions]);

  [[mock_breakpad_controller_ expect] setUploadingEnabled:NO];
  breakpad_helper::SetUploadingEnabled(false);
  EXPECT_OCMOCK_VERIFY(mock_breakpad_controller_);
  EXPECT_FALSE([SafeModeViewController hasSuggestions]);

  // Test when crash reporter and crash report uploading are enabled.
  [[mock_breakpad_controller_ expect] setUploadingEnabled:YES];
  breakpad_helper::SetUploadingEnabled(true);
  EXPECT_OCMOCK_VERIFY(mock_breakpad_controller_);

  [mock_breakpad_controller_ cr_expectGetCrashReportCount:0];
  EXPECT_FALSE([SafeModeViewController hasSuggestions]);
  EXPECT_OCMOCK_VERIFY(mock_breakpad_controller_);

  [mock_breakpad_controller_ cr_expectGetCrashReportCount:kCrashReportCount];
  EXPECT_TRUE([SafeModeViewController hasSuggestions]);
  EXPECT_OCMOCK_VERIFY(mock_breakpad_controller_);
}

}  // namespace
