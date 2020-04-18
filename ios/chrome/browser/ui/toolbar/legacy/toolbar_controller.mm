// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller.h"

#include <QuartzCore/QuartzCore.h>

#include "base/format_macros.h"
#include "base/i18n/rtl.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#import "ios/chrome/browser/ui/animation_util.h"
#include "ios/chrome/browser/ui/bubble/bubble_util.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_animator.h"
#import "ios/chrome/browser/ui/image_util/image_util.h"
#import "ios/chrome/browser/ui/reversed_animation.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tools_menu_button.h"
#import "ios/chrome/browser/ui/toolbar/buttons/tools_menu_button_observer_bridge.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller+protected.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_utils.h"
#include "ios/chrome/browser/ui/toolbar/toolbar_resource_macros.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;
using ios::material::TimingFunction;

// Helper class to display a UIButton with the image and text centered
// vertically and horizontally.
@interface ToolbarCenteredButton : UIButton {
}
@end

@implementation ToolbarCenteredButton

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.titleLabel.textAlignment = NSTextAlignmentCenter;
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  CGSize size = self.bounds.size;
  CGPoint center = CGPointMake(size.width / 2, size.height / 2);
  self.imageView.center = center;
  self.imageView.frame = AlignRectToPixel(self.imageView.frame);
  self.titleLabel.frame = self.bounds;
}

@end

@interface ToolbarController () {
  // The shadow view. Only used on iPhone.
  UIImageView* fullBleedShadowView_;

  ToolbarToolsMenuButton* toolsMenuButton_;
  UIButton* stackButton_;
  UIButton* shareButton_;
  NSArray* standardButtons_;
  ToolsMenuButtonObserverBridge* toolsMenuButtonObserverBridge_;
  ToolbarControllerStyle style_;
}

// Leading and trailing safe area constraint for faking a safe area. These
// constraints are activated by calling activateFakeSafeAreaInsets and
// deactivateFakeSafeAreaInsets.
@property(nonatomic, strong) NSLayoutConstraint* leadingFakeSafeAreaConstraint;
@property(nonatomic, strong) NSLayoutConstraint* trailingFakeSafeAreaConstraint;

// These constraints pin the content view to the safe area. They are temporarily
// disabled when a fake safe area is simulated by calling
// activateFakeSafeAreaInsets.
@property(nonatomic, strong) NSLayoutConstraint* leadingSafeAreaConstraint;
@property(nonatomic, strong) NSLayoutConstraint* trailingSafeAreaConstraint;
// Style of this toolbar.
@property(nonatomic, readonly, assign) ToolbarControllerStyle style;
// The view containing all the content of the toolbar. It respects the trailing
// and leading anchors of the safe area.
@property(nonatomic, readonly, strong) UIView* contentView;

// Returns the background image that should be used for |style|.
- (UIImage*)getBackgroundImageForStyle:(ToolbarControllerStyle)style;

// Whether the share button should be visible in the toolbar.
- (BOOL)shareButtonShouldBeVisible;

@end

@implementation ToolbarController

@synthesize readingListModel = readingListModel_;
@synthesize contentView = contentView_;
@synthesize backgroundView = backgroundView_;
@synthesize shadowView = shadowView_;
@synthesize style = style_;
@synthesize heightConstraint = heightConstraint_;
@synthesize dispatcher = dispatcher_;
@synthesize leadingFakeSafeAreaConstraint = _leadingFakeSafeAreaConstraint;
@synthesize trailingFakeSafeAreaConstraint = _trailingFakeSafeAreaConstraint;
@synthesize leadingSafeAreaConstraint = _leadingSafeAreaConstraint;
@synthesize trailingSafeAreaConstraint = _trailingSafeAreaConstraint;
@dynamic view;

