// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test.h"

#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BrowserCoordinatorTest::BrowserCoordinatorTest() {
  // Initialize the browser.
  chrome_browser_state_ = TestChromeBrowserState::Builder().Build();
  delegate_ = std::make_unique<FakeWebStateListDelegate>();
  browser_ =
      std::make_unique<Browser>(chrome_browser_state_.get(), delegate_.get());
}

BrowserCoordinatorTest::~BrowserCoordinatorTest() = default;
