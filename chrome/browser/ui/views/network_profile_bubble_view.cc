// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/network_profile_bubble.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/layout/fill_layout.h"

namespace {

// Bubble layout constants.
const int kNotificationBubbleWidth = 250;

class NetworkProfileBubbleView : public views::BubbleDialogDelegateView,
                                 public views::LinkListener {
 public:
  NetworkProfileBubbleView(views::View* anchor,
                           content::PageNavigator* navigator,
                           Profile* profile);
 private:
  ~NetworkProfileBubbleView() override;

  // views::BubbleDialogDelegateView:
  void Init() override;
  views::View* CreateExtraView() override;
  int GetDialogButtons() const override;
  bool Accept() override;

  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  // Used for loading pages.
  content::PageNavigator* navigator_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(NetworkProfileBubbleView);
};

////////////////////////////////////////////////////////////////////////////////
// NetworkProfileBubbleView, public:

NetworkProfileBubbleView::NetworkProfileBubbleView(
    views::View* anchor,
    content::PageNavigator* navigator,
    Profile* profile)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_RIGHT),
      navigator_(navigator),
      profile_(profile) {
  // Compensate for built-in vertical padding in the anchor view's image.
  set_anchor_view_insets(gfx::Insets(
      GetLayoutConstant(LOCATION_BAR_BUBBLE_ANCHOR_VERTICAL_INSET), 0));
  chrome::RecordDialogCreation(
      chrome::DialogIdentifier::NETWORK_SHARE_PROFILE_WARNING);
}

////////////////////////////////////////////////////////////////////////////////
// NetworkProfileBubbleView, private:

NetworkProfileBubbleView::~NetworkProfileBubbleView() {
}

void NetworkProfileBubbleView::Init() {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  views::Label* label = new views::Label(
      l10n_util::GetStringFUTF16(IDS_PROFILE_ON_NETWORK_WARNING,
          l10n_util::GetStringUTF16(IDS_PRODUCT_NAME)));
  label->SetMultiLine(true);
  label->SizeToFit(kNotificationBubbleWidth);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(label);
}

views::View* NetworkProfileBubbleView::CreateExtraView() {
  views::Link* learn_more =
      new views::Link(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  learn_more->set_listener(this);
  return learn_more;
}

int NetworkProfileBubbleView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK;
}

bool NetworkProfileBubbleView::Accept() {
  NetworkProfileBubble::RecordUmaEvent(
      NetworkProfileBubble::METRIC_ACKNOWLEDGED);
  return true;
}

void NetworkProfileBubbleView::LinkClicked(views::Link* source,
                                           int event_flags) {
  NetworkProfileBubble::RecordUmaEvent(
      NetworkProfileBubble::METRIC_LEARN_MORE_CLICKED);
  WindowOpenDisposition disposition =
      ui::DispositionFromEventFlags(event_flags);
  content::OpenURLParams params(
      GURL("https://sites.google.com/a/chromium.org/dev/administrators/"
           "common-problems-and-solutions#network_profile"),
      content::Referrer(), disposition == WindowOpenDisposition::CURRENT_TAB
                               ? WindowOpenDisposition::NEW_FOREGROUND_TAB
                               : disposition,
      ui::PAGE_TRANSITION_LINK, false);
  navigator_->OpenURL(params);

  // If the user interacted with the bubble we don't reduce the number of
  // warnings left.
  PrefService* prefs = profile_->GetPrefs();
  int left_warnings = prefs->GetInteger(prefs::kNetworkProfileWarningsLeft);
  prefs->SetInteger(prefs::kNetworkProfileWarningsLeft, ++left_warnings);
  GetWidget()->Close();
}

}  // namespace

// static
void NetworkProfileBubble::ShowNotification(Browser* browser) {
  views::View* anchor = NULL;
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (browser_view && browser_view->toolbar())
    anchor = browser_view->toolbar()->app_menu_button();
  NetworkProfileBubbleView* bubble =
      new NetworkProfileBubbleView(anchor, browser, browser->profile());
  views::BubbleDialogDelegateView::CreateBubble(bubble)->Show();

  NetworkProfileBubble::SetNotificationShown(true);

  // Mark the time of the last bubble and reduce the number of warnings left
  // before the next silence period starts.
  PrefService* prefs = browser->profile()->GetPrefs();
  prefs->SetInt64(prefs::kNetworkProfileLastWarningTime,
                  base::Time::Now().ToTimeT());
  int left_warnings = prefs->GetInteger(prefs::kNetworkProfileWarningsLeft);
  if (left_warnings > 0)
    prefs->SetInteger(prefs::kNetworkProfileWarningsLeft, --left_warnings);
}
