// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/download/download_feedback_dialog_view.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/supports_user_data.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/security_interstitials/core/urls.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/page_navigator.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/widget/widget.h"

using content::OpenURLParams;

namespace {

const void* const kDialogStatusKey = &kDialogStatusKey;

class DialogStatusData : public base::SupportsUserData::Data {
 public:
  DialogStatusData() : currently_shown_(false) {}
  ~DialogStatusData() override {}
  bool currently_shown() const { return currently_shown_; }
  void set_currently_shown(bool shown) { currently_shown_ = shown; }
 private:
  bool currently_shown_;
};

}  // namespace

// static
void DownloadFeedbackDialogView::Show(
    gfx::NativeWindow parent_window,
    Profile* profile,
    content::PageNavigator* navigator,
    const UserDecisionCallback& callback) {
  // This dialog should only be shown if it hasn't been shown before.
  DCHECK(!safe_browsing::ExtendedReportingPrefExists(*profile->GetPrefs()));

  // Determine if any prefs need to be updated prior to showing the dialog.
  safe_browsing::UpdatePrefsBeforeSecurityInterstitial(profile->GetPrefs());

  // Only one dialog should be shown at a time, so check to see if another one
  // is open. If another one is open, treat this parallel call as if reporting
  // is disabled (to be conservative).
  DialogStatusData* data =
      static_cast<DialogStatusData*>(profile->GetUserData(kDialogStatusKey));
  if (data == NULL) {
    data = new DialogStatusData();
    profile->SetUserData(kDialogStatusKey, base::WrapUnique(data));
  }
  if (data->currently_shown() == false) {
    data->set_currently_shown(true);
    DownloadFeedbackDialogView* window =
        new DownloadFeedbackDialogView(profile, navigator, callback);
    constrained_window::CreateBrowserModalDialogViews(
        window, parent_window)->Show();
  } else {
    callback.Run(false);
  }
}

DownloadFeedbackDialogView::DownloadFeedbackDialogView(
    Profile* profile,
    content::PageNavigator* navigator,
    const UserDecisionCallback& callback)
    : profile_(profile),
      navigator_(navigator),
      callback_(callback),
      explanation_box_view_(
          new views::MessageBoxView(views::MessageBoxView::InitParams(
              l10n_util::GetStringUTF16(safe_browsing::ChooseOptInTextResource(
                  *profile->GetPrefs(),
                  IDS_FEEDBACK_SERVICE_DIALOG_EXPLANATION,
                  IDS_FEEDBACK_SERVICE_DIALOG_EXPLANATION_SCOUT))))),
      link_view_(new views::Link(
          l10n_util::GetStringUTF16(IDS_SAFE_BROWSING_PRIVACY_POLICY_PAGE))),
      title_text_(l10n_util::GetStringUTF16(IDS_FEEDBACK_SERVICE_DIALOG_TITLE)),
      ok_button_text_(l10n_util::GetStringUTF16(
          IDS_FEEDBACK_SERVICE_DIALOG_OK_BUTTON_LABEL)),
      cancel_button_text_(l10n_util::GetStringUTF16(
          IDS_FEEDBACK_SERVICE_DIALOG_CANCEL_BUTTON_LABEL)) {
  link_view_->set_listener(this);
  chrome::RecordDialogCreation(
      chrome::DialogIdentifier::SAFE_BROWSING_DOWNLOAD_FEEDBACK);
}

DownloadFeedbackDialogView::~DownloadFeedbackDialogView() {}

int DownloadFeedbackDialogView::GetDefaultDialogButton() const {
  return ui::DIALOG_BUTTON_CANCEL;
}

base::string16 DownloadFeedbackDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return (button == ui::DIALOG_BUTTON_OK) ?
      ok_button_text_ : cancel_button_text_;
}

bool DownloadFeedbackDialogView::OnButtonClicked(bool accepted) {
  safe_browsing::SetExtendedReportingPrefAndMetric(
      profile_->GetPrefs(), accepted,
      safe_browsing::SBER_OPTIN_SITE_DOWNLOAD_FEEDBACK_POPUP);
  DialogStatusData* data =
     static_cast<DialogStatusData*>(profile_->GetUserData(kDialogStatusKey));
  DCHECK(data);
  data->set_currently_shown(false);

  UMA_HISTOGRAM_BOOLEAN("Download.FeedbackDialogEnabled", accepted);

  callback_.Run(accepted);
  return true;
}

bool DownloadFeedbackDialogView::Cancel() {
  return OnButtonClicked(false);
}

bool DownloadFeedbackDialogView::Accept() {
  return OnButtonClicked(true);
}

ui::ModalType DownloadFeedbackDialogView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 DownloadFeedbackDialogView::GetWindowTitle() const {
  return title_text_;
}

void DownloadFeedbackDialogView::DeleteDelegate() {
  delete this;
}

views::Widget* DownloadFeedbackDialogView::GetWidget() {
  return explanation_box_view_->GetWidget();
}

const views::Widget* DownloadFeedbackDialogView::GetWidget() const {
  return explanation_box_view_->GetWidget();
}

views::View* DownloadFeedbackDialogView::GetContentsView() {
  return explanation_box_view_;
}

views::View* DownloadFeedbackDialogView::CreateExtraView() {
  return link_view_;
}

void DownloadFeedbackDialogView::LinkClicked(
    views::Link* source, int event_flags) {
  WindowOpenDisposition disposition =
      ui::DispositionFromEventFlags(event_flags);
  content::OpenURLParams params(
      GURL(security_interstitials::kSafeBrowsingPrivacyPolicyUrl),
      content::Referrer(),
      disposition == WindowOpenDisposition::CURRENT_TAB
          ? WindowOpenDisposition::NEW_FOREGROUND_TAB
          : disposition,
      ui::PAGE_TRANSITION_LINK, false);
  navigator_->OpenURL(params);
}
