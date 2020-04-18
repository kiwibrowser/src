// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_handset_view_controller.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/keyboard/UIKeyCommand+Chrome.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/legacy_recent_tabs_table_coordinator.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/panel_bar_view.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// A UIViewController that forces the status bar to be visible.
@interface RecentTabsHandsetViewController ()

@property(nonatomic, strong) PanelBarView* panelBarView;
@property(nonatomic, strong) UIViewController* panelViewController;

@end

@implementation RecentTabsHandsetViewController

@synthesize panelBarView = _panelBarView;
@synthesize panelViewController = _panelViewController;
@synthesize commandHandler = _commandHandler;

- (instancetype)initWithViewController:(UIViewController*)panelViewController {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _panelViewController = panelViewController;
  }
  return self;
}

- (void)viewDidLoad {
  self.panelBarView = [[PanelBarView alloc] init];
  [self.panelBarView setCloseTarget:self action:@selector(didFinish)];
  UIImageView* shadow =
      [[UIImageView alloc] initWithImage:NativeImage(IDR_IOS_TOOLBAR_SHADOW)];

  [self.panelBarView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [self.panelViewController.view
      setTranslatesAutoresizingMaskIntoConstraints:NO];
  [shadow setTranslatesAutoresizingMaskIntoConstraints:NO];

  [self.view addSubview:self.panelBarView];

  [self addChildViewController:self.panelViewController];
  [self.view addSubview:self.panelViewController.view];
  [self.panelViewController didMoveToParentViewController:self];

  [self.view addSubview:shadow];

  NSDictionary* viewsDictionary = @{
    @"bar" : self.panelBarView,
    @"table" : self.panelViewController.view,
    @"shadow" : shadow
  };

  NSArray* constraints = @[
    @"V:|-0-[bar]-0-[table]-0-|", @"V:[bar]-0-[shadow]", @"H:|-0-[bar]-0-|",
    @"H:|-0-[table]-0-|", @"H:|-0-[shadow]-0-|"
  ];

  ApplyVisualConstraints(constraints, viewsDictionary);
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:
           (id<UIViewControllerTransitionCoordinator>)coordinator {
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [self.panelBarView setNeedsUpdateConstraints];
}

- (BOOL)prefersStatusBarHidden {
  return NO;
}

- (NSArray*)keyCommands {
  __weak RecentTabsHandsetViewController* weakSelf = self;
  return @[ [UIKeyCommand cr_keyCommandWithInput:UIKeyInputEscape
                                   modifierFlags:Cr_UIKeyModifierNone
                                           title:nil
                                          action:^{
                                            [weakSelf didFinish];
                                          }] ];
}

#pragma mark Accessibility

- (BOOL)accessibilityPerformEscape {
  [self didFinish];
  return YES;
}

- (void)didFinish {
  [self.commandHandler dismissRecentTabsWithCompletion:nil];
}

@end
