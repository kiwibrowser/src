// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/profiles/signin_view_controller_delegate_mac.h"

#import <Cocoa/Cocoa.h>

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/signin/unified_consent_helper.h"
#include "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/signin/sync_confirmation_ui.h"
#include "chrome/common/url_constants.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/ui_features.h"

namespace {

// Width of the different dialogs that make up the signin flow.
const int kModalDialogWidth = 448;

// Width of the confirmation dialog with DICE.
const int kModalDialogWidthForDice = 512;

// Height of the tab-modal dialog displaying the password-separated signin
// flow. It matches the dimensions of the server content the dialog displays.
const CGFloat kFixedGaiaViewHeight = 612;

// Initial height of the sync confirmation modal dialog.
const int kSyncConfirmationDialogHeight = 351;

// Initial height of the signin error modal dialog.
const int kSigninErrorDialogHeight = 164;

CGFloat GetSyncConfirmationDialogPreferredHeight(Profile* profile) {
  // If sync is disabled, then the sync confirmation dialog looks like an error
  // dialog and thus it has the same preferred size.
  return profile->IsSyncAllowed() ? kSyncConfirmationDialogHeight
                                  : kSigninErrorDialogHeight;
}

int GetSyncConfirmationDialogPreferredWidth(Profile* profile) {
  // If unified-consent enabled, we show a different sync confirmation dialog
  // which uses a different width.
  return IsUnifiedConsentEnabled(profile) && profile->IsSyncAllowed()
             ? kModalDialogWidthForDice
             : kModalDialogWidth;
}

}  // namespace

SigninViewControllerDelegateMac::SigninViewControllerDelegateMac(
    SigninViewController* signin_view_controller,
    std::unique_ptr<content::WebContents> web_contents,
    Browser* browser,
    NSRect frame,
    ui::ModalType dialog_modal_type,
    bool wait_for_size)
    : SigninViewControllerDelegate(signin_view_controller,
                                   web_contents.get(),
                                   browser),
      web_contents_(std::move(web_contents)),
      dialog_modal_type_(dialog_modal_type),
      window_frame_(frame) {
  if (!wait_for_size)
    DisplayModal();
}

SigninViewControllerDelegateMac::~SigninViewControllerDelegateMac() {}

void SigninViewControllerDelegateMac::OnConstrainedWindowClosed(
    ConstrainedWindowMac* window) {
  CleanupAndDeleteThis();
}

// static
std::unique_ptr<content::WebContents>
SigninViewControllerDelegateMac::CreateGaiaWebContents(
    content::WebContentsDelegate* delegate,
    profiles::BubbleViewMode mode,
    Profile* profile,
    signin_metrics::AccessPoint access_point) {
  GURL url =
      signin::GetSigninURLFromBubbleViewMode(profile, mode, access_point);

  std::unique_ptr<content::WebContents> web_contents(
      content::WebContents::Create(
          content::WebContents::CreateParams(profile)));

  if (delegate)
    web_contents->SetDelegate(delegate);

  web_contents->GetController().LoadURL(url, content::Referrer(),
                                        ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                        std::string());
  NSView* webview = web_contents->GetNativeView();
  [webview setFrameSize:NSMakeSize(kModalDialogWidth, kFixedGaiaViewHeight)];

  content::RenderWidgetHostView* rwhv = web_contents->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetBackgroundColor(profiles::kAvatarBubbleGaiaBackgroundColor);

  return web_contents;
}

// static
std::unique_ptr<content::WebContents>
SigninViewControllerDelegateMac::CreateSyncConfirmationWebContents(
    Browser* browser,
    bool is_consent_bump) {
  return CreateDialogWebContents(
      browser,
      is_consent_bump ? chrome::kChromeUISyncConsentBumpURL
                      : chrome::kChromeUISyncConfirmationURL,
      GetSyncConfirmationDialogPreferredHeight(browser->profile()),
      GetSyncConfirmationDialogPreferredWidth(browser->profile()));
}

// static
std::unique_ptr<content::WebContents>
SigninViewControllerDelegateMac::CreateSigninErrorWebContents(
    Browser* browser) {
  return CreateDialogWebContents(browser, chrome::kChromeUISigninErrorURL,
                                 kSigninErrorDialogHeight, base::nullopt);
}

// static
std::unique_ptr<content::WebContents>
SigninViewControllerDelegateMac::CreateDialogWebContents(
    Browser* browser,
    const std::string& url,
    int dialog_height,
    base::Optional<int> opt_width) {
  int dialog_width = opt_width.value_or(kModalDialogWidth);
  std::unique_ptr<content::WebContents> web_contents(
      content::WebContents::Create(
          content::WebContents::CreateParams(browser->profile())));
  web_contents->GetController().LoadURL(GURL(url), content::Referrer(),
                                        ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                        std::string());

  SigninWebDialogUI* web_dialog_ui = static_cast<SigninWebDialogUI*>(
      web_contents->GetWebUI()->GetController());
  web_dialog_ui->InitializeMessageHandlerWithBrowser(browser);

  NSView* webview = web_contents->GetNativeView();
  [webview setFrameSize:NSMakeSize(dialog_width, dialog_height)];

  return web_contents;
}

