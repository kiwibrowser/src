// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_header_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_observer.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_utils.h"
#import "ios/chrome/browser/ui/image_util/image_util.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_header_constants.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_toolbar_controller.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_utils.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_snapshot_providing.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#import "ui/gfx/ios/uikit_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface NewTabPageHeaderView ()<ToolbarSnapshotProviding> {
  NewTabPageToolbarController* _toolbarController;
  UIImageView* _searchBoxBorder;
  UIImageView* _shadow;
}

@end

@implementation NewTabPageHeaderView

#pragma mark - Public

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.clipsToBounds = YES;
  }
  return self;
}


- (UIView*)toolBarView {
  return [_toolbarController view];
}

- (void)addToolbarWithReadingListModel:(ReadingListModel*)readingListModel
                            dispatcher:(id)dispatcher {
  DCHECK(!_toolbarController);
  DCHECK(readingListModel);

  _toolbarController =
      [[NewTabPageToolbarController alloc] initWithDispatcher:dispatcher];
  _toolbarController.readingListModel = readingListModel;

  [self addSubview:[_toolbarController view]];

  [self addConstraintsToToolbar];
}

- (void)setCanGoForward:(BOOL)canGoForward {
  [_toolbarController setCanGoForward:canGoForward];
  [self hideToolbarViewsForNewTabPage];
}

- (void)setCanGoBack:(BOOL)canGoBack {
  [_toolbarController setCanGoBack:canGoBack];
  [self hideToolbarViewsForNewTabPage];
}

- (void)hideToolbarViewsForNewTabPage {
  [_toolbarController hideViewsForNewTabPage:YES];
};

- (void)setToolbarTabCount:(int)tabCount {
  [_toolbarController setTabCount:tabCount];
}

- (void)addViewsToSearchField:(UIView*)searchField {
  [searchField setBackgroundColor:[UIColor whiteColor]];
  UIImage* searchBorderImage =
      StretchableImageNamed(@"ntp_google_search_box", 12, 12);
  _searchBoxBorder = [[UIImageView alloc] initWithImage:searchBorderImage];
  [_searchBoxBorder setFrame:[searchField bounds]];
  [_searchBoxBorder setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                                        UIViewAutoresizingFlexibleHeight];
  [searchField insertSubview:_searchBoxBorder atIndex:0];

  UIImage* fullBleedShadow = NativeImage(IDR_IOS_TOOLBAR_SHADOW_FULL_BLEED);
  _shadow = [[UIImageView alloc] initWithImage:fullBleedShadow];
  CGRect shadowFrame = [searchField bounds];
  shadowFrame.origin.y = searchField.bounds.size.height;
  shadowFrame.size.height = fullBleedShadow.size.height;
  [_shadow setFrame:shadowFrame];
  [_shadow setUserInteractionEnabled:NO];
  [_shadow setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                               UIViewAutoresizingFlexibleTopMargin];
  [searchField addSubview:_shadow];
  [_shadow setAlpha:0];
}

- (CGFloat)searchFieldProgressForOffset:(CGFloat)offset
                         safeAreaInsets:(UIEdgeInsets)safeAreaInsets {
  NOTREACHED();
  return 0;
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
  // The scroll offset at which point searchField's frame should stop growing.
  CGFloat maxScaleOffset = self.frame.size.height -
                           ntp_header::kMinHeaderHeight - safeAreaInsets.top;
  // The scroll offset at which point searchField's frame should start
  // growing.
  CGFloat startScaleOffset = maxScaleOffset - ntp_header::kAnimationDistance;
  CGFloat percent = 0;
  if (offset > startScaleOffset) {
    CGFloat animatingOffset = offset - startScaleOffset;
    percent = MIN(1, MAX(0, animatingOffset / ntp_header::kAnimationDistance));
  }

  if (screenWidth == 0 || contentWidth == 0)
    return;

  CGFloat searchFieldNormalWidth =
      content_suggestions::searchFieldWidth(contentWidth);

  // Calculate the amount to grow the width and height of searchField so that
  // its frame covers the entire toolbar area.
  CGFloat maxXInset = ui::AlignValueToUpperPixel(
      (searchFieldNormalWidth - screenWidth) / 2 - 1);
  CGFloat maxHeightDiff =
      ntp_header::ToolbarHeight() - content_suggestions::kSearchFieldHeight;

  widthConstraint.constant = searchFieldNormalWidth - 2 * maxXInset * percent;
  topMarginConstraint.constant = -content_suggestions::searchFieldTopMargin() -
                                 ntp_header::kMaxTopMarginDiff * percent;
  heightConstraint.constant =
      content_suggestions::kSearchFieldHeight + maxHeightDiff * percent;

  [_searchBoxBorder setAlpha:(1 - percent)];
  [_shadow setAlpha:percent];

  // Adjust the position of the search field's subviews by adjusting their
  // constraint constant value.
  CGFloat constantDiff =
      percent * (ntp_header::kMaxHorizontalMarginDiff + safeAreaInsets.left);
  for (NSLayoutConstraint* constraint in constraints) {
    if (constraint.constant > 0)
      constraint.constant =
          constantDiff + ntp_header::kHintLabelSidePaddingLegacy;
    else
      constraint.constant = -constantDiff;
  }
}

- (void)safeAreaInsetsDidChange {
  [super safeAreaInsetsDidChange];
  _toolbarController.heightConstraint.constant =
      ToolbarHeightWithTopOfScreenOffset([_toolbarController statusBarOffset]);
}

- (void)fadeOutShadow {
  [UIView animateWithDuration:ios::material::kDuration1
                   animations:^{
                     [_shadow setAlpha:0];
                   }];
}

#pragma mark - ToolbarOwner

- (CGRect)toolbarFrame {
  return _toolbarController.view.frame;
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
  UIView* toolbar = _toolbarController.view;
  CGRect oldFrame = toolbar.frame;
  CGRect newFrame = oldFrame;
  newFrame.size.width = width;

  toolbar.frame = newFrame;
  [_toolbarController activateFakeSafeAreaInsets:safeAreaInsets];

  UIView* toolbarSnapshotView;
  if ([toolbar window]) {
    // Take a snapshot only if it has been added to the view hierarchy.
    toolbarSnapshotView = [toolbar snapshotViewAfterScreenUpdates:NO];
  } else {
    toolbarSnapshotView = [[UIView alloc] initWithFrame:toolbar.frame];
    [toolbarSnapshotView layer].contents = static_cast<id>(
        CaptureViewWithOption(toolbar, 0, kClientSideRendering).CGImage);
  }

  toolbar.frame = oldFrame;
  [_toolbarController deactivateFakeSafeAreaInsets];

  return toolbarSnapshotView;
}

- (UIColor*)toolbarBackgroundColor {
  return [UIColor whiteColor];
}

#pragma mark - Private

- (void)addConstraintsToToolbar {
  _toolbarController.heightConstraint.constant =
      ToolbarHeightWithTopOfScreenOffset([_toolbarController statusBarOffset]);
  _toolbarController.heightConstraint.active = YES;
  [NSLayoutConstraint activateConstraints:@[
    [[_toolbarController view].leadingAnchor
        constraintEqualToAnchor:self.leadingAnchor],
    [[_toolbarController view].topAnchor
        constraintEqualToAnchor:self.topAnchor],
    [[_toolbarController view].trailingAnchor
        constraintEqualToAnchor:self.trailingAnchor],
  ]];
}

@end
