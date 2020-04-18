// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_toolbar_controller.h"

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "components/strings/grit/components_strings.h"
#include "components/toolbar/toolbar_model.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller+protected.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller_constants.h"
#import "ios/chrome/browser/ui/toolbar/public/fakebox_focuser.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"
#include "ios/chrome/browser/ui/toolbar/toolbar_resource_macros.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;

namespace {

const CGFloat kButtonYOffset = 4.0;
const CGFloat kBackButtonLeading = 0;
const CGFloat kForwardButtonLeading = 48;
const CGSize kBackButtonSize = {48, 48};
const CGSize kForwardButtonSize = {48, 48};
const CGFloat kOmniboxFocuserTrailing = 96;

enum {
  NTPToolbarButtonNameBack = NumberOfToolbarButtonNames,
  NTPToolbarButtonNameForward,
  NumberOfNTPToolbarButtonNames,
};

}  // namespace

@interface NewTabPageToolbarController () {
  UIButton* _backButton;
  UIButton* _forwardButton;
  UIButton* _omniboxFocuser;
}

// |YES| if the google landing toolbar can show the forward arrow.
@property(nonatomic, assign) BOOL canGoForward;

// |YES| if the google landing toolbar can show the back arrow.
@property(nonatomic, assign) BOOL canGoBack;

@end

@implementation NewTabPageToolbarController

@synthesize canGoForward = _canGoForward;
@synthesize canGoBack = _canGoBack;
@dynamic dispatcher;

- (instancetype)initWithDispatcher:(id<ApplicationCommands,
                                       BrowserCommands,
                                       OmniboxFocuser,
                                       FakeboxFocuser,
                                       ToolbarCommands,
                                       UrlLoader>)dispatcher {
  self = [super initWithStyle:ToolbarControllerStyleLightMode
                   dispatcher:dispatcher];
  if (self) {
    [self.backgroundView setHidden:YES];

    CGFloat boundingWidth = self.view.bounds.size.width;
    LayoutRect backButtonLayout =
        LayoutRectMake(kBackButtonLeading, boundingWidth, kButtonYOffset,
                       kBackButtonSize.width, kBackButtonSize.height);
    _backButton =
        [[UIButton alloc] initWithFrame:LayoutRectGetRect(backButtonLayout)];
    [_backButton
        setAutoresizingMask:UIViewAutoresizingFlexibleTrailingMargin() |
                            UIViewAutoresizingFlexibleBottomMargin];
    LayoutRect forwardButtonLayout =
        LayoutRectMake(kForwardButtonLeading, boundingWidth, kButtonYOffset,
                       kForwardButtonSize.width, kForwardButtonSize.height);
    _forwardButton =
        [[UIButton alloc] initWithFrame:LayoutRectGetRect(forwardButtonLayout)];
    [_forwardButton
        setAutoresizingMask:UIViewAutoresizingFlexibleTrailingMargin() |
                            UIViewAutoresizingFlexibleBottomMargin];
    _omniboxFocuser = [[UIButton alloc] init];
    [_omniboxFocuser
        setAccessibilityLabel:l10n_util::GetNSString(IDS_ACCNAME_LOCATION)];

    _omniboxFocuser.translatesAutoresizingMaskIntoConstraints = NO;

    [self.contentView addSubview:_backButton];
    [self.contentView addSubview:_forwardButton];
    [self.contentView addSubview:_omniboxFocuser];
    [NSLayoutConstraint activateConstraints:@[
      [_omniboxFocuser.leadingAnchor
          constraintEqualToAnchor:_forwardButton.trailingAnchor],
      [_omniboxFocuser.trailingAnchor
          constraintEqualToAnchor:self.contentView.trailingAnchor
                         constant:-kOmniboxFocuserTrailing],
      [_omniboxFocuser.topAnchor
          constraintEqualToAnchor:_forwardButton.topAnchor],
      [_omniboxFocuser.bottomAnchor
          constraintEqualToAnchor:_forwardButton.bottomAnchor]
    ]];

    [_backButton setImageEdgeInsets:UIEdgeInsetsMakeDirected(0, 0, 0, -10)];
    [_forwardButton setImageEdgeInsets:UIEdgeInsetsMakeDirected(0, -7, 0, 0)];

    // Set up the button images.
    [self setUpButton:_backButton
           withImageEnum:NTPToolbarButtonNameBack
         forInitialState:UIControlStateDisabled
        hasDisabledImage:YES
           synchronously:NO];
    [self setUpButton:_forwardButton
           withImageEnum:NTPToolbarButtonNameForward
         forInitialState:UIControlStateDisabled
        hasDisabledImage:YES
           synchronously:NO];

    UILongPressGestureRecognizer* backLongPress =
        [[UILongPressGestureRecognizer alloc]
            initWithTarget:self
                    action:@selector(handleLongPress:)];
    [_backButton addGestureRecognizer:backLongPress];
    [_backButton addTarget:self.dispatcher
                    action:@selector(goBack)
          forControlEvents:UIControlEventTouchUpInside];

    UILongPressGestureRecognizer* forwardLongPress =
        [[UILongPressGestureRecognizer alloc]
            initWithTarget:self
                    action:@selector(handleLongPress:)];
    [_forwardButton addGestureRecognizer:forwardLongPress];
    [_forwardButton addTarget:self.dispatcher
                       action:@selector(goForward)
             forControlEvents:UIControlEventTouchUpInside];

    [_omniboxFocuser addTarget:self
                        action:@selector(focusOmnibox:)
              forControlEvents:UIControlEventTouchUpInside];

    SetA11yLabelAndUiAutomationName(_backButton, IDS_ACCNAME_BACK, @"Back");
    SetA11yLabelAndUiAutomationName(_forwardButton, IDS_ACCNAME_FORWARD,
                                    @"Forward");

    [[self stackButton] addTarget:dispatcher
                           action:@selector(displayTabSwitcher)
                 forControlEvents:UIControlEventTouchUpInside];
  }
  return self;
}

