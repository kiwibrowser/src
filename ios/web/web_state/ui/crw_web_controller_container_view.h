// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEB_STATE_UI_CRW_WEB_CONTROLLER_CONTAINER_VIEW_H_
#define IOS_WEB_WEB_STATE_UI_CRW_WEB_CONTROLLER_CONTAINER_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/web/public/web_state/ui/crw_content_view.h"

@class CRWWebControllerContainerView;
@class CRWWebViewContentView;
@class CRWWebViewProxyImpl;
@protocol CRWNativeContent;

@protocol CRWWebControllerContainerViewDelegate<NSObject>

// Returns the proxy object that's backed by the CRWContentView displayed by
// |containerView|.
- (CRWWebViewProxyImpl*)contentViewProxyForContainerView:
        (CRWWebControllerContainerView*)containerView;

// Returns the height for any toolbars that overlap the top native content.
- (CGFloat)nativeContentHeaderHeightForContainerView:
    (CRWWebControllerContainerView*)containerView;

@end

// Container view class that manages the display of content within
// CRWWebController.
@interface CRWWebControllerContainerView : UIView

#pragma mark Content Views
// The web view content view being displayed.
@property(nonatomic, strong, readonly)
    CRWWebViewContentView* webViewContentView;
// The native controller whose content is being displayed.
@property(nonatomic, strong, readonly) id<CRWNativeContent> nativeController;
// The currently displayed transient content view.
@property(nonatomic, strong, readonly) CRWContentView* transientContentView;
@property(nonatomic, weak) id<CRWWebControllerContainerViewDelegate>
    delegate;  // weak

// Designated initializer.  |proxy|'s content view will be updated as different
// content is added to the container.
- (instancetype)initWithDelegate:
        (id<CRWWebControllerContainerViewDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

// CRWWebControllerContainerView should be initialized via
// |-initWithContentViewProxy:|.
- (instancetype)initWithCoder:(NSCoder*)decoder NS_UNAVAILABLE;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

// Returns YES if the container view is currently displaying content.
- (BOOL)isViewAlive;

// Removes all subviews and resets state to default.
- (void)resetContent;

// Replaces the currently displayed content with |webViewContentView|.
- (void)displayWebViewContentView:(CRWWebViewContentView*)webViewContentView;

// Replaces the currently displayed content with |nativeController|'s view.
- (void)displayNativeContent:(id<CRWNativeContent>)nativeController;

// Adds |transientContentView| as a subview above previously displayed content.
- (void)displayTransientContent:(CRWContentView*)transientContentView;

// Removes the transient content view, if one is displayed.
- (void)clearTransientContentView;

#pragma mark Toolbars

// |toolbar| will be resized to the container width, bottom-aligned, and added
// as the topmost subview.
- (void)addToolbar:(UIView*)toolbar;

// Adds each toolbar in |toolbars|.
- (void)addToolbars:(NSArray*)toolbars;

// Removes |toolbar| as a subview.
- (void)removeToolbar:(UIView*)toolbar;

// Removes all toolbars added via |-addToolbar:|.
- (void)removeAllToolbars;

@end

#endif  // IOS_WEB_WEB_STATE_UI_CRW_WEB_CONTROLLER_CONTAINER_VIEW_H_
