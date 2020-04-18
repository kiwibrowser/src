// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_view_controller.h"

#import "base/mac/foundation_util.h"
#include "base/metrics/user_metrics.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/commands/history_popup_commands.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_animator.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_component_options.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_configuration.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_constants.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tools_menu_button.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_updater.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_view.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"
#import "ios/chrome/browser/ui/toolbar/public/toolbar_controller_base_feature.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/browser/ui/util/named_guide.h"
#import "ios/chrome/browser/ui/util/named_guide_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#import "ios/third_party/material_components_ios/src/components/ProgressView/src/MaterialProgressView.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Fullscreen constants.
const CGFloat kIPadToolbarY = 53;
const CGFloat kScrollFadeDistance = 30;
}

@interface ToolbarViewController ()<ToolbarViewFullscreenDelegate>
@property(nonatomic, strong) ToolbarButtonFactory* buttonFactory;
@property(nonatomic, strong) ToolbarButtonUpdater* buttonUpdater;

@property(nonatomic, assign) BOOL voiceSearchEnabled;
// Whether a page is loading.
@property(nonatomic, assign, getter=isLoading) BOOL loading;

@property(nonatomic, strong) ToolbarView* view;

// Whether the location bar inset its page margins to the safe area when it was
// set.
@property(nonatomic, assign) BOOL locationBarDidInsetLayoutMargins;

@end

@implementation ToolbarViewController
@dynamic view;
@synthesize buttonFactory = _buttonFactory;
@synthesize buttonUpdater = _buttonUpdater;
@synthesize loading = _loading;
@synthesize locationBarDidInsetLayoutMargins =
    _locationBarDidInsetLayoutMargins;
@synthesize voiceSearchEnabled = _voiceSearchEnabled;
@synthesize omniboxFocuser = _omniboxFocuser;
@synthesize dispatcher = _dispatcher;
@synthesize expanded = _expanded;

#pragma mark - Public

- (instancetype)initWithDispatcher:
                    (id<ApplicationCommands, BrowserCommands>)dispatcher
                     buttonFactory:(ToolbarButtonFactory*)buttonFactory
                     buttonUpdater:(ToolbarButtonUpdater*)buttonUpdater
                    omniboxFocuser:(id<OmniboxFocuser>)omniboxFocuser {
  _dispatcher = dispatcher;
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _buttonFactory = buttonFactory;
    _buttonUpdater = buttonUpdater;
    _omniboxFocuser = omniboxFocuser;
    _expanded = NO;
  }
  return self;
}

- (void)addToolbarExpansionAnimations:(UIViewPropertyAnimator*)animator
                   completionAnimator:
                       (UIViewPropertyAnimator*)completionAnimator {
  // iPad should never try to animate.
  DCHECK(!IsIPadIdiom());
  [NSLayoutConstraint
      deactivateConstraints:self.view.regularToolbarConstraints];
  [NSLayoutConstraint activateConstraints:self.view.expandedToolbarConstraints];
  // By unhiding the button we will make it layout into the correct position in
  // the StackView.
  self.view.contractButton.hidden = NO;
  self.view.contractButton.alpha = 0;

  void (^animations)() = ^{
    [self setHorizontalTranslationOffset:kToolbarButtonAnimationOffset
                                forViews:self.view.leadingStackViewButtons];
    [self setHorizontalTranslationOffset:-kToolbarButtonAnimationOffset
                                forViews:self.view.trailingStackViewButtons];
    [self setAllToolbarButtonsOpacity:0];
  };
  void (^completion)(UIViewAnimatingPosition finalPosition) = ^(
      UIViewAnimatingPosition finalPosition) {
    [self setHorizontalTranslationOffset:0
                                forViews:self.view.leadingStackViewButtons];
    [self setHorizontalTranslationOffset:0
                                forViews:self.view.trailingStackViewButtons];
  };

  [UIViewPropertyAnimator
      runningPropertyAnimatorWithDuration:ios::material::kDuration2
                                    delay:0
                                  options:UIViewAnimationOptionCurveEaseIn
                               animations:animations
                               completion:completion];

  [animator addAnimations:^{
    [self.view layoutIfNeeded];
    self.view.shadowView.alpha = 0;
    self.view.fullBleedShadowView.alpha = 1;
    self.view.locationBarShadow.alpha = 0;
  }];

  // If locationBarLeadingButton exists fade it in.
  if (self.view.locationBarLeadingButton) {
    [self
        setHorizontalTranslationOffset:-kToolbarButtonAnimationOffset
                              forViews:@[ self.view.locationBarLeadingButton ]];
    [animator addAnimations:^{
      self.view.locationBarLeadingButton.hidden = NO;
      [self setHorizontalTranslationOffset:0
                                  forViews:@[
                                    self.view.locationBarLeadingButton
                                  ]];
      self.view.locationBarLeadingButton.alpha = 1;
    }
                delayFactor:ios::material::kDuration2];
  }

  [animator addCompletion:^(UIViewAnimatingPosition finalPosition) {
    CGFloat borderWidth = (finalPosition == UIViewAnimatingPositionEnd)
                              ? 0
                              : kLocationBarBorderWidth;
    self.view.locationBarContainer.layer.borderWidth = borderWidth;
  }];

  // When the locationBarContainer has been expanded the Contract button will
  // fade in.
  [self setHorizontalTranslationOffset:kToolbarButtonAnimationOffset
                              forViews:@[ self.view.contractButton ]];
  [completionAnimator addAnimations:^{
    self.view.contractButton.alpha = 1;
    [self setHorizontalTranslationOffset:0
                                forViews:@[ self.view.contractButton ]];
  }];

  self.expanded = YES;
}