- (instancetype)initWithStyle:(ToolbarControllerStyle)style
                   dispatcher:(id<ApplicationCommands,
                                  BrowserCommands,
                                  OmniboxFocuser,
                                  ToolbarCommands>)dispatcher {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    style_ = style;
    dispatcher_ = dispatcher;
    DCHECK_LT(style_, ToolbarControllerStyleMaxStyles);

    InterfaceIdiom idiom = IsIPadIdiom() ? IPAD_IDIOM : IPHONE_IDIOM;
    CGRect viewFrame = kToolbarFrame[idiom];
    CGRect backgroundFrame = kBackgroundViewFrame[idiom];
    CGRect stackButtonFrame = LayoutRectGetRect(kStackButtonFrame);
    CGRect toolsMenuButtonFrame =
        LayoutRectGetRect(kToolsMenuButtonFrame[idiom]);

    if (idiom == IPHONE_IDIOM) {
      CGFloat statusBarOffset = [self statusBarOffset];
      viewFrame.size.height += statusBarOffset;
      backgroundFrame.size.height += statusBarOffset;
      stackButtonFrame.origin.y += statusBarOffset;
      toolsMenuButtonFrame.origin.y += statusBarOffset;
    }

    self.view = [[LegacyToolbarView alloc] initWithFrame:viewFrame];
    [self.view setTranslatesAutoresizingMaskIntoConstraints:NO];

    UIViewAutoresizing autoresizingMask =
        UIViewAutoresizingFlexibleLeadingMargin() |
        UIViewAutoresizingFlexibleTopMargin;

    backgroundView_ = [[UIImageView alloc] initWithFrame:backgroundFrame];

    [self.view addSubview:backgroundView_];
    [self.view setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
    [backgroundView_ setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                         UIViewAutoresizingFlexibleHeight];

    contentView_ = [[UIView alloc] initWithFrame:viewFrame];
    contentView_.translatesAutoresizingMaskIntoConstraints = NO;
    contentView_.layer.allowsGroupOpacity = YES;
    [self.view addSubview:contentView_];
    NSLayoutConstraint* safeAreaLeading = nil;
    NSLayoutConstraint* safeAreaTrailing = nil;
    if (@available(iOS 11.0, *)) {
      UILayoutGuide* safeArea = self.view.safeAreaLayoutGuide;
      safeAreaLeading = [contentView_.leadingAnchor
          constraintEqualToAnchor:safeArea.leadingAnchor];
      safeAreaTrailing = [contentView_.trailingAnchor
          constraintEqualToAnchor:safeArea.trailingAnchor];
    } else {
      safeAreaLeading = [contentView_.leadingAnchor
          constraintEqualToAnchor:self.view.leadingAnchor];
      safeAreaTrailing = [contentView_.trailingAnchor
          constraintEqualToAnchor:self.view.trailingAnchor];
    }
    _leadingSafeAreaConstraint = safeAreaLeading;
    _trailingSafeAreaConstraint = safeAreaTrailing;
    _leadingFakeSafeAreaConstraint = [contentView_.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor];
    _trailingFakeSafeAreaConstraint = [contentView_.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor];
    [NSLayoutConstraint activateConstraints:@[
      safeAreaLeading,
      safeAreaTrailing,
      [contentView_.topAnchor constraintEqualToAnchor:self.view.topAnchor],
      [contentView_.bottomAnchor
          constraintEqualToAnchor:self.view.bottomAnchor],
    ]];

    toolsMenuButton_ =
        [[ToolbarToolsMenuButton alloc] initWithFrame:toolsMenuButtonFrame
                                                style:style_];
    [toolsMenuButton_ addTarget:self.dispatcher
                         action:@selector(showToolsMenu)
               forControlEvents:UIControlEventTouchUpInside];
    [toolsMenuButton_ setAutoresizingMask:autoresizingMask];
    [contentView_ addSubview:toolsMenuButton_];

    if (idiom == IPAD_IDIOM) {
      CGRect shareButtonFrame = LayoutRectGetRect(kShareMenuButtonFrame);
      shareButton_ = [[UIButton alloc] initWithFrame:shareButtonFrame];
      [shareButton_ setAutoresizingMask:autoresizingMask];
      [self setUpButton:shareButton_
             withImageEnum:ToolbarButtonNameShare
           forInitialState:UIControlStateNormal
          hasDisabledImage:YES
             synchronously:NO];
      [shareButton_ addTarget:self.dispatcher
                       action:@selector(sharePage)
             forControlEvents:UIControlEventTouchUpInside];
      SetA11yLabelAndUiAutomationName(shareButton_, IDS_IOS_TOOLS_MENU_SHARE,
                                      kToolbarShareButtonIdentifier);
      [contentView_ addSubview:shareButton_];
    }

    CGRect shadowFrame = kShadowViewFrame[idiom];
    shadowFrame.origin.y = CGRectGetMaxY(backgroundFrame);
    shadowView_ = [[UIImageView alloc] initWithFrame:shadowFrame];
    [shadowView_ setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                     UIViewAutoresizingFlexibleTopMargin];

    [shadowView_ setUserInteractionEnabled:NO];
    [self.view addSubview:shadowView_];
    [shadowView_ setImage:NativeImage(IDR_IOS_TOOLBAR_SHADOW)];

    if (idiom == IPHONE_IDIOM) {
      // iPad omnibox does not expand to full bleed.
      CGRect fullBleedShadowFrame = kFullBleedShadowViewFrame;
      fullBleedShadowFrame.origin.y = shadowFrame.origin.y;
      fullBleedShadowView_ =
          [[UIImageView alloc] initWithFrame:fullBleedShadowFrame];
      [fullBleedShadowView_
          setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                              UIViewAutoresizingFlexibleTopMargin];

      [fullBleedShadowView_ setUserInteractionEnabled:NO];
      [fullBleedShadowView_ setAlpha:0];
      [self.view addSubview:fullBleedShadowView_];
      [fullBleedShadowView_
          setImage:NativeImage(IDR_IOS_TOOLBAR_SHADOW_FULL_BLEED)];
    }

    // UIImageViews do not default to userInteractionEnabled:YES.
    [self.view setUserInteractionEnabled:YES];
    [backgroundView_ setUserInteractionEnabled:YES];

    UIImage* tile = [self getBackgroundImageForStyle:style];
    [[self backgroundView]
        setImage:StretchableImageFromUIImage(tile, 0.0, 3.0)];

    if (idiom == IPHONE_IDIOM) {
      stackButton_ =
          [[ToolbarCenteredButton alloc] initWithFrame:stackButtonFrame];
      [[stackButton_ titleLabel]
          setFont:[self fontForSize:kFontSizeFewerThanTenTabs]];
      [stackButton_
          setTitleColor:[UIColor colorWithWhite:kStackButtonNormalColors[style_]
                                          alpha:1.0]
               forState:UIControlStateNormal];
      UIColor* highlightColor =
          UIColorFromRGB(kStackButtonHighlightedColors[style_], 1.0);
      [stackButton_ setTitleColor:highlightColor
                         forState:UIControlStateHighlighted];

      [stackButton_ setAutoresizingMask:autoresizingMask];

      [self setUpButton:stackButton_
             withImageEnum:ToolbarButtonNameStack
           forInitialState:UIControlStateNormal
          hasDisabledImage:NO
             synchronously:NO];
      [contentView_ addSubview:stackButton_];
    }
    [self registerEventsForButton:toolsMenuButton_];

    SetA11yLabelAndUiAutomationName(stackButton_, IDS_IOS_TOOLBAR_SHOW_TABS,
                                    kToolbarStackButtonIdentifier);
    SetA11yLabelAndUiAutomationName(toolsMenuButton_, IDS_IOS_TOOLBAR_SETTINGS,
                                    kToolbarToolsMenuButtonIdentifier);
    [self updateStandardButtons];
  }
  return self;
}

