// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/incognito_view_controller.h"

#include <string>

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/ntp/incognito_view.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_controller_delegate.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/url_loader.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kDistanceToFadeToolbar = 50.0;
}  // namespace

@interface IncognitoViewController ()<UIScrollViewDelegate>
// The scrollview containing the actual views.
@property(nonatomic, strong) IncognitoView* incognitoView;

@property(nonatomic, weak) id<NewTabPageControllerDelegate> toolbarDelegate;
@property(nonatomic, weak) id<UrlLoader> loader;
@end

@implementation IncognitoViewController

@synthesize incognitoView = _incognitoView;
@synthesize toolbarDelegate = _toolbarDelegate;
@synthesize loader = _loader;

// Property declared in NewTabPagePanelProtocol.
@synthesize delegate = _delegate;

- (id)initWithLoader:(id<UrlLoader>)loader
     toolbarDelegate:(id<NewTabPageControllerDelegate>)toolbarDelegate {
  self = [super init];
  if (self) {
    _loader = loader;
    if (!IsIPadIdiom()) {
      _toolbarDelegate = toolbarDelegate;
      [_toolbarDelegate setToolbarBackgroundToIncognitoNTPColorWithAlpha:1];
    }
  }
  return self;
}

- (void)viewDidLoad {
  self.incognitoView = [[IncognitoView alloc]
      initWithFrame:[UIApplication sharedApplication].keyWindow.bounds
          urlLoader:self.loader];
  [self.incognitoView setAutoresizingMask:UIViewAutoresizingFlexibleHeight |
                                          UIViewAutoresizingFlexibleWidth];

  [self.incognitoView
      setBackgroundColor:[UIColor colorWithWhite:34 / 255.0 alpha:1.0]];

  if (!IsIPadIdiom()) {
    [self.incognitoView setDelegate:self];
  }

  [self.view addSubview:self.incognitoView];
}

- (void)dealloc {
  [_toolbarDelegate setToolbarBackgroundToIncognitoNTPColorWithAlpha:0];
  [_incognitoView setDelegate:nil];
}

#pragma mark - NewTabPagePanelProtocol

- (void)reload {
}

- (void)wasShown {
  CGFloat alpha =
      [self incognitoBackgroundAlphaForScrollView:self.incognitoView];
  [self.toolbarDelegate setToolbarBackgroundToIncognitoNTPColorWithAlpha:alpha];
}

- (void)wasHidden {
  [self.toolbarDelegate setToolbarBackgroundToIncognitoNTPColorWithAlpha:0];
}

- (void)dismissModals {
}

- (CGFloat)alphaForBottomShadow {
  return 0;
}

- (CGPoint)scrollOffset {
  return CGPointZero;
}

- (void)willUpdateSnapshot {
}

#pragma mark - UIScrollViewDelegate methods

- (void)scrollViewDidScroll:(UIScrollView*)scrollView {
  CGFloat alpha =
      [self incognitoBackgroundAlphaForScrollView:self.incognitoView];
  [self.toolbarDelegate setToolbarBackgroundToIncognitoNTPColorWithAlpha:alpha];
}

#pragma mark - Private

// Calculate the alpha for the toolbar background color of the NTP's color.
- (CGFloat)incognitoBackgroundAlphaForScrollView:(UIScrollView*)scrollView {
  CGFloat alpha = (kDistanceToFadeToolbar - scrollView.contentOffset.y) /
                  kDistanceToFadeToolbar;
  return MAX(alpha, 0);
}

@end
