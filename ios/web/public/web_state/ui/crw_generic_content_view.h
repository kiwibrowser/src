// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEB_STATE_UI_CRW_GENERIC_CONTENT_VIEW_H_
#define IOS_WEB_PUBLIC_WEB_STATE_UI_CRW_GENERIC_CONTENT_VIEW_H_

#import "ios/web/public/web_state/ui/crw_content_view.h"

// Wraps an arbitrary native UIView in a CRWContentView.
@interface CRWGenericContentView : CRWContentView

// The view that was passed to |-initWithContentView:|.  This is the view that
// is displayed in |self.scrollView|.
@property(nonatomic, strong, readonly) UIView* view;

// Initializes the CRWNativeContentContainerView to display |view|, which
// will be added to the scroll view.
- (instancetype)initWithView:(UIView*)view NS_DESIGNATED_INITIALIZER;

// CRWGenericContentViews should be initialized via |-initWithView:|.
- (instancetype)initWithCoder:(NSCoder*)decoder NS_UNAVAILABLE;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

@end

#endif  // IOS_WEB_PUBLIC_WEB_STATE_UI_CRW_GENERIC_CONTENT_VIEW_H_
