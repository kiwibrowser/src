// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_LEGACY_TOOLBAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_LEGACY_TOOLBAR_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/util/relaxed_bounds_constraints_hittest.h"

@protocol ToolbarViewDelegate;

@interface LegacyToolbarView : UIView<RelaxedBoundsConstraintsHitTestSupport>
- (instancetype)initWithNibName:(NSString*)name
                         bundle:(NSBundle*)bundle NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;

// Records whether or not the toolbar is currently involved in a transition
// animation.
@property(nonatomic, assign, getter=isAnimatingTransition)
    BOOL animatingTransition;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_LEGACY_TOOLBAR_VIEW_H_