- (void)addToolbarContractionAnimations:(UIViewPropertyAnimator*)animator {
  // iPad should never try to animate.
  DCHECK(!IsIPadIdiom());

  // If locationBarLeadingButton exists fade it out before the rest of the
  // Toolbar is contracted.
  if (self.view.locationBarLeadingButton) {
    [UIViewPropertyAnimator
        runningPropertyAnimatorWithDuration:ios::material::kDuration2
        delay:0
        options:UIViewAnimationOptionCurveEaseIn
        animations:^{
          self.view.locationBarLeadingButton.alpha = 0;
          [self setHorizontalTranslationOffset:-kToolbarButtonAnimationOffset
                                      forViews:@[
                                        self.view.locationBarLeadingButton
                                      ]];
        }
        completion:^(UIViewAnimatingPosition finalPosition) {
          [self setHorizontalTranslationOffset:0
                                      forViews:@[
                                        self.view.locationBarLeadingButton
                                      ]];
        }];
  }

  [NSLayoutConstraint
      deactivateConstraints:self.view.expandedToolbarConstraints];
  [NSLayoutConstraint activateConstraints:self.view.regularToolbarConstraints];
  // Change the Toolbar buttons opacity to 0 since these will fade in once the
  // locationBarContainer has been contracted.
  [self setAllToolbarButtonsOpacity:0];
  [animator addAnimations:^{
    self.view.locationBarContainer.layer.borderWidth = kLocationBarBorderWidth;
    [self.view layoutIfNeeded];
    self.view.contractButton.hidden = YES;
    self.view.contractButton.alpha = 0;
    self.view.shadowView.alpha = 1;
    self.view.fullBleedShadowView.alpha = 0;
    self.view.locationBarShadow.alpha = 1;
    self.view.locationBarLeadingButton.hidden = YES;
  }];

  // Once the locationBarContainer has been contracted fade in ToolbarButtons.
  void (^fadeButtonsIn)() = ^{
    [self.view layoutIfNeeded];
    [self setHorizontalTranslationOffset:0
                                forViews:self.view.leadingStackViewButtons];
    [self setHorizontalTranslationOffset:0
                                forViews:self.view.trailingStackViewButtons];
    [self setAllToolbarButtonsOpacity:1];
  };
  [animator addCompletion:^(UIViewAnimatingPosition finalPosition) {
    [self setHorizontalTranslationOffset:kToolbarButtonAnimationOffset
                                forViews:self.view.leadingStackViewButtons];
    [self setHorizontalTranslationOffset:-kToolbarButtonAnimationOffset
                                forViews:self.view.trailingStackViewButtons];
    [UIViewPropertyAnimator
        runningPropertyAnimatorWithDuration:ios::material::kDuration1
                                      delay:ios::material::kDuration4
                                    options:UIViewAnimationOptionCurveEaseOut
                                 animations:fadeButtonsIn
                                 completion:nil];
  }];
  self.expanded = NO;
}

