// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_view.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/metrics/user_metrics.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_header_view.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_controller.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_overlay_view.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_view.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kHeaderHeight = 95;
const CGFloat kNewTabButtonMarginFromEdges = 48;
const CGFloat kNewTabButtonWidth = 48;
}

@interface TabSwitcherView ()<UIScrollViewDelegate> {
  TabSwitcherHeaderView* _headerView;
  UIScrollView* _scrollView;
  MDCButton* _openNewTabButton;
  NSMutableArray* _panels;
  NewTabButtonStyle _openNewTabButtonStyle;
  NSInteger _previousPanelIndex;
}

// Returns the header view frame.
- (CGRect)headerViewFrame;
// Returns the scrollview frame.
- (CGRect)scrollViewFrame;
// Returns the new tab button frame.
- (CGRect)openNewTabButtonFrame;
// Returns the new tab button frame when hidden.
- (CGRect)openNewTabButtonFrameOffscreen;
// Called when the new tab button is pressed.
- (void)openNewTabButtonPressed;
// Returns the index of the panel presented in |_scrollview|. When two panels
// are visible, it returns the most visible one.
- (NSInteger)currentPageIndex;
// Select the panel at the given index, updating both the header and content.
// The scrollview scroll will be |animated| if requested.
- (void)selectPanelAtIndex:(NSInteger)index animated:(BOOL)animated;
@end

@implementation TabSwitcherView

@synthesize delegate = delegate_;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _openNewTabButtonStyle = NewTabButtonStyle::UNINITIALIZED;
    [self loadSubviews];
    _panels = [[NSMutableArray alloc] init];
    _previousPanelIndex = -1;
  }
  return self;
}

- (TabSwitcherHeaderView*)headerView {
  return _headerView;
}

- (UIScrollView*)scrollView {
  return _scrollView;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  NSInteger pageIndexBeforeLayout = [self currentPageIndex];
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  [_headerView setFrame:[self headerViewFrame]];
  [_scrollView setFrame:[self scrollViewFrame]];
  [CATransaction commit];
  [self updateScrollViewContent];
  [self selectPanelAtIndex:pageIndexBeforeLayout animated:NO];
}

- (void)selectPanelAtIndex:(NSInteger)index {
  [self selectPanelAtIndex:index animated:!UIAccessibilityIsVoiceOverRunning()];
}

- (void)addPanelView:(UIView*)view atIndex:(NSUInteger)index {
  if (UseRTLLayout())
    [view setTransform:CGAffineTransformMakeScale(-1, 1)];
  [_scrollView addSubview:view];
  [_panels insertObject:view atIndex:index];
  [self updateScrollViewContent];
}

- (void)removePanelViewAtIndex:(NSUInteger)index {
  [self removePanelViewAtIndex:index updateScrollView:YES];
}

- (void)removePanelViewAtIndex:(NSUInteger)index updateScrollView:(BOOL)update {
  DCHECK_EQ([[_panels objectAtIndex:index] superview], _scrollView);
  [[_panels objectAtIndex:index] removeFromSuperview];
  [_panels removeObjectAtIndex:index];

  if (_previousPanelIndex > -1) {
    NSUInteger previousPanelIndexUnsigned =
        static_cast<NSUInteger>(_previousPanelIndex);
    if (index < previousPanelIndexUnsigned)
      _previousPanelIndex--;
    else if (index == previousPanelIndexUnsigned) {
      [self panelWasHiddenAtIndex:_previousPanelIndex];
      _previousPanelIndex = -1;
    }
  }
  if (update)
    [self updateScrollViewContent];
}

- (NSInteger)currentPanelIndex {
  return [self currentPageIndex];
}

- (void)updateOverlayButtonState {
  NSInteger panelIndex = [self currentPageIndex];
  NewTabButtonStyle newButtonStyle =
      [delegate_ buttonStyleForPanelAtIndex:panelIndex];
  [self setNewTabButtonStyle:newButtonStyle];
  BOOL dismissButtonVisible =
      [self.delegate shouldShowDismissButtonForPanelAtIndex:panelIndex];
  [UIView beginAnimations:nil context:NULL];
  [[_headerView dismissButton] setAlpha:dismissButtonVisible ? 1.0 : 0.0];
  [UIView commitAnimations];
}

- (void)wasShown {
  [self currentPanelWasShown];
}

- (void)wasHidden {
  [self panelWasHiddenAtIndex:self.currentPageIndex];
}

#pragma mark - Private

- (void)selectPanelAtIndex:(NSInteger)index animated:(BOOL)animated {
  CGPoint pageOffset = CGPointZero;
  pageOffset.x = self.bounds.size.width * index;
  [_scrollView setContentOffset:pageOffset animated:animated];
  [_headerView selectItemAtIndex:index];
  UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification,
                                  nil);
}

