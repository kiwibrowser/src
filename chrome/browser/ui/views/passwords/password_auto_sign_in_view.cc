// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/passwords/password_auto_sign_in_view.h"

#include "build/build_config.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/passwords/password_dialog_prompts.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/passwords/credentials_item_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/layout/fill_layout.h"

#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
#include "chrome/browser/ui/views/frame/browser_view.h"
#endif

int PasswordAutoSignInView::auto_signin_toast_timeout_ = 3;

PasswordAutoSignInView::~PasswordAutoSignInView() = default;

PasswordAutoSignInView::PasswordAutoSignInView(
    content::WebContents* web_contents,
    views::View* anchor_view,
    const gfx::Point& anchor_point,
    DisplayReason reason)
    : PasswordBubbleViewBase(web_contents, anchor_view, anchor_point, reason) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  const autofill::PasswordForm& form = model()->pending_password();
  base::string16 upper_text, lower_text = form.username_value;

  set_margins(
      ChromeLayoutProvider::Get()->GetInsetsMetric(views::INSETS_DIALOG));

  if (ChromeLayoutProvider::Get()->IsHarmonyMode()) {
    upper_text =
        l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_AUTO_SIGNIN_TITLE_MD);
  } else {
    lower_text = l10n_util::GetStringFUTF16(
        IDS_MANAGE_PASSWORDS_AUTO_SIGNIN_TITLE, lower_text);
  }
  CredentialsItemView* credential = new CredentialsItemView(
      this, upper_text, lower_text, kButtonHoverColor, &form,
      content::BrowserContext::GetDefaultStoragePartition(model()->GetProfile())
          ->GetURLLoaderFactoryForBrowserProcess()
          .get());
  credential->SetEnabled(false);
  AddChildView(credential);

  // Setup the observer and maybe start the timer.
  Browser* browser = chrome::FindBrowserWithWebContents(GetWebContents());
  DCHECK(browser);

  // Sign-in dialogs opened for inactive browser windows do not auto-close on
  // MacOS. This matches existing Cocoa bubble behavior.
  // TODO(varkha): Remove the limitation as part of http://crbug/671916 .
  if (browser->window()->IsActive()) {
    timer_.Start(FROM_HERE, GetTimeout(), this,
                 &PasswordAutoSignInView::OnTimer);
  }
}

void PasswordAutoSignInView::OnWidgetActivationChanged(views::Widget* widget,
                                                       bool active) {
  if (active && !timer_.IsRunning()) {
    timer_.Start(FROM_HERE, GetTimeout(), this,
                 &PasswordAutoSignInView::OnTimer);
  }
  LocationBarBubbleDelegateView::OnWidgetActivationChanged(widget, active);
}

int PasswordAutoSignInView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

gfx::Size PasswordAutoSignInView::CalculatePreferredSize() const {
  const int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
                        DISTANCE_BUBBLE_PREFERRED_WIDTH) -
                    margins().width();
  return gfx::Size(width, GetHeightForWidth(width));
}

void PasswordAutoSignInView::ButtonPressed(views::Button* sender,
                                           const ui::Event& event) {
  NOTREACHED();
}

void PasswordAutoSignInView::OnTimer() {
  model()->OnAutoSignInToastTimeout();
  CloseBubble();
}

base::TimeDelta PasswordAutoSignInView::GetTimeout() {
  return base::TimeDelta::FromSeconds(
      PasswordAutoSignInView::auto_signin_toast_timeout_);
}