#pragma mark - Public API

- (void)setReadingListModel:(ReadingListModel*)readingListModel {
  readingListModel_ = readingListModel;
  if (readingListModel_) {
    toolsMenuButtonObserverBridge_ =
        [[ToolsMenuButtonObserverBridge alloc] initWithModel:readingListModel_
                                               toolbarButton:toolsMenuButton_];
  }
}

- (void)activateFakeSafeAreaInsets:(UIEdgeInsets)fakeSafeAreaInsets {
  self.leadingFakeSafeAreaConstraint.constant =
      UIEdgeInsetsGetLeading(fakeSafeAreaInsets);
  self.trailingFakeSafeAreaConstraint.constant =
      -UIEdgeInsetsGetTrailing(fakeSafeAreaInsets);
  self.leadingSafeAreaConstraint.active = NO;
  self.trailingSafeAreaConstraint.active = NO;
  self.leadingFakeSafeAreaConstraint.active = YES;
  self.trailingFakeSafeAreaConstraint.active = YES;
}

- (void)deactivateFakeSafeAreaInsets {
  self.leadingFakeSafeAreaConstraint.active = NO;
  self.trailingFakeSafeAreaConstraint.active = NO;
  self.leadingSafeAreaConstraint.active = YES;
  self.trailingSafeAreaConstraint.active = YES;
}

