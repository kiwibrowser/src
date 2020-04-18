// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_view.h"

#import <UIKit/UIKit.h>

#include "base/logging.h"
#import "ios/chrome/browser/ui/UIView+SizeClassSupport.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_utils.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_header_constants.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_configuration.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_constants.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_snapshot_providing.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/browser/ui/util/named_guide.h"
#import "ios/chrome/browser/ui/util/named_guide_util.h"
#import "ui/gfx/ios/NSString+CrStringDrawing.h"
#import "ui/gfx/ios/uikit_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Left margin for search icon.
const CGFloat kSearchIconLeftMargin = 9;

// Landscape inset for fake omnibox background container
const CGFloat kBackgroundLandscapeInset = 169;

}  // namespace

@interface ContentSuggestionsHeaderView ()<ToolbarSnapshotProviding>

// Layout constraints for fake omnibox background image and blur.
@property(nonatomic, strong) NSLayoutConstraint* backgroundHeightConstraint;
@property(nonatomic, strong) NSLayoutConstraint* backgroundLeadingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* backgroundTrailingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* blurTopConstraint;

@end

@implementation ContentSuggestionsHeaderView

@synthesize backgroundHeightConstraint = _backgroundHeightConstraint;
@synthesize backgroundLeadingConstraint = _backgroundLeadingConstraint;
@synthesize backgroundTrailingConstraint = _backgroundTrailingConstraint;
@synthesize blurTopConstraint = _blurTopConstraint;
@synthesize toolBarView = _toolBarView;

#pragma mark - Public

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.clipsToBounds = YES;
  }
  return self;
}

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  self.toolBarView.hidden = IsRegularXRegularSizeClass(self);
}

#pragma mark - Private

// Scale the the hint label down to at most content_suggestions::kHintTextScale.
// Also maintains autoresizing frame origin after the transform.
- (void)scaleHintLabel:(UIView*)hintLabel percent:(CGFloat)percent {
  CGFloat scaleValue = (content_suggestions::kHintTextScale - 1) * percent + 1;
  hintLabel.transform = CGAffineTransformMakeScale(scaleValue, scaleValue);
  // The transform above is anchored around the center of the frame, which means
  // the origin x and y value will be updated as well as it's width and height.
  // Since the source of truth for this views layout is governed by it's parent
  // view in autolayout, reset the frame's origin.x to 0 below.
  CGRect frame = hintLabel.frame;
  frame.origin.x = 0;
  hintLabel.frame = frame;
}

#pragma mark - NTPHeaderViewAdapter