#pragma mark - Overridden superclass public methods.

- (BOOL)imageShouldFlipForRightToLeftLayoutDirection:(int)imageEnum {
  DCHECK(imageEnum < NumberOfNTPToolbarButtonNames);
  if (imageEnum < NumberOfToolbarButtonNames)
    return [super imageShouldFlipForRightToLeftLayoutDirection:imageEnum];
  if (imageEnum == NTPToolbarButtonNameBack ||
      imageEnum == NTPToolbarButtonNameForward) {
    return YES;
  }
  return NO;
}

- (void)hideViewsForNewTabPage:(BOOL)hide {
  [super hideViewsForNewTabPage:hide];
  // Show the back/forward buttons if there is forward history.
  BOOL forwardEnabled = self.canGoForward;
  [_backButton setHidden:!forwardEnabled && hide];
  [_backButton setEnabled:self.canGoBack];
  [_forwardButton setHidden:!forwardEnabled && hide];
}

#pragma mark - Overridden superclass protected methods.

- (CGFloat)statusBarOffset {
  return 0;
}

- (int)imageEnumForButton:(UIButton*)button {
  if (button == _backButton)
    return NTPToolbarButtonNameBack;
  if (button == _forwardButton)
    return NTPToolbarButtonNameForward;
  return [super imageEnumForButton:button];
}

- (int)imageIdForImageEnum:(int)index
                     style:(ToolbarControllerStyle)style
                  forState:(ToolbarButtonUIState)state {
  DCHECK(style < ToolbarControllerStyleMaxStyles);
  DCHECK(state < NumberOfToolbarButtonUIStates);

  if (index >= NumberOfNTPToolbarButtonNames)
    NOTREACHED();
  if (index < NumberOfToolbarButtonNames)
    return [super imageIdForImageEnum:index style:style forState:state];

  index -= NumberOfToolbarButtonNames;

  const int numberOfAddedNames =
      NumberOfNTPToolbarButtonNames - NumberOfToolbarButtonNames;
  // Name, style [light, dark], UIControlState [normal, pressed, disabled]
  static int
      buttonImageIds[numberOfAddedNames][2][NumberOfToolbarButtonUIStates] = {
          TOOLBAR_IDR_THREE_STATE(BACK), TOOLBAR_IDR_THREE_STATE(FORWARD),
      };
  return buttonImageIds[index][style][state];
}

- (IBAction)recordUserMetrics:(id)sender {
  if (sender == _backButton) {
    base::RecordAction(UserMetricsAction("MobileToolbarBack"));
  } else if (sender == _forwardButton) {
    base::RecordAction(UserMetricsAction("MobileToolbarForward"));
  } else {
    [super recordUserMetrics:sender];
  }
}

#pragma mark - Private methods.

- (void)handleLongPress:(UILongPressGestureRecognizer*)gesture {
  if (gesture.state != UIGestureRecognizerStateBegan)
    return;

  if (gesture.view == _backButton) {
    [self.dispatcher showTabHistoryPopupForBackwardHistory];
  } else if (gesture.view == _forwardButton) {
    [self.dispatcher showTabHistoryPopupForForwardHistory];
  }
}

- (void)focusOmnibox:(id)sender {
  [self.dispatcher focusFakebox];
}

@end