- (void)setToolsMenuIsVisibleForToolsMenuButton:(BOOL)isVisible {
  [toolsMenuButton_ setToolsMenuIsVisible:isVisible];
}

#pragma mark Appearance

- (void)setBackgroundAlpha:(CGFloat)alpha {
  [backgroundView_ setAlpha:alpha];
  [shadowView_ setAlpha:alpha];
}

- (void)setTabCount:(NSInteger)tabCount {
  if (!stackButton_)
    return;
  // Enable or disable the stack view icon based on the number of tabs. This
  // locks the user in the stack view when there are no tabs.
  [stackButton_ setEnabled:tabCount > 0 ? YES : NO];

  // Update the text shown in the |stackButton_|. Note that the button's title
  // may be empty or contain an easter egg, but the accessibility value will
  // always be equal to |tabCount|. Also, the text of |stackButton_| is shifted
  // up, via |kEasterEggTitleInsets|, to avoid overlapping with the button's
  // outline.
  NSString* stackButtonValue =
      [NSString stringWithFormat:@"%" PRIdNS, tabCount];
  NSString* stackButtonTitle;
  if (tabCount <= 0) {
    stackButtonTitle = @"";
  } else if (tabCount > kStackButtonMaxTabCount) {
    stackButtonTitle = @":)";
    [[stackButton_ titleLabel]
        setFont:[self fontForSize:kFontSizeFewerThanTenTabs]];
  } else {
    stackButtonTitle = stackButtonValue;
    if (tabCount < 10) {
      [[stackButton_ titleLabel]
          setFont:[self fontForSize:kFontSizeFewerThanTenTabs]];
    } else {
      [[stackButton_ titleLabel]
          setFont:[self fontForSize:kFontSizeTenTabsOrMore]];
    }
  }

  [stackButton_ setTitle:stackButtonTitle forState:UIControlStateNormal];
  [stackButton_ setAccessibilityValue:stackButtonValue];
}

- (void)setUpButton:(UIButton*)button
       withImageEnum:(int)imageEnum
     forInitialState:(UIControlState)initialState
    hasDisabledImage:(BOOL)hasDisabledImage
       synchronously:(BOOL)synchronously {
  [self registerEventsForButton:button];
  // Add the non-initial images after a slight delay, to help performance
  // and responsiveness on startup.
  dispatch_time_t addImageDelay =
      dispatch_time(DISPATCH_TIME_NOW, kNonInitialImageAdditionDelayNanosec);

  void (^normalImageBlock)(void) = ^{
    UIImage* image =
        [self imageForImageEnum:imageEnum forState:ToolbarButtonUIStateNormal];
    [button setImage:image forState:UIControlStateNormal];
  };
  if (synchronously || initialState == UIControlStateNormal)
    normalImageBlock();
  else
    dispatch_after(addImageDelay, dispatch_get_main_queue(), normalImageBlock);

  void (^pressedImageBlock)(void) = ^{
    UIImage* image =
        [self imageForImageEnum:imageEnum forState:ToolbarButtonUIStatePressed];
    [button setImage:image forState:UIControlStateHighlighted];
  };
  if (synchronously || initialState == UIControlStateHighlighted)
    pressedImageBlock();
  else
    dispatch_after(addImageDelay, dispatch_get_main_queue(), pressedImageBlock);

  if (hasDisabledImage) {
    void (^disabledImageBlock)(void) = ^{
      UIImage* image = [self imageForImageEnum:imageEnum
                                      forState:ToolbarButtonUIStateDisabled];
      [button setImage:image forState:UIControlStateDisabled];
    };
    if (synchronously || initialState == UIControlStateDisabled) {
      disabledImageBlock();
    } else {
      dispatch_after(addImageDelay, dispatch_get_main_queue(),
                     disabledImageBlock);
    }
  }
}

