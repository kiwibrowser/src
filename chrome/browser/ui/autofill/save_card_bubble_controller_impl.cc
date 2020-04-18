// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/save_card_bubble_controller_impl.h"

#include <stddef.h>

#include "chrome/browser/ui/autofill/save_card_bubble_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_handle.h"
#include "ui/base/l10n/l10n_util.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(autofill::SaveCardBubbleControllerImpl);

namespace autofill {

namespace {

// Number of seconds the bubble and icon will survive navigations, starting
// from when the bubble is shown.
// TODO(bondd): Share with ManagePasswordsUIController.
const int kSurviveNavigationSeconds = 5;

}  // namespace

SaveCardBubbleControllerImpl::SaveCardBubbleControllerImpl(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      save_card_bubble_view_(nullptr),
      pref_service_(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())) {}

SaveCardBubbleControllerImpl::~SaveCardBubbleControllerImpl() {
  if (save_card_bubble_view_)
    save_card_bubble_view_->Hide();
}

void SaveCardBubbleControllerImpl::ShowBubbleForLocalSave(
    const CreditCard& card,
    const base::Closure& save_card_callback) {
  // Don't show the bubble if it's already visible.
  if (save_card_bubble_view_)
    return;

  is_uploading_ = false;
  is_reshow_ = false;
  legal_message_lines_.clear();

  AutofillMetrics::LogSaveCardPromptMetric(
      AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, is_uploading_,
      is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));

  card_ = card;
  save_card_callback_ = save_card_callback;
  ShowBubble();
}

void SaveCardBubbleControllerImpl::ShowBubbleForUpload(
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    const base::Closure& save_card_callback) {
  // Don't show the bubble if it's already visible.
  if (save_card_bubble_view_)
    return;

  is_uploading_ = true;
  is_reshow_ = false;
  AutofillMetrics::LogSaveCardPromptMetric(
      AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, is_uploading_,
      is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));

  if (!LegalMessageLine::Parse(*legal_message, &legal_message_lines_,
                               /*escape_apostrophes=*/true)) {
    AutofillMetrics::LogSaveCardPromptMetric(
        AutofillMetrics::SAVE_CARD_PROMPT_END_INVALID_LEGAL_MESSAGE,
        is_uploading_, is_reshow_,
        pref_service_->GetInteger(
            prefs::kAutofillAcceptSaveCreditCardPromptState));
    return;
  }

  card_ = card;
  save_card_callback_ = save_card_callback;
  ShowBubble();
}

void SaveCardBubbleControllerImpl::HideBubble() {
  if (save_card_bubble_view_) {
    save_card_bubble_view_->Hide();
    save_card_bubble_view_ = nullptr;
  }
}

void SaveCardBubbleControllerImpl::ReshowBubble() {
  // Don't show the bubble if it's already visible.
  if (save_card_bubble_view_)
    return;

  is_reshow_ = true;
  AutofillMetrics::LogSaveCardPromptMetric(
      AutofillMetrics::SAVE_CARD_PROMPT_SHOW_REQUESTED, is_uploading_,
      is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));

  ShowBubble();
}

bool SaveCardBubbleControllerImpl::IsIconVisible() const {
  return !save_card_callback_.is_null();
}

SaveCardBubbleView* SaveCardBubbleControllerImpl::save_card_bubble_view()
    const {
  return save_card_bubble_view_;
}

base::string16 SaveCardBubbleControllerImpl::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(
      IDS_AUTOFILL_SAVE_CARD_PROMPT_TITLE_TO_CLOUD_V3);
}

base::string16 SaveCardBubbleControllerImpl::GetExplanatoryMessage() const {
  if (is_uploading_) {
    return IsAutofillUpstreamUpdatePromptExplanationExperimentEnabled()
               ? l10n_util::GetStringUTF16(
                     IDS_AUTOFILL_SAVE_CARD_PROMPT_UPLOAD_EXPLANATION_V3)
               : l10n_util::GetStringUTF16(
                     IDS_AUTOFILL_SAVE_CARD_PROMPT_UPLOAD_EXPLANATION_V2);
  }
  return base::string16();
}

const CreditCard SaveCardBubbleControllerImpl::GetCard() const {
  return card_;
}