- (void)updateForSnapshotOnNTP:(BOOL)onNTP {
  self.view.progressBar.hidden = YES;
  if (onNTP) {
    self.view.backgroundView.alpha = 1;
    self.view.locationBarContainer.hidden = YES;
    // The back button is visible only if the forward button is enabled.
    self.view.backButton.hiddenInCurrentState =
        !self.view.forwardButton.enabled;
  }
}

- (void)resetAfterSnapshot {
  self.view.backgroundView.alpha = 0;
  self.view.locationBarContainer.hidden = NO;
  self.view.backButton.hiddenInCurrentState = NO;
}

- (void)setBackgroundToIncognitoNTPColorWithAlpha:(CGFloat)alpha {
  self.view.backgroundView.alpha = alpha;
  self.view.shadowView.alpha = 1 - alpha;
  self.view.progressBar.alpha = 1 - alpha;
}

- (void)showPrerenderingAnimation {
  __weak ToolbarViewController* weakSelf = self;
  [self.view.progressBar setProgress:0];
  [self.view.progressBar setHidden:NO
                          animated:YES
                        completion:^(BOOL finished) {
                          [weakSelf stopProgressBar];
                        }];
}

- (void)locationBarIsFirstResonderOnIPad:(BOOL)isFirstResponder {
  // This is an iPad only function.
  DCHECK(IsIPadIdiom());
  self.view.bookmarkButton.hiddenInCurrentState = isFirstResponder;
}

- (void)activateFakeSafeAreaInsets:(UIEdgeInsets)fakeSafeAreaInsets {
  self.view.leadingFakeSafeAreaConstraint.constant =
      UIEdgeInsetsGetLeading(fakeSafeAreaInsets) + [self leadingMargin];
  self.view.trailingFakeSafeAreaConstraint.constant =
      -UIEdgeInsetsGetTrailing(fakeSafeAreaInsets);
  self.view.leadingSafeAreaConstraint.active = NO;
  self.view.trailingSafeAreaConstraint.active = NO;
  self.view.leadingFakeSafeAreaConstraint.active = YES;
  self.view.trailingFakeSafeAreaConstraint.active = YES;
}

- (void)deactivateFakeSafeAreaInsets {
  self.view.leadingFakeSafeAreaConstraint.active = NO;
  self.view.trailingFakeSafeAreaConstraint.active = NO;
  self.view.leadingSafeAreaConstraint.active = YES;
  self.view.trailingSafeAreaConstraint.active = YES;
}

#pragma mark - View lifecyle

- (void)loadView {
  self.view = [[ToolbarView alloc] init];
  self.view.delegate = self;
  self.view.buttonFactory = self.buttonFactory;
  self.view.leadingMargin = [self leadingMargin];

  [self.view setUp];
  [self setUpToolbarButtons];
}

- (void)viewDidAppear:(BOOL)animated {
  [self updateAllButtonsVisibility];
  [super viewDidAppear:animated];
}

#pragma mark - Property accessors

- (void)setLocationBarView:(UIView*)locationBarView {
  // Don't inset |locationBarView|'s layout margins from the safe area because
  // it's being added to a FullscreenUIElement that is expected to animate
  // ouside of the safe area layout guide.
  if (@available(iOS 11, *)) {
    self.view.locationBarView.insetsLayoutMarginsFromSafeArea =
        self.locationBarDidInsetLayoutMargins;
    self.locationBarDidInsetLayoutMargins =
        locationBarView.insetsLayoutMarginsFromSafeArea;
  }
  self.view.locationBarView = locationBarView;
  if (@available(iOS 11, *))
    self.view.locationBarView.insetsLayoutMarginsFromSafeArea = NO;
}

- (ToolbarToolsMenuButton*)toolsMenuButton {
  return self.view.toolsMenuButton;
}

- (UIColor*)backgroundColorNTP {
  return self.view.backgroundView.backgroundColor;
}

#pragma mark - Components Setup