- (void)addToolbarView:(UIView*)toolbarView {
  DCHECK(!self.toolBarView);
  _toolBarView = toolbarView;
  [self addSubview:self.toolBarView];
  // TODO(crbug.com/808431) This logic is duplicated in various places and
  // should be refactored away for content suggestions.
  NSArray<GuideName*>* guideNames = @[
    kOmniboxGuide,
    kBackButtonGuide,
    kForwardButtonGuide,
    kToolsMenuGuide,
    kTabSwitcherGuide,
  ];
  AddNamedGuidesToView(guideNames, self);
  id<LayoutGuideProvider> layoutGuide = SafeAreaLayoutGuideForView(self);
  [NSLayoutConstraint activateConstraints:@[
    [self.toolBarView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [self.toolBarView.topAnchor constraintEqualToAnchor:layoutGuide.topAnchor],
    [self.toolBarView.trailingAnchor
        constraintEqualToAnchor:self.trailingAnchor],
  ]];
}

- (void)addViewsToSearchField:(UIView*)searchField {
  ToolbarButtonFactory* buttonFactory =
      [[ToolbarButtonFactory alloc] initWithStyle:NORMAL];
  UIBlurEffect* blurEffect = buttonFactory.toolbarConfiguration.blurEffect;
  UIView* blur = nil;
  if (blurEffect) {
    blur = [[UIVisualEffectView alloc] initWithEffect:blurEffect];
  } else {
    blur = [[UIView alloc] init];
  }
  blur.backgroundColor = buttonFactory.toolbarConfiguration.blurBackgroundColor;
  blur.layer.cornerRadius = kAdaptiveLocationBarCornerRadius;
  [searchField insertSubview:blur atIndex:0];
  blur.translatesAutoresizingMaskIntoConstraints = NO;
  self.blurTopConstraint =
      [blur.topAnchor constraintEqualToAnchor:searchField.topAnchor];
  [NSLayoutConstraint activateConstraints:@[
    [blur.leadingAnchor constraintEqualToAnchor:searchField.leadingAnchor],
    [blur.trailingAnchor constraintEqualToAnchor:searchField.trailingAnchor],
    self.blurTopConstraint,
    [blur.bottomAnchor constraintEqualToAnchor:searchField.bottomAnchor]
  ]];

  UIVisualEffect* vibrancy = [buttonFactory.toolbarConfiguration
      vibrancyEffectForBlurEffect:blurEffect];
  UIVisualEffectView* vibrancyView =
      [[UIVisualEffectView alloc] initWithEffect:vibrancy];
  [searchField insertSubview:vibrancyView atIndex:1];
  vibrancyView.translatesAutoresizingMaskIntoConstraints = NO;
  AddSameConstraints(vibrancyView, searchField);

  UIView* backgroundContainer = [[UIView alloc] init];
  backgroundContainer.userInteractionEnabled = NO;
  backgroundContainer.backgroundColor =
      UIColorFromRGB(content_suggestions::kSearchFieldBackgroundColor);
  backgroundContainer.layer.cornerRadius = kAdaptiveLocationBarCornerRadius;
  [vibrancyView.contentView addSubview:backgroundContainer];

  backgroundContainer.translatesAutoresizingMaskIntoConstraints = NO;
  self.backgroundLeadingConstraint = [backgroundContainer.leadingAnchor
      constraintEqualToAnchor:searchField.leadingAnchor];
  self.backgroundTrailingConstraint = [backgroundContainer.trailingAnchor
      constraintEqualToAnchor:searchField.trailingAnchor];
  self.backgroundHeightConstraint = [backgroundContainer.heightAnchor
      constraintEqualToConstant:content_suggestions::kSearchFieldHeight];
  [NSLayoutConstraint activateConstraints:@[
    [backgroundContainer.centerYAnchor
        constraintEqualToAnchor:searchField.centerYAnchor],
    self.backgroundLeadingConstraint,
    self.backgroundTrailingConstraint,
    self.backgroundHeightConstraint,
  ]];

  UIImage* search_icon = [UIImage imageNamed:@"ntp_search_icon"];
  UIImageView* search_view = [[UIImageView alloc] initWithImage:search_icon];
  [searchField addSubview:search_view];
  search_view.translatesAutoresizingMaskIntoConstraints = NO;
  [NSLayoutConstraint activateConstraints:@[
    [search_view.centerYAnchor
        constraintEqualToAnchor:backgroundContainer.centerYAnchor],
    [search_view.leadingAnchor
        constraintEqualToAnchor:backgroundContainer.leadingAnchor
                       constant:kSearchIconLeftMargin],
  ]];
}

- (CGFloat)searchFieldProgressForOffset:(CGFloat)offset
                         safeAreaInsets:(UIEdgeInsets)safeAreaInsets {
  // The scroll offset at which point searchField's frame should stop growing.
  CGFloat maxScaleOffset = self.frame.size.height -
                           ntp_header::kMinHeaderHeight - safeAreaInsets.top;
  // The scroll offset at which point searchField's frame should start
  // growing.
  CGFloat startScaleOffset = maxScaleOffset - ntp_header::kAnimationDistance;
  CGFloat percent = 0;
  if (offset && offset > startScaleOffset) {
    CGFloat animatingOffset = offset - startScaleOffset;
    percent = MIN(1, MAX(0, animatingOffset / ntp_header::kAnimationDistance));
  }
  return percent;
}

- (void)updateSearchFieldWidth:(NSLayoutConstraint*)widthConstraint
                        height:(NSLayoutConstraint*)heightConstraint
                     topMargin:(NSLayoutConstraint*)topMarginConstraint
                     hintLabel:(UILabel*)hintLabel
            subviewConstraints:(NSArray*)constraints
                     forOffset:(CGFloat)offset
                   screenWidth:(CGFloat)screenWidth
                safeAreaInsets:(UIEdgeInsets)safeAreaInsets {
  CGFloat contentWidth = std::max<CGFloat>(
      0, screenWidth - safeAreaInsets.left - safeAreaInsets.right);
  if (screenWidth == 0 || contentWidth == 0)
    return;

  CGFloat searchFieldNormalWidth =
      content_suggestions::searchFieldWidth(contentWidth);

  CGFloat percent =
      [self searchFieldProgressForOffset:offset safeAreaInsets:safeAreaInsets];
  if (IsRegularXRegularSizeClass(self)) {
    self.alpha = 1 - percent;
    widthConstraint.constant = searchFieldNormalWidth;
    self.backgroundHeightConstraint.constant =
        content_suggestions::kSearchFieldHeight;
    [self scaleHintLabel:hintLabel percent:percent];
    self.blurTopConstraint.constant = 0;
    return;
  } else {
    self.alpha = 1;
  }

  // Grow the blur to cover the safeArea top.
  self.blurTopConstraint.constant = -safeAreaInsets.top * percent;

  // Calculate the amount to grow the width and height of searchField so that
  // its frame covers the entire toolbar area.
  CGFloat maxXInset =
      ui::AlignValueToUpperPixel((searchFieldNormalWidth - screenWidth) / 2);
  CGFloat maxHeightDiff =
      ntp_header::ToolbarHeight() - content_suggestions::kSearchFieldHeight;

  widthConstraint.constant = searchFieldNormalWidth - 2 * maxXInset * percent;
  topMarginConstraint.constant = -content_suggestions::searchFieldTopMargin() -
                                 ntp_header::kMaxTopMarginDiff * percent;
  heightConstraint.constant =
      content_suggestions::kSearchFieldHeight + maxHeightDiff * percent;

  // Calculate the amount to shrink the width and height of background so that
  // it's where the focused adapative toolbar focuses.
  CGFloat inset = IsLandscape() ? kBackgroundLandscapeInset : 0;
  self.backgroundLeadingConstraint.constant =
      (safeAreaInsets.left + kExpandedLocationBarHorizontalMargin + inset) *
      percent;
  self.backgroundTrailingConstraint.constant =
      -(safeAreaInsets.right + kExpandedLocationBarHorizontalMargin + inset) *
      percent;

  CGFloat kLocationBarHeight =
      kAdaptiveToolbarHeight - 2 * kAdaptiveLocationBarVerticalMargin;
  CGFloat minHeightDiff =
      kLocationBarHeight - content_suggestions::kSearchFieldHeight;
  self.backgroundHeightConstraint.constant =
      content_suggestions::kSearchFieldHeight + minHeightDiff * percent;

  // Scale the hintLabel, and make sure the frame stays left aligned.
  [self scaleHintLabel:hintLabel percent:percent];

  // Adjust the position of the search field's subviews by adjusting their
  // constraint constant value.
  CGFloat constantDiff = percent * (ntp_header::kMaxHorizontalMarginDiff +
                                    inset + safeAreaInsets.left);
  for (NSLayoutConstraint* constraint in constraints) {
    if (constraint.constant > 0)
      constraint.constant = constantDiff + ntp_header::kHintLabelSidePadding;
    else
      constraint.constant = -constantDiff;
  }
}

- (void)fadeOutShadow {
  // No-op.
}

- (void)hideToolbarViewsForNewTabPage {
  // No-op.
}

- (void)setToolbarTabCount:(int)tabCount {
  // No-op.
}

- (void)setCanGoForward:(BOOL)canGoForward {
  // No-op.
}

- (void)setCanGoBack:(BOOL)canGoBack {
  // No-op.
}

#pragma mark - ToolbarOwner

- (CGRect)toolbarFrame {
  return self.toolBarView.frame;
}

- (id<ToolbarSnapshotProviding>)toolbarSnapshotProvider {
  return self;
}

#pragma mark - ToolbarSnapshotProviding

- (UIView*)snapshotForTabSwitcher {
  return nil;
}

- (UIView*)snapshotForStackViewWithWidth:(CGFloat)width
                          safeAreaInsets:(UIEdgeInsets)safeAreaInsets {
  return self.toolBarView;
}

- (UIColor*)toolbarBackgroundColor {
  return nil;
}

@end
