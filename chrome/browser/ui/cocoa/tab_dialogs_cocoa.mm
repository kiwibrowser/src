// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/tab_dialogs_cocoa.h"

#include <memory>

#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/hung_renderer_controller.h"
#import "chrome/browser/ui/cocoa/profiles/profile_signin_confirmation_dialog_cocoa.h"
#include "chrome/browser/ui/cocoa/tab_dialogs_views_mac.h"
#include "chrome/browser/ui/sync/profile_signin_confirmation_helper.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/ui_features.h"

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
#import "chrome/browser/ui/cocoa/content_settings/collected_cookies_mac.h"
#endif

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
// static
void TabDialogs::CreateForWebContents(content::WebContents* contents) {
  DCHECK(contents);

  if (!FromWebContents(contents)) {
    std::unique_ptr<TabDialogs> tab_dialogs =
        chrome::ShowAllDialogsWithViewsToolkit()
            ? std::make_unique<TabDialogsViewsMac>(contents)
            : std::make_unique<TabDialogsCocoa>(contents);
    contents->SetUserData(UserDataKey(), std::move(tab_dialogs));
  }
}
#endif

TabDialogsCocoa::TabDialogsCocoa(content::WebContents* contents)
    : web_contents_(contents) {
  DCHECK(contents);
}

TabDialogsCocoa::~TabDialogsCocoa() {
}

gfx::NativeView TabDialogsCocoa::GetDialogParentView() const {
  // View hierarchy of the contents view:
  // NSView  -- switchView, same for all tabs
  // +- TabContentsContainerView  -- TabContentsController's view
  //    +- WebContentsViewCocoa
  //
  // Changing it? Do not forget to modify
  // -[TabStripController swapInTabAtIndex:] too.
  return [web_contents_->GetNativeView() superview];
}

void TabDialogsCocoa::ShowCollectedCookies() {
#if BUILDFLAG(MAC_VIEWS_BROWSER)
  NOTREACHED() << "MacViewsBrowser builds can't use Cocoa dialogs";
#else
  // Deletes itself on close.
  new CollectedCookiesMac(web_contents_);
#endif
}

void TabDialogsCocoa::ShowHungRendererDialog(
    content::RenderWidgetHost* render_widget_host) {
  [HungRendererController showForWebContents:web_contents_
                            renderWidgetHost:render_widget_host];
}

void TabDialogsCocoa::HideHungRendererDialog(
    content::RenderWidgetHost* render_widget_host) {
  [HungRendererController endForWebContents:web_contents_
                           renderWidgetHost:render_widget_host];
}

bool TabDialogsCocoa::IsShowingHungRendererDialog() {
  return [HungRendererController isShowing];
}

void TabDialogsCocoa::ShowProfileSigninConfirmation(
    Browser* browser,
    Profile* profile,
    const std::string& username,
    std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate) {
  ProfileSigninConfirmationDialogCocoa::Show(browser, web_contents_, profile,
                                             username, std::move(delegate));
}

void TabDialogsCocoa::ShowManagePasswordsBubble(bool user_action) {
  // The method is implemented by TabDialogsViewsMac and only that one is to be
  // called due to MacViews release.
  NOTREACHED();
}

void TabDialogsCocoa::HideManagePasswordsBubble() {
  // The method is implemented by TabDialogsViewsMac and only that one is to be
  // called due to MacViews release.
  NOTREACHED();
}
