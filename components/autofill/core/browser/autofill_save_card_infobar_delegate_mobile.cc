// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_save_card_infobar_delegate_mobile.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/legal_message_line.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/grit/components_scaled_resources.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace autofill {

AutofillSaveCardInfoBarDelegateMobile::AutofillSaveCardInfoBarDelegateMobile(
    bool upload,
    const CreditCard& card,
    std::unique_ptr<base::DictionaryValue> legal_message,
    const base::Closure& save_card_callback,
    PrefService* pref_service)
    : ConfirmInfoBarDelegate(),
      upload_(upload),
      save_card_callback_(save_card_callback),
      pref_service_(pref_service),
      had_user_interaction_(false),
      issuer_icon_id_(CreditCard::IconResourceId(card.network())),
      card_label_(card.NetworkAndLastFourDigits()),
      card_sub_label_(card.AbbreviatedExpirationDateForDisplay()) {
  if (legal_message) {
    if (!LegalMessageLine::Parse(*legal_message, &legal_messages_,
                                 /*escape_apostrophes=*/true)) {
      AutofillMetrics::LogCreditCardInfoBarMetric(
          AutofillMetrics::INFOBAR_NOT_SHOWN_INVALID_LEGAL_MESSAGE, upload_,
          pref_service_->GetInteger(
              prefs::kAutofillAcceptSaveCreditCardPromptState));
      return;
    }
  }

  AutofillMetrics::LogCreditCardInfoBarMetric(
      AutofillMetrics::INFOBAR_SHOWN, upload_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));
}

AutofillSaveCardInfoBarDelegateMobile::
    ~AutofillSaveCardInfoBarDelegateMobile() {
  if (!had_user_interaction_)
    LogUserAction(AutofillMetrics::INFOBAR_IGNORED);
}

void AutofillSaveCardInfoBarDelegateMobile::OnLegalMessageLinkClicked(
    GURL url) {
  infobar()->owner()->OpenURL(url, WindowOpenDisposition::NEW_FOREGROUND_TAB);
}

bool AutofillSaveCardInfoBarDelegateMobile::LegalMessagesParsedSuccessfully() {
  // If we are uploading to the server, verify that legal lines have been parsed
  // into |legal_messages_|.
  return !upload_ || !legal_messages_.empty();
}

bool AutofillSaveCardInfoBarDelegateMobile::IsGooglePayBrandingEnabled() const {
  return upload_ &&
         base::FeatureList::IsEnabled(
             features::kAutofillUpstreamUseGooglePayBrandingOnMobile);
}

base::string16 AutofillSaveCardInfoBarDelegateMobile::GetDescriptionText()
    const {
  // Without Google Pay branding, the title acts as the description (see
  // |GetMessageText|).
  if (!IsGooglePayBrandingEnabled())
    return base::string16();

  return IsAutofillUpstreamUpdatePromptExplanationExperimentEnabled()
             ? l10n_util::GetStringUTF16(
                   IDS_AUTOFILL_SAVE_CARD_PROMPT_UPLOAD_EXPLANATION_V3)
             : l10n_util::GetStringUTF16(
                   IDS_AUTOFILL_SAVE_CARD_PROMPT_UPLOAD_EXPLANATION_V2);
}

int AutofillSaveCardInfoBarDelegateMobile::GetIconId() const {
  return IsGooglePayBrandingEnabled() ? IDR_AUTOFILL_GOOGLE_PAY_WITH_DIVIDER
                                      : IDR_INFOBAR_AUTOFILL_CC;
}

base::string16 AutofillSaveCardInfoBarDelegateMobile::GetMessageText() const {
  return l10n_util::GetStringUTF16(
      IsGooglePayBrandingEnabled() || !upload_
          ? IDS_AUTOFILL_SAVE_CARD_PROMPT_TITLE_TO_CLOUD_V3
          : IDS_AUTOFILL_SAVE_CARD_PROMPT_TITLE_TO_CLOUD);
}

infobars::InfoBarDelegate::InfoBarIdentifier
AutofillSaveCardInfoBarDelegateMobile::GetIdentifier() const {
  return AUTOFILL_CC_INFOBAR_DELEGATE_MOBILE;
}

bool AutofillSaveCardInfoBarDelegateMobile::ShouldExpire(
    const NavigationDetails& details) const {
  // The user has submitted a form, causing the page to navigate elsewhere. We
  // don't want the infobar to be expired at this point, because the user won't
  // get a chance to answer the question.
  return false;
}

void AutofillSaveCardInfoBarDelegateMobile::InfoBarDismissed() {
  LogUserAction(AutofillMetrics::INFOBAR_DENIED);
}

int AutofillSaveCardInfoBarDelegateMobile::GetButtons() const {
  return BUTTON_OK;
}

base::string16 AutofillSaveCardInfoBarDelegateMobile::GetButtonLabel(
    InfoBarButton button) const {
  if (button != BUTTON_OK) {
    NOTREACHED() << "Unsupported button label requested.";
    return base::string16();
  }

  return l10n_util::GetStringUTF16(IDS_AUTOFILL_SAVE_CARD_PROMPT_ACCEPT);
}

bool AutofillSaveCardInfoBarDelegateMobile::Accept() {
  save_card_callback_.Run();
  save_card_callback_.Reset();
  LogUserAction(AutofillMetrics::INFOBAR_ACCEPTED);
  return true;
}

void AutofillSaveCardInfoBarDelegateMobile::LogUserAction(
    AutofillMetrics::InfoBarMetric user_action) {
  DCHECK(!had_user_interaction_);

  AutofillMetrics::LogCreditCardInfoBarMetric(
      user_action, upload_,
      pref_service_->GetInteger(
          prefs::kAutofillAcceptSaveCreditCardPromptState));
  pref_service_->SetInteger(
      prefs::kAutofillAcceptSaveCreditCardPromptState,
      user_action == AutofillMetrics::INFOBAR_ACCEPTED
          ? prefs::PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISION_ACCEPTED
          : prefs::PREVIOUS_SAVE_CREDIT_CARD_PROMPT_USER_DECISION_DENIED);
  had_user_interaction_ = true;
}

}  // namespace autofill