- (BOOL)imageShouldFlipForRightToLeftLayoutDirection:(int)imageEnum {
  // None of the images this class knows about should flip.
  return NO;
}

- (void)hideViewsForNewTabPage:(BOOL)hide {
  DCHECK(!IsIPadIdiom());
  [shadowView_ setHidden:hide];
}

- (NSLayoutConstraint*)heightConstraint {
  if (!heightConstraint_) {
    heightConstraint_ = [self.view.heightAnchor constraintEqualToConstant:0];
  }
  return heightConstraint_;
}

#pragma mark Animations

- (void)triggerToolsMenuButtonAnimation {
  [toolsMenuButton_ triggerAnimation];
}

#pragma mark - Protected API

- (IBAction)recordUserMetrics:(id)sender {
  if (sender == toolsMenuButton_)
    base::RecordAction(UserMetricsAction("MobileToolbarShowMenu"));
  else if (sender == stackButton_)
    base::RecordAction(UserMetricsAction("MobileToolbarShowStackView"));
  else if (sender == shareButton_)
    base::RecordAction(UserMetricsAction("MobileToolbarShareMenu"));
  else
    NOTREACHED();
}

- (CGFloat)statusBarOffset {
  return StatusBarHeight();
}

- (UIButton*)stackButton {
  return stackButton_;
}

- (CGRect)specificControlsArea {
  // Return the rect to the leading side of the leading-most trailing control.
  UIView* trailingControl = toolsMenuButton_;
  if (!IsIPadIdiom())
    trailingControl = stackButton_;
  if ([self shareButtonShouldBeVisible])
    trailingControl = shareButton_;
  LayoutRect trailing = LayoutRectForRectInBoundingRect(
      trailingControl.frame, self.contentView.bounds);
  LayoutRect controlsArea = LayoutRectGetLeadingLayout(trailing);
  controlsArea.size.height = self.contentView.bounds.size.height;
  controlsArea.position.originY = self.contentView.bounds.origin.y;
  CGRect controlsFrame = LayoutRectGetRect(controlsArea);

  if (!IsIPadIdiom()) {
    controlsFrame.origin.y += StatusBarHeight();
    controlsFrame.size.height -= StatusBarHeight();
  }
  return controlsFrame;
}

- (int)imageEnumForButton:(UIButton*)button {
  if (button == stackButton_)
    return ToolbarButtonNameStack;
  return NumberOfToolbarButtonNames;
}

- (int)imageIdForImageEnum:(int)index
                     style:(ToolbarControllerStyle)style
                  forState:(ToolbarButtonUIState)state {
  DCHECK(index < NumberOfToolbarButtonNames);
  DCHECK(style < ToolbarControllerStyleMaxStyles);
  DCHECK(state < NumberOfToolbarButtonUIStates);
  // Incognito mode gets dark buttons.
  if (style == ToolbarControllerStyleIncognitoMode)
    style = ToolbarControllerStyleDarkMode;

  // Name, style [light, dark], UIControlState [normal, pressed, disabled]
  static int buttonImageIds[NumberOfToolbarButtonNames][2]
                           [NumberOfToolbarButtonUIStates] = {
                               TOOLBAR_IDR_THREE_STATE(OVERVIEW),
                               TOOLBAR_IDR_THREE_STATE(SHARE),
                           };

  DCHECK(buttonImageIds[index][style][state]);
  return buttonImageIds[index][style][state];
}

#pragma mark - ToolsMenuPresentationProvider

- (UIButton*)presentingButtonForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator {
  return toolsMenuButton_;
}

