// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/tab_dialogs_views_mac.h"

#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/bubble_anchor_helper_views.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#include "chrome/browser/ui/cocoa/location_bar/manage_passwords_decoration.h"
#include "chrome/browser/ui/views/collected_cookies_views.h"
#include "chrome/browser/ui/views/passwords/password_bubble_view_base.h"
#include "chrome/browser/ui/views/sync/profile_signin_confirmation_dialog_views.h"
#include "chrome/browser/ui/views/tab_dialogs_views.h"
#include "chrome/browser/ui/views_mode_controller.h"
#include "content/public/browser/web_contents.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/ui_features.h"
#import "ui/gfx/mac/coordinate_conversion.h"

namespace {

gfx::Point ScreenPointFromBrowser(Browser* browser, NSPoint ns_point) {
  return gfx::ScreenPointFromNSPoint(ui::ConvertPointFromWindowToScreen(
      browser->window()->GetNativeWindow(), ns_point));
}
}

#if BUILDFLAG(MAC_VIEWS_BROWSER)
// static
void TabDialogs::CreateForWebContents(content::WebContents* contents) {
  DCHECK(contents);

  if (!FromWebContents(contents)) {
    // This is a bit subtle: if IsViewsBrowserCocoa(), that means this is a
    // mac_views_browser build using a Cocoa browser window, in which case
    // TabDialogsViewsMac is the right implementation; mostly it inherits
    // behavior from TabDialogsCocoa, which will only work with a Cocoa browser
    // window. If !IsViewsBrowserCocoa(), there is a Views browser window, so
    // TabDialogsViews (which is the only implementation that works with a Views
    // browser window) is the right implementation.
    //
    // Note that the ternary below can't use std::make_unique<> because
    // TabDialogsViewsMac and TabDialogsViews are not compatible types (neither
    // is an ancestor of the other).
    std::unique_ptr<TabDialogs> tab_dialogs(
        views_mode_controller::IsViewsBrowserCocoa()
            ? static_cast<TabDialogs*>(new TabDialogsViewsMac(contents))
            : static_cast<TabDialogs*>(new TabDialogsViews(contents)));
    contents->SetUserData(UserDataKey(), std::move(tab_dialogs));
  }
}
#endif

TabDialogsViewsMac::TabDialogsViewsMac(content::WebContents* contents)
    : TabDialogsCocoa(contents) {}

TabDialogsViewsMac::~TabDialogsViewsMac() {}

void TabDialogsViewsMac::ShowCollectedCookies() {
  // Deletes itself on close.
  new CollectedCookiesViews(web_contents());
}

void TabDialogsViewsMac::ShowProfileSigninConfirmation(
    Browser* browser,
    Profile* profile,
    const std::string& username,
    std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate) {
  ProfileSigninConfirmationDialogViews::ShowDialog(browser, profile, username,
                                                   std::move(delegate));
}

void TabDialogsViewsMac::ShowManagePasswordsBubble(bool user_action) {
  if (!chrome::ShowAllDialogsWithViewsToolkit()) {
    TabDialogsCocoa::ShowManagePasswordsBubble(user_action);
    return;
  }
  NSWindow* window = [web_contents()->GetNativeView() window];
  if (!window) {
    // The tab isn't active right now.
    return;
  }

  // Don't show the bubble again if it's already showing. A second click on the
  // location icon in the omnibox will dismiss an open bubble. This behaviour is
  // consistent with the non-Mac views implementation.
  // Note that when the browser is toolkit-views, IsBubbleShown() is checked
  // earlier because the bubble is shown on mouse release (but dismissed on
  // mouse pressed). A Cocoa browser does both on mouse pressed, so a check
  // when showing is sufficient.
  if (PasswordBubbleViewBase::manage_password_bubble() &&
      PasswordBubbleViewBase::manage_password_bubble()
          ->GetWidget()
          ->IsVisible())
    return;

  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  bool has_location_bar =
      browser && browser->SupportsWindowFeature(Browser::FEATURE_LOCATIONBAR);

  NSPoint ns_anchor_point;
  views::BubbleBorder::Arrow arrow = views::BubbleBorder::TOP_RIGHT;
  if (has_location_bar) {
    BrowserWindowController* bwc =
        [BrowserWindowController browserWindowControllerForWindow:window];
    LocationBarViewMac* location_bar = [bwc locationBarBridge];
    ns_anchor_point = location_bar->GetBubblePointForDecoration(
        location_bar->manage_passwords_decoration());
  } else {
    // Center the bubble if there's no location bar.
    NSRect content_frame = [[window contentView] frame];
    ns_anchor_point = NSMakePoint(NSMidX(content_frame), NSMaxY(content_frame));
    arrow = views::BubbleBorder::TOP_CENTER;
  }
  gfx::Point anchor_point = ScreenPointFromBrowser(browser, ns_anchor_point);
  gfx::NativeView parent =
      platform_util::GetViewForWindow(browser->window()->GetNativeWindow());
  DCHECK(parent);

  LocationBarBubbleDelegateView::DisplayReason reason =
      user_action ? LocationBarBubbleDelegateView::USER_GESTURE
                  : LocationBarBubbleDelegateView::AUTOMATIC;

  PasswordBubbleViewBase* bubble_view = PasswordBubbleViewBase::CreateBubble(
      web_contents(), nullptr, anchor_point, reason);
  bubble_view->set_arrow(arrow);
  bubble_view->set_parent_window(parent);
  views::BubbleDialogDelegateView::CreateBubble(bubble_view);
  bubble_view->ShowForReason(reason);
  KeepBubbleAnchored(bubble_view, GetManagePasswordDecoration(window));
}

void TabDialogsViewsMac::HideManagePasswordsBubble() {
  // Close toolkit-views bubble.
  if (!PasswordBubbleViewBase::manage_password_bubble())
    return;
  const content::WebContents* bubble_web_contents =
      PasswordBubbleViewBase::manage_password_bubble()->GetWebContents();
  if (web_contents() == bubble_web_contents)
    PasswordBubbleViewBase::CloseCurrentBubble();
}
