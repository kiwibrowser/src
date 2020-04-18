// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_SCROLL_VIEW_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_SCROLL_VIEW_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "cwv_export.h"

NS_ASSUME_NONNULL_BEGIN

@protocol CWVScrollViewDelegate;

// Scroll view inside the web view. This is not a subclass of UIScrollView
// because the underlying //ios/web API only exposes a proxy object of the
// scroll view, not the raw UIScrollView.
//
// These methods are forwarded to the internal UIScrollView. Please see the
// <UIKit/UIScrollView.h> documentation for details about the following methods.
CWV_EXPORT
@interface CWVScrollView : NSObject

// Not KVO compliant.
@property(nonatomic, readonly) CGRect bounds;
@property(nonatomic) UIEdgeInsets scrollIndicatorInsets;
@property(nonatomic, weak) id<CWVScrollViewDelegate> delegate;
@property(nonatomic, readonly, getter=isDecelerating) BOOL decelerating;
@property(nonatomic, readonly, getter=isDragging) BOOL dragging;
@property(nonatomic, readonly, getter=isTracking) BOOL tracking;
@property(nonatomic) BOOL scrollsToTop;
@property(nonatomic)
    UIScrollViewContentInsetAdjustmentBehavior contentInsetAdjustmentBehavior
        API_AVAILABLE(ios(11.0));
@property(nonatomic, readonly) UIPanGestureRecognizer* panGestureRecognizer;
@property(nonatomic, readonly, copy) NSArray<__kindof UIView*>* subviews;

// KVO compliant.
@property(nonatomic) CGPoint contentOffset;
@property(nonatomic, readonly) CGSize contentSize;

// Be careful when using this property. There's a bug with the
// underlying WKWebView where the web view does not respect contentInsets
// properly when laying out content and calculating innerHeight for Javascript.
// Content is laid out based on the entire height of the web view rather than
// the height between the top and bottom insets.
// https://bugs.webkit.org/show_bug.cgi?id=134230
// rdar://23584409 (not available on Open Radar)
//
// Not KVO compliant.
@property(nonatomic) UIEdgeInsets contentInset;

- (void)setContentOffset:(CGPoint)contentOffset animated:(BOOL)animated;
- (void)addGestureRecognizer:(UIGestureRecognizer*)gestureRecognizer;
- (void)removeGestureRecognizer:(UIGestureRecognizer*)gestureRecognizer;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_SCROLL_VIEW_H_