#pragma mark - Private Methods
#pragma mark Animations

// Add position and opacity animations to |view|'s layer. The opacity
// animation goes from 0 to 1. The position animation goes from
// [view.layer.position offset in the leading direction by |leadingOffset|)
// to view.layer.position. Both animations occur after |delay| seconds.
- (void)fadeInView:(UIView*)view
    fromLeadingOffset:(LayoutOffset)leadingOffset
         withDuration:(NSTimeInterval)duration
           afterDelay:(NSTimeInterval)delay {
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  [CATransaction setCompletionBlock:^{
    [view.layer removeAnimationForKey:@"fadeIn"];
  }];
  view.alpha = 1.0;

  // Animate the position of |view| |leadingOffset| pixels after |delay|.
  CGRect shiftedFrame = CGRectLayoutOffset(view.frame, leadingOffset);
  CAAnimation* shiftAnimation =
      FrameAnimationMake(view.layer, shiftedFrame, view.frame);
  shiftAnimation.duration = duration;
  shiftAnimation.beginTime = delay;
  shiftAnimation.timingFunction = TimingFunction(ios::material::CurveEaseInOut);

  // Animate the opacity of |view| to 1 after |delay|.
  CAAnimation* fadeAnimation = OpacityAnimationMake(0.0, 1.0);
  fadeAnimation.duration = duration;
  fadeAnimation.beginTime = delay;
  shiftAnimation.timingFunction = TimingFunction(ios::material::CurveEaseInOut);

  // Add group animation to layer.
  CAAnimation* group = AnimationGroupMake(@[ shiftAnimation, fadeAnimation ]);
  [view.layer addAnimation:group forKey:@"fadeIn"];

  [CATransaction commit];
}

- (void)fadeOutStandardControls {
  // The opacity animation has a different duration from the position animation.
  // Thus they require separate CATransations.

  // Animate the opacity of the buttons to 0.
  [CATransaction begin];
  [CATransaction setAnimationDuration:ios::material::kDuration2];
  [CATransaction
      setAnimationTimingFunction:TimingFunction(ios::material::CurveEaseIn)];
  CABasicAnimation* fadeButtons =
      [CABasicAnimation animationWithKeyPath:@"opacity"];
  fadeButtons.fromValue = @1;
  fadeButtons.toValue = @0;

  for (UIButton* button in standardButtons_) {
    if (![button isHidden]) {
      [button layer].opacity = 0;
      [[button layer] addAnimation:fadeButtons forKey:@"fade"];
    }
  }
  [CATransaction commit];

  // Animate the buttons 10 pixels in the leading-to-trailing direction
  [CATransaction begin];
  [CATransaction setAnimationDuration:ios::material::kDuration1];
  [CATransaction
      setAnimationTimingFunction:TimingFunction(ios::material::CurveEaseIn)];

  for (UIButton* button in standardButtons_) {
    CABasicAnimation* shiftButton =
        [CABasicAnimation animationWithKeyPath:@"position"];
    CGPoint startPosition = [button layer].position;
    CGPoint endPosition =
        CGPointLayoutOffset(startPosition, kButtonFadeOutXOffset);
    shiftButton.fromValue = [NSValue valueWithCGPoint:startPosition];
    shiftButton.toValue = [NSValue valueWithCGPoint:endPosition];
    [[button layer] addAnimation:shiftButton forKey:@"shiftButton"];
  }

  [CATransaction commit];

  // Fade to the full bleed shadow.
  [UIView animateWithDuration:ios::material::kDuration1
                   animations:^{
                     [shadowView_ setAlpha:0];
                     [fullBleedShadowView_ setAlpha:1];
                   }];
}

- (void)fadeInStandardControls {
  for (UIButton* button in standardButtons_) {
    [self fadeInView:button
        fromLeadingOffset:10
             withDuration:ios::material::kDuration2
               afterDelay:ios::material::kDuration1];
  }

  // Fade to the normal shadow.
  [UIView animateWithDuration:ios::material::kDuration1
                   animations:^{
                     [shadowView_ setAlpha:self.backgroundView.alpha];
                     [fullBleedShadowView_ setAlpha:0];
                   }];
}