- (NSInteger)currentPageIndex {
  if ([_scrollView frame].size.width == 0)
    return 0;
  NSInteger index = floor(
      [_scrollView contentOffset].x / [_scrollView frame].size.width + 0.5);
  return std::max<NSInteger>(std::min<NSInteger>(index, [_panels count] - 1),
                             0);
}

- (void)updateHeaderSelection {
  [_headerView selectItemAtIndex:[self currentPageIndex]];
}

- (CGRect)frameForPanelAtIndex:(NSInteger)index {
  CGSize panelSize = [_scrollView frame].size;
  CGRect panelFrame =
      CGRectMake(panelSize.width * index, 0, panelSize.width, panelSize.height);
  return panelFrame;
}

- (void)updateScrollViewContent {
  CGSize panelSize = [_scrollView frame].size;
  CGSize currentContentSize = [_scrollView contentSize];
  currentContentSize.width = [_panels count] * panelSize.width;
  [_scrollView setContentSize:currentContentSize];
  for (NSUInteger i = 0; i < [_panels count]; i++) {
    id panelView = [_panels objectAtIndex:i];
    [panelView setFrame:[self frameForPanelAtIndex:i]];
  }
}

- (void)loadSubviews {
  // Creates and add the header view showing the list of panels.
  TabSwitcherHeaderView* headerView =
      [[TabSwitcherHeaderView alloc] initWithFrame:[self headerViewFrame]];
  [self addSubview:headerView];
  _headerView = headerView;

  // Creates and add the scrollview containing the panels.
  UIScrollView* scrollView =
      [[UIScrollView alloc] initWithFrame:[self scrollViewFrame]];
  [scrollView setBackgroundColor:[[MDCPalette greyPalette] tint900]];
  [scrollView setAlwaysBounceHorizontal:YES];
  [scrollView setDelegate:self];
  [scrollView setPagingEnabled:YES];
  [scrollView setDelegate:self];
  if (UseRTLLayout())
    [scrollView setTransform:CGAffineTransformMakeScale(-1, 1)];
  [self addSubview:scrollView];
  _scrollView = scrollView;

  // Creates and add the floating new tab button.
  _openNewTabButton = [[MDCFloatingButton alloc] init];
  UIImage* openNewTabButtonImage =
      [[UIImage imageNamed:@"tabswitcher_new_tab_fab"]
          imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  [_openNewTabButton setImage:openNewTabButtonImage
                     forState:UIControlStateNormal];
  [[_openNewTabButton imageView] setTintColor:[UIColor whiteColor]];
  [_openNewTabButton setFrame:[self openNewTabButtonFrame]];
  // When the button is positioned with autolayout, the animation of the
  // button's position conflicts with other animations.
  [_openNewTabButton
      setAutoresizingMask:UIViewAutoresizingFlexibleTopMargin |
                          UIViewAutoresizingFlexibleLeadingMargin()];
  [_openNewTabButton addTarget:self
                        action:@selector(openNewTabButtonPressed)
              forControlEvents:UIControlEventTouchUpInside];
  [self addSubview:_openNewTabButton];
  [self setNewTabButtonStyle:NewTabButtonStyle::GRAY];
}

- (CGRect)headerViewFrame {
  const CGFloat kStatusBarHeight =
      [[UIApplication sharedApplication] statusBarFrame].size.height;
  CGRect headerViewFrame = self.bounds;
  headerViewFrame.origin.y += kStatusBarHeight;
  headerViewFrame.size.height = kHeaderHeight;
  return headerViewFrame;
}

- (CGRect)scrollViewFrame {
  CGRect scrollViewFrame = self.bounds;
  scrollViewFrame.origin.y = CGRectGetMaxY([self headerViewFrame]);
  scrollViewFrame.size.height -= scrollViewFrame.origin.y;
  return scrollViewFrame;
}

- (CGRect)openNewTabButtonFrame {
  CGRect buttonFrame = self.bounds;
  buttonFrame.origin.y = self.bounds.size.height -
                         kNewTabButtonMarginFromEdges - kNewTabButtonWidth;
  if (UseRTLLayout()) {
    buttonFrame.origin.x = kNewTabButtonMarginFromEdges;
  } else {
    buttonFrame.origin.x = self.bounds.size.width -
                           kNewTabButtonMarginFromEdges - kNewTabButtonWidth;
  }
  buttonFrame.size.width = kNewTabButtonWidth;
  buttonFrame.size.height = kNewTabButtonWidth;
  return buttonFrame;
}

- (CGRect)openNewTabButtonFrameOffscreen {
  CGRect buttonFrame = [self openNewTabButtonFrame];
  buttonFrame.origin.y = self.bounds.size.height + kNewTabButtonWidth;
  return buttonFrame;
}

- (void)openNewTabButtonPressed {
  [self.delegate openNewTabInPanelAtIndex:[self currentPageIndex]];
}

- (void)setNewTabButtonStyle:(NewTabButtonStyle)newStyle {
  if (newStyle == _openNewTabButtonStyle)
    return;
  _openNewTabButtonStyle = newStyle;
  [UIView animateWithDuration:0.25
      animations:^{
        switch (_openNewTabButtonStyle) {
          case NewTabButtonStyle::HIDDEN:
            [_openNewTabButton setFrame:[self openNewTabButtonFrameOffscreen]];
            break;
          case NewTabButtonStyle::BLUE: {
            [_openNewTabButton setFrame:[self openNewTabButtonFrame]];
            MDCPalette* palette = [MDCPalette cr_bluePalette];
            [_openNewTabButton
                setInkColor:[[palette tint300] colorWithAlphaComponent:0.5f]];
            [_openNewTabButton setBackgroundColor:[palette tint500]
                                         forState:UIControlStateNormal];
            [_openNewTabButton
                setBackgroundColor:[UIColor colorWithWhite:0.8f alpha:1.0f]
                          forState:UIControlStateDisabled];
            [_openNewTabButton
                setAccessibilityLabel:l10n_util::GetNSString(
                                          IDS_IOS_TAB_SWITCHER_CREATE_NEW_TAB)];
            break;
          }
          case NewTabButtonStyle::GRAY: {
            [_openNewTabButton setFrame:[self openNewTabButtonFrame]];
            MDCPalette* palette = [MDCPalette greyPalette];
            [_openNewTabButton
                setInkColor:[[palette tint300] colorWithAlphaComponent:0.25f]];
            [_openNewTabButton setBackgroundColor:[palette tint500]
                                         forState:UIControlStateNormal];
            [_openNewTabButton
                setBackgroundColor:[UIColor colorWithWhite:0.8f alpha:1.0f]
                          forState:UIControlStateDisabled];
            [_openNewTabButton
                setAccessibilityLabel:
                    l10n_util::GetNSString(
                        IDS_IOS_TAB_SWITCHER_CREATE_NEW_INCOGNITO_TAB)];
            break;
          }
          case NewTabButtonStyle::UNINITIALIZED:
            NOTREACHED();
        }
      }
      completion:^(BOOL finished) {
        // Informs VoiceOver that the |_openNewTabButton| visibility may have
        // changed.
        UIAccessibilityPostNotification(
            UIAccessibilityLayoutChangedNotification, nil);
      }];
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewDidEndScrollingAnimation:(UIScrollView*)scrollView {
  [self updateHeaderSelection];
}

- (void)scrollViewDidScroll:(UIScrollView*)scrollView {
  if ([scrollView isDragging]) {
    [self updateHeaderSelection];
  }
  [self updateOverlayButtonState];

  NSInteger panelIndex = [self currentPanelIndex];
  if (panelIndex != _previousPanelIndex) {
    if (_previousPanelIndex != -1)
      [self panelWasHiddenAtIndex:_previousPanelIndex];
    [self currentPanelWasShown];
  }
  _previousPanelIndex = panelIndex;
}

- (void)panelWasHiddenAtIndex:(NSUInteger)index {
  if (index >= [_panels count])
    return;
  id panel = [_panels objectAtIndex:index];
  if ([panel respondsToSelector:@selector(wasHidden)])
    [panel wasHidden];
}

- (void)currentPanelWasShown {
  id panel = [_panels objectAtIndex:self.currentPanelIndex];
  if ([panel respondsToSelector:@selector(wasShown)])
    [panel wasShown];
}

#pragma mark - UIScrollViewAccessibilityDelegate

- (NSString*)accessibilityScrollStatusForScrollView:(UIScrollView*)scrollView {
  NSInteger panelIndex = [self currentPageIndex];
  return [_headerView panelTitleAtIndex:panelIndex];
}

#pragma mark - UIAccessibilityAction

- (BOOL)accessibilityPerformEscape {
  // If the Dismiss Button is visible, then the escape gesture triggers the
  // dismissal of the tab switcher.
  NSInteger panelIndex = [self currentPageIndex];
  if ([self.delegate shouldShowDismissButtonForPanelAtIndex:panelIndex]) {
    [delegate_ tabSwitcherViewDelegateDismissTabSwitcher:self];
    return YES;
  }
  return NO;
}

- (BOOL)accessibilityPerformMagicTap {
  // If the New Tab Button is visible, then the magic tap opens a new tab.
  NSInteger panelIndex = [self currentPageIndex];
  NewTabButtonStyle buttonStyle =
      [delegate_ buttonStyleForPanelAtIndex:panelIndex];
  switch (buttonStyle) {
    case NewTabButtonStyle::BLUE:
    case NewTabButtonStyle::GRAY:
      [self.delegate openNewTabInPanelAtIndex:panelIndex];
      return YES;
    case NewTabButtonStyle::UNINITIALIZED:
    case NewTabButtonStyle::HIDDEN:
      return NO;
  }
}

@end