void SaveCardBubbleControllerImpl::OnSaveButton() {
  save_card_callback_.Run();
  save_card_callback_.Reset();
  AutofillMetrics::LogSaveCardPromptMetric(
      AutofillMetrics::SAVE_CARD_PROMPT_END_ACCEPTED, is_uploading_, is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));
  pref_service_->SetInteger(
      prefs::kAutofillAcceptSaveCreditCardPromptState,
      prefs::PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISION_ACCEPTED);
}

void SaveCardBubbleControllerImpl::OnCancelButton() {
  save_card_callback_.Reset();
  AutofillMetrics::LogSaveCardPromptMetric(
      AutofillMetrics::SAVE_CARD_PROMPT_END_DENIED, is_uploading_, is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));
  pref_service_->SetInteger(
      prefs::kAutofillAcceptSaveCreditCardPromptState,
      prefs::PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISION_DENIED);
}

void SaveCardBubbleControllerImpl::OnLearnMoreClicked() {
  OpenUrl(GURL(kHelpURL));
  AutofillMetrics::LogSaveCardPromptMetric(
      AutofillMetrics::SAVE_CARD_PROMPT_DISMISS_CLICK_LEARN_MORE, is_uploading_,
      is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));
}

void SaveCardBubbleControllerImpl::OnLegalMessageLinkClicked(const GURL& url) {
  OpenUrl(url);
  AutofillMetrics::LogSaveCardPromptMetric(
      AutofillMetrics::SAVE_CARD_PROMPT_DISMISS_CLICK_LEGAL_MESSAGE,
      is_uploading_, is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));
}

void SaveCardBubbleControllerImpl::OnBubbleClosed() {
  save_card_bubble_view_ = nullptr;
  UpdateIcon();
}

const LegalMessageLines& SaveCardBubbleControllerImpl::GetLegalMessageLines()
    const {
  return legal_message_lines_;
}

base::TimeDelta SaveCardBubbleControllerImpl::Elapsed() const {
  return timer_->Elapsed();
}

void SaveCardBubbleControllerImpl::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;

  // Nothing to do if there's no bubble available.
  if (save_card_callback_.is_null())
    return;

  // Don't react to same-document (fragment) navigations.
  if (navigation_handle->IsSameDocument())
    return;

  // Don't do anything if a navigation occurs before a user could reasonably
  // interact with the bubble.
  if (Elapsed() < base::TimeDelta::FromSeconds(kSurviveNavigationSeconds))
    return;

  // Otherwise, get rid of the bubble and icon.
  save_card_callback_.Reset();
  bool bubble_was_visible = save_card_bubble_view_;
  if (bubble_was_visible) {
    save_card_bubble_view_->Hide();
    OnBubbleClosed();
  } else {
    UpdateIcon();
  }
  AutofillMetrics::LogSaveCardPromptMetric(
      bubble_was_visible
          ? AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_SHOWING
          : AutofillMetrics::SAVE_CARD_PROMPT_END_NAVIGATION_HIDDEN,
      is_uploading_, is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));
}

void SaveCardBubbleControllerImpl::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::HIDDEN)
    HideBubble();
}

void SaveCardBubbleControllerImpl::WebContentsDestroyed() {
  HideBubble();
}

void SaveCardBubbleControllerImpl::ShowBubble() {
  DCHECK(!save_card_callback_.is_null());
  DCHECK(!save_card_bubble_view_);

  // Need to create location bar icon before bubble, otherwise bubble will be
  // unanchored.
  UpdateIcon();

  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  save_card_bubble_view_ = browser->window()->ShowSaveCreditCardBubble(
      web_contents(), this, is_reshow_);
  DCHECK(save_card_bubble_view_);

  // Update icon after creating |save_card_bubble_view_| so that icon will show
  // its "toggled on" state.
  UpdateIcon();

  timer_.reset(new base::ElapsedTimer());

  AutofillMetrics::LogSaveCardPromptMetric(
      AutofillMetrics::SAVE_CARD_PROMPT_SHOWN, is_uploading_, is_reshow_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));
}

void SaveCardBubbleControllerImpl::UpdateIcon() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  if (!browser)
    return;
  LocationBar* location_bar = browser->window()->GetLocationBar();
  location_bar->UpdateSaveCreditCardIcon();
}

void SaveCardBubbleControllerImpl::OpenUrl(const GURL& url) {
  web_contents()->OpenURL(content::OpenURLParams(
      url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_LINK, false));
}

}  // namespace autofill
