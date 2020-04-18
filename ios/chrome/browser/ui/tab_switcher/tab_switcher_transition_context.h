// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_TRANSITION_CONTEXT_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_TRANSITION_CONTEXT_H_

#import <UIKit/UIKit.h>

@class BrowserViewController;
@class TabModel;
@protocol TabStripFoldAnimation;

// Holds the informations about a browser view controller that will be used
// to create the transition from the browser view controller to the tab switcher
// and vice versa.
@interface TabSwitcherTransitionContextContent : NSObject

// Create a transition context content from a browser view controller.
+ (instancetype)tabSwitcherTransitionContextContentFromBVC:
    (BrowserViewController*)bvc;

// Returns a placeholder representing the BVC's tab strip.
- (UIView<TabStripFoldAnimation>*)generateTabStripPlaceholderView;

// Holds a snapshot view of the browser view controller's toolbar.
@property(nonatomic, retain) UIView* toolbarSnapshotView;

// The tabID that was in the browser view controller when the transition context
// content was created. Used to understand what might have changed in the model
// when entering and leaving the tab switcher.
@property(nonatomic, copy) NSString* initialTabID;
@end

// A Tab switcher transition context holds the informations needed to compute
// the transition from a browser view controller to the tab switcher view
// controller.
@interface TabSwitcherTransitionContext : NSObject

// Create a tab switcher transition context from the current browser view
// controller, the main view controller (regular) and the off the record view
// controller (incognito).
+ (instancetype)
tabSwitcherTransitionContextWithCurrent:(BrowserViewController*)currentBVC
                                mainBVC:(BrowserViewController*)mainBVC
                                 otrBVC:(BrowserViewController*)otrBVC;

// The snapshot image of the currently selected tab of the current view
// controller.
@property(nonatomic, retain) UIImage* tabSnapshotImage;
// The tab switcher transition context content for the incognito (or main)
// browser view controller.
@property(nonatomic, retain)
    TabSwitcherTransitionContextContent* incognitoContent;
// The tab switcher transition context content for the regular (or off the
// record) browser view controller.
@property(nonatomic, retain)
    TabSwitcherTransitionContextContent* regularContent;
// The tab model of the broswser view controller selected when the transition
// context was created.
@property(nonatomic, retain) TabModel* initialTabModel;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_TRANSITION_CONTEXT_H_
