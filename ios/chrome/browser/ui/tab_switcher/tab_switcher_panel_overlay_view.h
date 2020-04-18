// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_OVERLAY_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_OVERLAY_VIEW_H_

#import <UIKit/UIKit.h>

@protocol ApplicationCommands;
@protocol BrowserCommands;
@protocol SigninPresenter;
@class SigninPromoView;
@protocol SyncPresenter;
@class TabSwitcherPanelOverlayView;

namespace ios {
class ChromeBrowserState;
}

enum class TabSwitcherPanelOverlayType {
  OVERLAY_PANEL_EMPTY,
  OVERLAY_PANEL_USER_SIGNED_OUT,
  OVERLAY_PANEL_USER_SIGNED_IN_SYNC_OFF,
  OVERLAY_PANEL_USER_SIGNED_IN_SYNC_ON_NO_SESSIONS,
  OVERLAY_PANEL_USER_SIGNED_IN_SYNC_IN_PROGRESS,
  OVERLAY_PANEL_USER_NO_OPEN_TABS,
  OVERLAY_PANEL_USER_NO_INCOGNITO_TABS
};

enum class TabSwitcherSignInPanelsType;

TabSwitcherPanelOverlayType PanelOverlayTypeFromSignInPanelsType(
    TabSwitcherSignInPanelsType signInPanelType);

// Delegate protocol for TabSwitcherPanelOverlayView.
@protocol TabSwitcherPanelOverlayViewDelegate<NSObject>

// Called when TabSwitcherPanelOverlayView is shown.
- (void)tabSwitcherPanelOverlViewWasShown:
    (TabSwitcherPanelOverlayView*)tabSwitcherPanelOverlayView;
// Called when TabSwitcherPanelOverlayView is hidden.
- (void)tabSwitcherPanelOverlViewWasHidden:
    (TabSwitcherPanelOverlayView*)tabSwitcherPanelOverlayView;

@end

@interface TabSwitcherPanelOverlayView : UIView

@property(nonatomic, assign) TabSwitcherPanelOverlayType overlayType;
@property(nonatomic, readonly, weak) id<SigninPresenter, SyncPresenter>
    presenter;
@property(nonatomic, readonly, weak) id<ApplicationCommands, BrowserCommands>
    dispatcher;
// Sign-in promo view. Nil if the |overlayType| is not
// |OVERLAY_PANEL_USER_SIGNED_OUT|.
@property(nonatomic, readonly) SigninPromoView* signinPromoView;
// Delegate, can be nil.
@property(nonatomic, weak) id<TabSwitcherPanelOverlayViewDelegate> delegate;

- (instancetype)initWithFrame:(CGRect)frame
                 browserState:(ios::ChromeBrowserState*)browserState
                    presenter:(id<SigninPresenter, SyncPresenter>)presenter
                   dispatcher:
                       (id<ApplicationCommands, BrowserCommands>)dispatcher;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_OVERLAY_VIEW_H_
