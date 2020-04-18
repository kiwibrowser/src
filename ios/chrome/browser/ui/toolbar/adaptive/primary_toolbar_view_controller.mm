// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view_controller.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/UIView+SizeClassSupport.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_animator.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_view_controller+subclassing.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view_controller_delegate.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_configuration.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_constants.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tools_menu_button.h"
#import "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/named_guide.h"
#import "ios/third_party/material_components_ios/src/components/ProgressView/src/MaterialProgressView.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PrimaryToolbarViewController ()
// Redefined to be a PrimaryToolbarView.
@property(nonatomic, strong) PrimaryToolbarView* view;
@property(nonatomic, assign) BOOL isNTP;
@end

@implementation PrimaryToolbarViewController

@synthesize delegate = _delegate;
@synthesize isNTP = _isNTP;
@dynamic view;

#pragma mark - Public

- (void)showPrerenderingAnimation {
  __weak PrimaryToolbarViewController* weakSelf = self;
  [self.view.progressBar setProgress:0];
  [self.view.progressBar setHidden:NO
                          animated:YES
                        completion:^(BOOL finished) {
                          [weakSelf stopProgressBar];
                        }];
}

#pragma mark - AdaptiveToolbarViewController

- (void)updateForSideSwipeSnapshotOnNTP:(BOOL)onNTP {
  [super updateForSideSwipeSnapshotOnNTP:onNTP];
  if (!onNTP)
    return;

  self.view.backgroundColor =
      self.buttonFactory.toolbarConfiguration.NTPBackgroundColor;
  self.view.locationBarContainer.hidden = YES;
}

- (void)resetAfterSideSwipeSnapshot {
  [super resetAfterSideSwipeSnapshot];
  self.view.backgroundColor = nil;
  self.view.locationBarContainer.hidden = NO;
}

#pragma mark - NewTabPageControllerDelegate

- (void)setScrollProgressForTabletOmnibox:(CGFloat)progress {
  [super setScrollProgressForTabletOmnibox:progress];
  self.view.locationBarBottomConstraint.constant =
      -kLocationBarVerticalMargin * progress;
  self.view.locationBarContainer.alpha = progress;
}
#pragma mark - UIViewController

- (void)loadView {
  DCHECK(self.buttonFactory);

  self.view =
      [[PrimaryToolbarView alloc] initWithButtonFactory:self.buttonFactory];

  // This method cannot be called from the init as the topSafeAnchor can only be
  // set to topLayoutGuide after the view creation on iOS 10.
  [self.view setUp];
}

- (void)didMoveToParentViewController:(UIViewController*)parent {
  [super didMoveToParentViewController:parent];
  UIView* omniboxView = self.view.locationBarContainer;
  [NamedGuide guideWithName:kOmniboxGuide view:omniboxView].constrainedView =
      omniboxView;
}

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  [self.delegate
      viewControllerTraitCollectionDidChange:previousTraitCollection];
}

#pragma mark - Property accessors

- (void)setLocationBarView:(UIView*)locationBarView {
  self.view.locationBarView = locationBarView;
}

- (void)setIsNTP:(BOOL)isNTP {
  if (isNTP == _isNTP)
    return;
  _isNTP = isNTP;
  if (!isNTP && self.view.cr_widthSizeClass == REGULAR &&
      self.view.cr_heightSizeClass == REGULAR) {
    // Reset any location bar view updates when not an NTP.
    [self setScrollProgressForTabletOmnibox:1];
  }
}

#pragma mark - ActivityServicePositioner

- (UIView*)shareButtonView {
  return self.view.shareButton;
}

#pragma mark - FullscreenUIElement

- (void)updateForFullscreenProgress:(CGFloat)progress {
  CGFloat alphaValue = fmax(progress * 2 - 1, 0);
  self.view.leadingStackView.alpha = alphaValue;
  self.view.trailingStackView.alpha = alphaValue;
  self.view.locationBarHeight.constant = AlignValueToPixel(
      kToolbarHeightFullscreen +
      (kAdaptiveToolbarHeight - 2 * kAdaptiveLocationBarVerticalMargin -
       kToolbarHeightFullscreen) *
          progress);
  self.view.locationBarContainer.backgroundColor =
      [self.buttonFactory.toolbarConfiguration
          locationBarBackgroundColorWithVisibility:alphaValue];
}

- (void)updateForFullscreenEnabled:(BOOL)enabled {
  if (!enabled)
    [self updateForFullscreenProgress:1.0];
}

- (void)finishFullscreenScrollWithAnimator:(FullscreenAnimator*)animator {
  [self addFullscreenAnimationsToAnimator:animator];
}

- (void)scrollFullscreenToTopWithAnimator:(FullscreenAnimator*)animator {
  [self addFullscreenAnimationsToAnimator:animator];
}

- (void)showToolbarWithAnimator:(FullscreenAnimator*)animator {
  [self addFullscreenAnimationsToAnimator:animator];
}

#pragma mark - FullscreenUIElement helpers

- (void)addFullscreenAnimationsToAnimator:(FullscreenAnimator*)animator {
  CGFloat finalProgress = animator.finalProgress;
  [animator addAnimations:^{
    [self updateForFullscreenProgress:finalProgress];
  }];
}

#pragma mark - ToolbarAnimatee

- (void)expandLocationBar {
  [self deactivateViewLocationBarConstraints];
  [NSLayoutConstraint activateConstraints:self.view.expandedConstraints];
  [self.view layoutIfNeeded];
}

- (void)contractLocationBar {
  [self deactivateViewLocationBarConstraints];
  if (IsSplitToolbarMode(self)) {
    [NSLayoutConstraint
        activateConstraints:self.view.contractedNoMarginConstraints];
  } else {
    [NSLayoutConstraint activateConstraints:self.view.contractedConstraints];
  }
  [self.view layoutIfNeeded];
}

- (void)showCancelButton {
  self.view.cancelButton.hidden = NO;
}

- (void)hideCancelButton {
  self.view.cancelButton.hidden = YES;
}

- (void)showControlButtons {
  for (ToolbarButton* button in self.view.allButtons) {
    button.alpha = 1;
  }
}

- (void)hideControlButtons {
  for (ToolbarButton* button in self.view.allButtons) {
    button.alpha = 0;
  }
}

#pragma mark - Private

// Deactivates the constraints on the location bar positioning.
- (void)deactivateViewLocationBarConstraints {
  [NSLayoutConstraint deactivateConstraints:self.view.contractedConstraints];
  [NSLayoutConstraint
      deactivateConstraints:self.view.contractedNoMarginConstraints];
  [NSLayoutConstraint deactivateConstraints:self.view.expandedConstraints];
}

@end
