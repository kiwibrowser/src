// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_VIEW_H_

#import <UIKit/UIKit.h>

@class MDCProgressView;
@class ToolbarButton;
@class ToolbarButtonFactory;
@class ToolbarToolsMenuButton;

// TODO(crbug.com/792440): Once the new notification for fullscreen is
// completed, this protocol can be removed. Protocol handling the fullscreen for
// the Toolbar.
@protocol ToolbarViewFullscreenDelegate
// Called when the frame of the Toolbar View has changed.
- (void)toolbarViewFrameChanged;
@end

// The view displaying the toolbar.
@interface ToolbarView : UIView

// The delegate used to handle frame changes.
@property(nonatomic, weak) id<ToolbarViewFullscreenDelegate> delegate;

// Factory used to create the buttons.
@property(nonatomic, strong) ToolbarButtonFactory* buttonFactory;

// Margin between the leading edge of the view and the leading stack view.
@property(nonatomic, assign) CGFloat leadingMargin;

// The location bar view, containing the omnibox.
@property(nonatomic, strong) UIView* locationBarView;

// The main objects in the view. Positionned as
// [leadingStackView][locationBarContainer][trailingStackView]. The stack views
// contain the buttons which should be shown on each sides of the location bar.
@property(nonatomic, strong, readonly) UIStackView* leadingStackView;
@property(nonatomic, strong, readonly) UIView* locationBarContainer;
@property(nonatomic, strong, readonly) UIStackView* trailingStackView;

// Buttons contained in the main stack views.
@property(nonatomic, strong, readonly) ToolbarToolsMenuButton* toolsMenuButton;
@property(nonatomic, strong, readonly) ToolbarButton* backButton;
@property(nonatomic, strong, readonly) ToolbarButton* forwardButton;
@property(nonatomic, strong, readonly) ToolbarButton* tabSwitchStripButton;
@property(nonatomic, strong, readonly) ToolbarButton* shareButton;
@property(nonatomic, strong, readonly) ToolbarButton* reloadButton;
@property(nonatomic, strong, readonly) ToolbarButton* stopButton;

// Views contained in the locationBarContainer.
@property(nonatomic, strong, readonly)
    UIStackView* locationBarContainerStackView;
@property(nonatomic, strong, readonly) ToolbarButton* locationBarLeadingButton;
@property(nonatomic, strong, readonly) ToolbarButton* contractButton;
@property(nonatomic, strong, readonly) ToolbarButton* voiceSearchButton;
@property(nonatomic, strong, readonly) ToolbarButton* bookmarkButton;

// The shadow below the toolbar when the omnibox is contracted. Lazily
// instantiated.
@property(nonatomic, strong, readonly) UIImageView* shadowView;
// The shadow below the expanded omnibox. Lazily instantiated.
@property(nonatomic, strong, readonly) UIImageView* fullBleedShadowView;
// The shadow below the contracted location bar.
@property(nonatomic, strong, readonly) UIImageView* locationBarShadow;

// Progress bar displayed below the toolbar.
@property(nonatomic, strong, readonly) MDCProgressView* progressBar;

// Background view, used to display the incognito NTP background color on the
// toolbar.
@property(nonatomic, strong, readonly) UIView* backgroundView;

// Constraints used for the regular/contracted Toolbar state that will be
// deactivated and replaced by |_expandedToolbarConstraints| when animating the
// toolbar expansion.
@property(nonatomic, strong, readonly)
    NSMutableArray* regularToolbarConstraints;
// Constraints used to layout the Toolbar to its expanded state. If these are
// active the locationBarContainer will expand to the size of this VC's view.
// The locationBarView will only expand up to the VC's view safeAreaLayoutGuide.
@property(nonatomic, strong, readonly) NSArray* expandedToolbarConstraints;

// These constraints pin the content view to the safe area. They are temporarily
// disabled when a fake safe area is simulated by calling
// activateFakeSafeAreaInsets.
@property(nonatomic, strong, readonly)
    NSLayoutConstraint* leadingSafeAreaConstraint;
@property(nonatomic, strong, readonly)
    NSLayoutConstraint* trailingSafeAreaConstraint;
// Leading and trailing safe area constraint for faking a safe area. These
// constraints are activated by calling activateFakeSafeAreaInsets and
// deactivateFakeSafeAreaInsets.
@property(nonatomic, strong, readonly)
    NSLayoutConstraint* leadingFakeSafeAreaConstraint;
@property(nonatomic, strong, readonly)
    NSLayoutConstraint* trailingFakeSafeAreaConstraint;

// Array containing all the |_leadingStackView| buttons, lazily instantiated.
@property(nonatomic, strong, readonly)
    NSArray<ToolbarButton*>* leadingStackViewButtons;
// Array containing all the |_trailingStackView| buttons, lazily instantiated.
@property(nonatomic, strong, readonly)
    NSArray<ToolbarButton*>* trailingStackViewButtons;

- (void)setUp;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_VIEW_H_
