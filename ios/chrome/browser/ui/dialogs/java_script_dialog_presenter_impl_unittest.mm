// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/dialogs/java_script_dialog_presenter_impl.h"

#import <Foundation/Foundation.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using JavaScriptDialogPresenterImplTest = PlatformTest;

// Tests that GetTruncatedMessageText() correctly truncates the length of a
// string to the expected length.
TEST_F(JavaScriptDialogPresenterImplTest, GetTruncatedMessageText) {
  NSMutableString* text = [@"text" mutableCopy];
  while (text.length < kJavaScriptDialogMaxMessageLength) {
    [text appendString:text];
  }
  ASSERT_GT(text.length, kJavaScriptDialogMaxMessageLength);
  EXPECT_EQ(JavaScriptDialogPresenterImpl::GetTruncatedMessageText(text).length,
            kJavaScriptDialogMaxMessageLength);
}
