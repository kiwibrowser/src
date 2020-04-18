// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_VIEW_H_
#define IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_VIEW_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"
#include "ios/web/public/navigation_manager.h"

@protocol ApplicationCommands;

// Describes the mode of the Sad Tab, whether it should offer an attempt to
// reload content, or whether it should offer a way to provide feedback.
enum class SadTabViewMode {
  RELOAD = 0,  // A mode which allows the user to attempt a reload
  FEEDBACK,    // A mode which allows the user to provide feedback
};

// Protocol for actions from the SadTabView.
@protocol SadTabActionDelegate
// Shows reportAnIssue UI.
- (void)showReportAnIssue;
@end

// The view used to show "sad tab" content to the user when WKWebView's renderer
// process crashes.
@interface SadTabView : UIView

// Designated initializer. |navigationManager| allows the view to execute
// actions such as a reload if necessary.
- (instancetype)initWithMode:(SadTabViewMode)mode
           navigationManager:(web::NavigationManager*)navigationManager
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Determines the type of Sad Tab information that will be displayed.
@property(nonatomic, readonly) SadTabViewMode mode;

// The dispatcher for this view.
@property(nonatomic, weak) id<ApplicationCommands> dispatcher;

// The delegate for actions from this view.
@property(nonatomic, weak) id<SadTabActionDelegate> actionDelegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_VIEW_H_
