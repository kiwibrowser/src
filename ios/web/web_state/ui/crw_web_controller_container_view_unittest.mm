// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/ui/crw_web_controller_container_view.h"

#import "ios/web/web_state/ui/crw_web_view_proxy_impl.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The frame of CRWWebControllerContainerViewTest's |container_view_|.
const CGRect kContainerViewFrame = CGRectMake(0.0f, 0.0f, 200.0f, 300.0f);
// TestToolbarView's frame height.
const CGFloat kTestToolbarViewHeight = 50.0f;
}

// Test view to use as a toolbar.
@interface TestToolbarView : UIView
@end

@implementation TestToolbarView
- (CGSize)sizeThatFits:(CGSize)size {
  return CGSizeMake(size.width, kTestToolbarViewHeight);
}
@end

// Test CRWWebControllerContainerViewDelegate implementation.
@interface TestWebControllerContainerViewDelegate
    : NSObject<CRWWebControllerContainerViewDelegate> {
  CRWWebViewProxyImpl* _proxy;
}
- (instancetype)initWithContentViewProxy:(CRWWebViewProxyImpl*)proxy;
@end

@implementation TestWebControllerContainerViewDelegate

- (instancetype)initWithContentViewProxy:(CRWWebViewProxyImpl*)proxy {
  if ((self = [super init]))
    _proxy = proxy;
  return self;
}

- (CRWWebViewProxyImpl*)contentViewProxyForContainerView:
        (CRWWebControllerContainerView*)containerView {
  return _proxy;
}

- (CGFloat)nativeContentHeaderHeightForContainerView:
    (CRWWebControllerContainerView*)containerView {
  return 0;
}

@end

#pragma mark - CRWWebControllerContainerViewTest

class CRWWebControllerContainerViewTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    CRWWebViewProxyImpl* proxy =
        [OCMockObject niceMockForClass:[CRWWebViewProxyImpl class]];
    delegate_ = [[TestWebControllerContainerViewDelegate alloc]
        initWithContentViewProxy:proxy];
    container_view_ =
        [[CRWWebControllerContainerView alloc] initWithDelegate:delegate_];
    [container_view_ setFrame:kContainerViewFrame];
  }

  // The CRWWebControllerContainerViewDelegate (required for designated
  // initializer).
  TestWebControllerContainerViewDelegate* delegate_;
  // The container view being tested.
  CRWWebControllerContainerView* container_view_;
};

// Tests that |-addToolbar:| will successfully add the passed-in toolbar to the
// container view and will correctly reset its frame to be bottom-aligned with
// the container's width.
TEST_F(CRWWebControllerContainerViewTest, AddToolbar) {
  TestToolbarView* toolbar = [[TestToolbarView alloc] initWithFrame:CGRectZero];
  [container_view_ addToolbar:toolbar];
  [container_view_ layoutIfNeeded];
  // Check that the toolbar has been added to the container view.
  EXPECT_TRUE([toolbar isDescendantOfView:container_view_]);
  // Check the toolbar's frame.
  CGRect expected_toolbar_frame = CGRectMake(
      CGRectGetMinX([container_view_ bounds]),
      CGRectGetMaxY([container_view_ bounds]) - kTestToolbarViewHeight,
      CGRectGetWidth(kContainerViewFrame), kTestToolbarViewHeight);
  CGRect toolbar_container_frame =
      [container_view_ convertRect:[toolbar bounds] fromView:toolbar];
  EXPECT_TRUE(CGRectEqualToRect(expected_toolbar_frame,
                                toolbar_container_frame));
}