#pragma mark Helpers

// Returns the UIImage from the resources bundle for the |imageEnum| and
// |state|.  Uses the toolbar's current style.
- (UIImage*)imageForImageEnum:(int)imageEnum
                     forState:(ToolbarButtonUIState)state {
  int imageID =
      [self imageIdForImageEnum:imageEnum style:[self style] forState:state];
  return NativeReversableImage(
      imageID, [self imageShouldFlipForRightToLeftLayoutDirection:imageEnum]);
}

// Called whenever one of the standard controls is triggered. Does nothing,
// but can be overridden by subclasses to clear any state (e.g., close menus).
- (void)standardButtonPressed:(UIButton*)sender {
  // This check for valid button images assumes that the buttons all have a
  // different image for the highlighted state as for the normal state.
  // Currently, that assumption is true.
  if ([sender imageForState:UIControlStateHighlighted] ==
      [sender imageForState:UIControlStateNormal]) {
    // Update the button images synchronously - somehow the button was pressed
    // before the dispatched task completed.
    [self setUpButton:sender
           withImageEnum:[self imageEnumForButton:sender]
         forInitialState:UIControlStateNormal
        hasDisabledImage:NO
           synchronously:YES];
  }
}

// Update share button visibility and |standardButtons_| array.
- (void)updateStandardButtons {
  BOOL shareButtonShouldBeVisible = [self shareButtonShouldBeVisible];
  [shareButton_ setHidden:!shareButtonShouldBeVisible];
  NSMutableArray* standardButtons = [NSMutableArray array];
  [standardButtons addObject:toolsMenuButton_];
  if (stackButton_)
    [standardButtons addObject:stackButton_];
  if (shareButtonShouldBeVisible)
    [standardButtons addObject:shareButton_];
  standardButtons_ = standardButtons;
}

- (UIFont*)fontForSize:(NSInteger)size {
  return [[MDCTypography fontLoader] boldFontOfSize:size];
}

- (BOOL)shareButtonShouldBeVisible {
  // The share button only exists on iPad, and when some tabs are visible
  // (i.e. when not in DarkMode), and when the width is greater than
  // the tablet mini view.
  if (!IsIPadIdiom() || style_ == ToolbarControllerStyleDarkMode ||
      IsCompactTablet(self.view))
    return NO;
  return YES;
}

- (void)registerEventsForButton:(UIButton*)button {
  if (button != toolsMenuButton_) {
    // |target| must be |self| (as opposed to |nil|) because |self| isn't in the
    // responder chain.
    [button addTarget:self
                  action:@selector(standardButtonPressed:)
        forControlEvents:UIControlEventTouchUpInside];
  }
  [button addTarget:self
                action:@selector(recordUserMetrics:)
      forControlEvents:UIControlEventTouchUpInside];
}

- (UIImage*)getBackgroundImageForStyle:(ToolbarControllerStyle)style {
  int backgroundImageID;
  if (style == ToolbarControllerStyleLightMode)
    backgroundImageID = IDR_IOS_TOOLBAR_LIGHT_BACKGROUND;
  else
    backgroundImageID = IDR_IOS_TOOLBAR_DARK_BACKGROUND;

  return NativeImage(backgroundImageID);
}

#pragma mark - ActivityServicePositioner

- (UIView*)shareButtonView {
  return shareButton_;
}

#pragma mark - FullscreenUIElement

- (void)updateForFullscreenProgress:(CGFloat)progress {
  self.contentView.alpha = progress;
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

#pragma mark - CAAnimationDelegate
// WebToolbarController conforms to CAAnimationDelegate.
- (void)animationDidStart:(CAAnimation*)anim {
  // Once the buttons start fading in, set their opacity to 1 so there's no
  // flicker at the end of the animation.
  for (UIButton* button in standardButtons_) {
    if (anim == [[button layer] animationForKey:@"fadeIn"]) {
      [button layer].opacity = 1;
      return;
    }
  }
}

@end