void SigninViewControllerDelegateMac::PerformClose() {
  switch (dialog_modal_type_) {
    case ui::MODAL_TYPE_CHILD:
      if (constrained_window_.get())
        constrained_window_->CloseWebContentsModalDialog();
      break;
    case ui::MODAL_TYPE_WINDOW:
      if (window_.get()) {
        [window_.get().sheetParent endSheet:window_];
        window_.reset(nil);
        CleanupAndDeleteThis();
      }
      break;
    default:
      NOTREACHED() << "Unsupported dialog modal type " << dialog_modal_type_;
  }
}

void SigninViewControllerDelegateMac::ResizeNativeView(int height) {
  if (!window_) {
    window_frame_.size = NSMakeSize(window_frame_.size.width, height);
    DisplayModal();
  }
}

void SigninViewControllerDelegateMac::DisplayModal() {
  DCHECK(!window_);

  content::WebContents* host_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Avoid displaying the sign-in modal view if there are no active web
  // contents. This happens if the user closes the browser window before this
  // dialog has a chance to be displayed.
  if (!host_web_contents)
    return;

  window_.reset(
      [[ConstrainedWindowCustomWindow alloc]
          initWithContentRect:window_frame_]);
  window_.get().contentView = web_contents_->GetNativeView();
  base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
      [[CustomConstrainedWindowSheet alloc] initWithCustomWindow:window_]);
  switch (dialog_modal_type_) {
    case ui::MODAL_TYPE_CHILD:
      constrained_window_ =
          CreateAndShowWebModalDialogMac(this, host_web_contents, sheet);
      break;
    case ui::MODAL_TYPE_WINDOW:
      [host_web_contents->GetTopLevelNativeWindow() beginSheet:window_
                                             completionHandler:nil];
      break;
    default:
      NOTREACHED() << "Unsupported dialog modal type " << dialog_modal_type_;
  }
}

void SigninViewControllerDelegateMac::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  int chrome_command_id = [BrowserWindowUtils getCommandId:event];
  bool can_handle_command = [BrowserWindowUtils isTextEditingEvent:event] ||
                            chrome_command_id == IDC_CLOSE_WINDOW ||
                            chrome_command_id == IDC_EXIT;
  if ([BrowserWindowUtils shouldHandleKeyboardEvent:event] &&
      can_handle_command) {
    [[NSApp mainMenu] performKeyEquivalent:event.os_event];
  }
}

void SigninViewControllerDelegateMac::CleanupAndDeleteThis() {
  ResetSigninViewControllerDelegate();
  delete this;
}

// static
SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateModalSigninDelegateCocoa(
    SigninViewController* signin_view_controller,
    profiles::BubbleViewMode mode,
    Browser* browser,
    signin_metrics::AccessPoint access_point) {
  return new SigninViewControllerDelegateMac(
      signin_view_controller,
      SigninViewControllerDelegateMac::CreateGaiaWebContents(
          nullptr, mode, browser->profile(), access_point),
      browser, NSMakeRect(0, 0, kModalDialogWidth, kFixedGaiaViewHeight),
      ui::MODAL_TYPE_CHILD, false /* wait_for_size */);
}

// static
SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateSyncConfirmationDelegateCocoa(
    SigninViewController* signin_view_controller,
    Browser* browser,
    bool is_consent_bump) {
  return new SigninViewControllerDelegateMac(
      signin_view_controller,
      SigninViewControllerDelegateMac::CreateSyncConfirmationWebContents(
          browser, is_consent_bump),
      browser,
      NSMakeRect(0, 0,
                 GetSyncConfirmationDialogPreferredWidth(browser->profile()),
                 GetSyncConfirmationDialogPreferredHeight(browser->profile())),
      ui::MODAL_TYPE_WINDOW, true /* wait_for_size */);
}

// static
SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateSigninErrorDelegateCocoa(
    SigninViewController* signin_view_controller,
    Browser* browser) {
  return new SigninViewControllerDelegateMac(
      signin_view_controller,
      SigninViewControllerDelegateMac::CreateSigninErrorWebContents(browser),
      browser, NSMakeRect(0, 0, kModalDialogWidth, kSigninErrorDialogHeight),
      ui::MODAL_TYPE_WINDOW, true /* wait_for_size */);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateModalSigninDelegate(
    SigninViewController* signin_view_controller,
    profiles::BubbleViewMode mode,
    Browser* browser,
    signin_metrics::AccessPoint access_point) {
  return CreateModalSigninDelegateCocoa(signin_view_controller, mode, browser,
                                        access_point);
}

SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateSyncConfirmationDelegate(
    SigninViewController* signin_view_controller,
    Browser* browser) {
  return CreateSyncConfirmationDelegateCocoa(signin_view_controller, browser);
}

SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateSigninErrorDelegate(
    SigninViewController* signin_view_controller,
    Browser* browser) {
  return CreateSigninErrorDelegateCocoa(signin_view_controller, browser);
}
#endif