- (void)setUpToolbarButtons {
  // Back button.
  UILongPressGestureRecognizer* backHistoryLongPress =
      [[UILongPressGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(handleLongPress:)];
  [self.view.backButton addGestureRecognizer:backHistoryLongPress];
  [self addStandardActionsForButton:self.view.backButton];

  // Forward button.
  UILongPressGestureRecognizer* forwardHistoryLongPress =
      [[UILongPressGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(handleLongPress:)];
  [self.view.forwardButton addGestureRecognizer:forwardHistoryLongPress];
  [self addStandardActionsForButton:self.view.forwardButton];

  // TabSwitcher button.
  [self addStandardActionsForButton:self.view.tabSwitchStripButton];

  // TODO(crbug.com/799601): Delete this once its not needed.
  if (base::FeatureList::IsEnabled(kMemexTabSwitcher)) {
    UILongPressGestureRecognizer* tabSwitcherLongPress =
        [[UILongPressGestureRecognizer alloc]
            initWithTarget:self
                    action:@selector(handleLongPress:)];
    [self.view.tabSwitchStripButton addGestureRecognizer:tabSwitcherLongPress];
  }

  // Tools menu button.
  [self addStandardActionsForButton:self.view.toolsMenuButton];

  // Share button.
  [self addStandardActionsForButton:self.view.shareButton];

  // Reload button.
  [self addStandardActionsForButton:self.view.reloadButton];

  // Stop button.
  self.view.stopButton.hiddenInCurrentState = YES;
  [self addStandardActionsForButton:self.view.stopButton];

  // Voice Search button.
  self.view.voiceSearchButton.enabled = NO;
  [self addStandardActionsForButton:self.view.voiceSearchButton];

  // Bookmark button.
  [self addStandardActionsForButton:self.view.bookmarkButton];

  // Contract button.
  self.view.contractButton.alpha = 0;
  self.view.contractButton.hidden = YES;

  // LocationBar LeadingButton
  self.view.locationBarLeadingButton.alpha = 0;
  self.view.locationBarLeadingButton.hidden = YES;

  // Add buttons to button updater.
  self.buttonUpdater.backButton = self.view.backButton;
  self.buttonUpdater.forwardButton = self.view.forwardButton;
  self.buttonUpdater.voiceSearchButton = self.view.voiceSearchButton;
}

#pragma mark - Button Actions

- (void)handleLongPress:(UILongPressGestureRecognizer*)gesture {
  if (gesture.state != UIGestureRecognizerStateBegan)
    return;

  if (gesture.view == self.view.backButton) {
    [self.dispatcher showTabHistoryPopupForBackwardHistory];
  } else if (gesture.view == self.view.forwardButton) {
    [self.dispatcher showTabHistoryPopupForForwardHistory];
  } else if (gesture.view == self.view.tabSwitchStripButton) {
    // TODO(crbug.com/799601): Delete this once its not needed.
    [self.dispatcher displayTabSwitcher];
  }
}

#pragma mark - View Controller Containment

- (void)addChildViewController:(UIViewController*)viewController
                     toSubview:(UIView*)subview {
  if (!viewController || !subview) {
    return;
  }
  [self addChildViewController:viewController];
  viewController.view.translatesAutoresizingMaskIntoConstraints = YES;
  viewController.view.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  viewController.view.frame = subview.bounds;
  [subview addSubview:viewController.view];
  [viewController didMoveToParentViewController:self];
}

- (void)didMoveToParentViewController:(UIViewController*)parent {
  SetNamedGuideConstrainedViews(@{
    kOmniboxGuide : self.view.locationBarContainer,
    kBackButtonGuide : self.view.backButton.imageView,
    kForwardButtonGuide : self.view.forwardButton.imageView,
    kToolsMenuGuide : self.view.toolsMenuButton,
  });
  if (!IsIPadIdiom()) {
    UIView* tabSwitcherButton = self.view.tabSwitchStripButton.imageView;
    [NamedGuide guideWithName:kTabSwitcherGuide view:tabSwitcherButton]
        .constrainedView = tabSwitcherButton;
  }
}

#pragma mark - Trait Collection Changes

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  if (self.traitCollection.horizontalSizeClass !=
      previousTraitCollection.horizontalSizeClass) {
    [self updateAllButtonsVisibility];
  }
}

#pragma mark - ToolbarWebStateConsumer

- (void)setCanGoForward:(BOOL)canGoForward {
  self.view.forwardButton.enabled = canGoForward;
  // Update the visibility since the Forward button will be hidden on
  // CompactWidth when disabled.
  [self.view.forwardButton updateHiddenInCurrentSizeClass];
}

- (void)setCanGoBack:(BOOL)canGoBack {
  self.view.backButton.enabled = canGoBack;
}

- (void)setLoadingState:(BOOL)loading {
  _loading = loading;
  self.view.reloadButton.hiddenInCurrentState = loading;
  self.view.stopButton.hiddenInCurrentState = !loading;

  if (IsIPadIdiom())
    return;

  if (!loading) {
    [self stopProgressBar];
  } else if (self.view.progressBar.hidden) {
    [self.view.progressBar setProgress:0];
    [self.view.progressBar setHidden:NO animated:YES completion:nil];
    // Layout if needed the progress bar to avoir having the progress bar going
    // backward when opening a page from the NTP.
    [self.view.progressBar layoutIfNeeded];
  }
}

- (void)setIsNTP:(BOOL)isNTP {
  // This boolean is unused in the clean toolbar.
}

- (void)setLoadingProgressFraction:(double)progress {
  [self.view.progressBar setProgress:progress animated:YES completion:nil];
}

- (void)setTabCount:(int)tabCount {
  // Return if tabSwitchStripButton wasn't initialized.
  if (!self.view.tabSwitchStripButton)
    return;

  // Update the text shown in the |self.view.tabSwitchStripButton|. Note that
  // the button's title may be empty or contain an easter egg, but the
  // accessibility value will always be equal to |tabCount|.
  NSString* tabStripButtonValue = [NSString stringWithFormat:@"%d", tabCount];
  NSString* tabStripButtonTitle;
  id<MDCTypographyFontLoading> fontLoader = [MDCTypography fontLoader];
  if (tabCount <= 0) {
    tabStripButtonTitle = @"";
  } else if (tabCount > kShowTabStripButtonMaxTabCount) {
    // As an easter egg, show a smiley face instead of the count if the user has
    // more than 99 tabs open.
    tabStripButtonTitle = @":)";
    [[self.view.tabSwitchStripButton titleLabel]
        setFont:[fontLoader boldFontOfSize:kFontSizeFewerThanTenTabs]];
  } else {
    tabStripButtonTitle = tabStripButtonValue;
    if (tabCount < 10) {
      [[self.view.tabSwitchStripButton titleLabel]
          setFont:[fontLoader boldFontOfSize:kFontSizeFewerThanTenTabs]];
    } else {
      [[self.view.tabSwitchStripButton titleLabel]
          setFont:[fontLoader boldFontOfSize:kFontSizeTenTabsOrMore]];
    }
  }

  // TODO(crbug.com/799601): Delete this once its not needed.
  if (base::FeatureList::IsEnabled(kMemexTabSwitcher)) {
    tabStripButtonTitle = @"M";
    [[self.view.tabSwitchStripButton titleLabel]
        setFont:[fontLoader boldFontOfSize:kFontSizeFewerThanTenTabs]];
  }

  [self.view.tabSwitchStripButton setTitle:tabStripButtonTitle
                                  forState:UIControlStateNormal];
  [self.view.tabSwitchStripButton setAccessibilityValue:tabStripButtonValue];
}

- (void)setPageBookmarked:(BOOL)bookmarked {
  self.view.bookmarkButton.selected = bookmarked;
}

- (void)setVoiceSearchEnabled:(BOOL)voiceSearchEnabled {
  _voiceSearchEnabled = voiceSearchEnabled;
  if (voiceSearchEnabled) {
    self.view.voiceSearchButton.enabled = YES;
    [self.view.voiceSearchButton addTarget:self.dispatcher
                                    action:@selector(preloadVoiceSearch)
                          forControlEvents:UIControlEventTouchDown];
    [self.view.voiceSearchButton addTarget:self
                                    action:@selector(startVoiceSearch:)
                          forControlEvents:UIControlEventTouchUpInside];
  }
}

- (void)setShareMenuEnabled:(BOOL)enabled {
  self.view.shareButton.enabled = enabled;
}

#pragma mark - ToolbarViewFullscreenDelegate

- (void)toolbarViewFrameChanged {
  CGRect frame = self.view.frame;
  CGFloat distanceOffscreen =
      IsIPadIdiom()
          ? fmax((kIPadToolbarY - frame.origin.y) - kScrollFadeDistance, 0)
          : -1 * frame.origin.y;
  CGFloat fraction = 1 - fmin(distanceOffscreen / kScrollFadeDistance, 1);

  self.view.leadingStackView.alpha = fraction;
  self.view.locationBarContainer.alpha = fraction;
  self.view.trailingStackView.alpha = fraction;
}

#pragma mark - ActivityServicePositioner

- (UIView*)shareButtonView {
  return self.view.shareButton;
}

#pragma mark - FullscreenUIElement

- (void)updateForFullscreenProgress:(CGFloat)progress {
  self.view.leadingStackView.alpha = progress;
  self.view.locationBarContainer.alpha = progress;
  self.view.trailingStackView.alpha = progress;
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

#pragma mark - Helper Methods

// Updates all Buttons visibility to match any recent WebState change.
- (void)updateAllButtonsVisibility {
  NSMutableArray* allButtons = [[NSMutableArray alloc] init];
  [allButtons addObjectsFromArray:self.view.leadingStackViewButtons];
  [allButtons addObjectsFromArray:self.view.trailingStackViewButtons];
  [allButtons addObjectsFromArray:@[
    self.view.voiceSearchButton, self.view.bookmarkButton
  ]];
  for (ToolbarButton* button in allButtons) {
    [button updateHiddenInCurrentSizeClass];
  }
}

#pragma mark - Private

// Sets the progress of the progressBar to 1 then hides it.
- (void)stopProgressBar {
  __weak MDCProgressView* weakProgressBar = self.view.progressBar;
  __weak ToolbarViewController* weakSelf = self;
  [self.view.progressBar
      setProgress:1
         animated:YES
       completion:^(BOOL finished) {
         if (!weakSelf.loading) {
           [weakProgressBar setHidden:YES animated:YES completion:nil];
         }
       }];
}

// TODO(crbug.com/789104): Use named layout guide instead of passing the view.
// Target of the voice search button.
- (void)startVoiceSearch:(id)sender {
  UIView* view = base::mac::ObjCCastStrict<UIView>(sender);
  [NamedGuide guideWithName:kVoiceSearchButtonGuide view:view].constrainedView =
      view;
  [self.dispatcher startVoiceSearch];
}

// Sets all Toolbar Buttons opacity to |alpha|.
- (void)setAllToolbarButtonsOpacity:(CGFloat)alpha {
  for (UIButton* button in [self.view.leadingStackViewButtons
           arrayByAddingObjectsFromArray:self.view.trailingStackViewButtons]) {
    button.alpha = alpha;
  }
}

// Offsets the horizontal translation transform of all visible UIViews
// in |array| by |offset|. If the View is hidden it will assign the
// IdentityTransform. Used for fade in animations.
- (void)setHorizontalTranslationOffset:(LayoutOffset)offset
                              forViews:(NSArray<UIView*>*)array {
  for (UIView* view in array) {
    if (!view.hidden)
      view.transform = (offset != 0 && !view.hidden)
                           ? CGAffineTransformMakeTranslation(offset, 0)
                           : CGAffineTransformIdentity;
  }
}

// Returns the leading margin for the leading stack view.
- (CGFloat)leadingMargin {
  return IsIPadIdiom() ? kLeadingMarginIPad : 0;
}

// Registers the actions which will be triggered when tapping the button.
- (void)addStandardActionsForButton:(UIButton*)button {
  if (button != self.view.toolsMenuButton) {
    [button addTarget:self.omniboxFocuser
                  action:@selector(cancelOmniboxEdit)
        forControlEvents:UIControlEventTouchUpInside];
  }
  [button addTarget:self
                action:@selector(recordUserMetrics:)
      forControlEvents:UIControlEventTouchUpInside];
}

// Records the use of a button.
- (IBAction)recordUserMetrics:(id)sender {
  if (sender == self.view.backButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarBack"));
  } else if (sender == self.view.forwardButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarForward"));
  } else if (sender == self.view.reloadButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarReload"));
  } else if (sender == self.view.stopButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarStop"));
  } else if (sender == self.view.voiceSearchButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarVoiceSearch"));
  } else if (sender == self.view.bookmarkButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarToggleBookmark"));
  } else if (sender == self.view.toolsMenuButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarShowMenu"));
  } else if (sender == self.view.tabSwitchStripButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarShowStackView"));
  } else if (sender == self.view.shareButton) {
    base::RecordAction(base::UserMetricsAction("MobileToolbarShareMenu"));
  } else {
    NOTREACHED();
  }
}
@end
