// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/translate_infobar_delegate.h"

#include <algorithm>
#include <utility>

#include "base/i18n/string_compare.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "components/strings/grit/components_strings.h"
#include "components/translate/core/browser/language_state.h"
#include "components/translate/core/browser/translate_accept_languages.h"
#include "components/translate/core/browser/translate_client.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/browser/translate_driver.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/common/translate_constants.h"
#include "ui/base/l10n/l10n_util.h"

namespace translate {

const base::Feature kTranslateCompactUI{"TranslateCompactUI",
                                        base::FEATURE_ENABLED_BY_DEFAULT};

const size_t TranslateInfoBarDelegate::kNoIndex = TranslateUIDelegate::kNoIndex;

TranslateInfoBarDelegate::~TranslateInfoBarDelegate() {
}

infobars::InfoBarDelegate::InfoBarIdentifier
TranslateInfoBarDelegate::GetIdentifier() const {
  return TRANSLATE_INFOBAR_DELEGATE_NON_AURA;
}

// static
void TranslateInfoBarDelegate::Create(
    bool replace_existing_infobar,
    const base::WeakPtr<TranslateManager>& translate_manager,
    infobars::InfoBarManager* infobar_manager,
    bool is_off_the_record,
    translate::TranslateStep step,
    const std::string& original_language,
    const std::string& target_language,
    TranslateErrors::Type error_type,
    bool triggered_from_menu) {
  DCHECK(translate_manager);
  DCHECK(infobar_manager);

  // Check preconditions.
  if (step != translate::TRANSLATE_STEP_TRANSLATE_ERROR) {
    DCHECK(TranslateDownloadManager::IsSupportedLanguage(target_language));
    if (!TranslateDownloadManager::IsSupportedLanguage(original_language)) {
      // The original language can only be "unknown" for the "translating"
      // infobar, which is the case when the user started a translation from the
      // context menu.
      DCHECK(step == translate::TRANSLATE_STEP_TRANSLATING ||
             step == translate::TRANSLATE_STEP_AFTER_TRANSLATE);
      DCHECK_EQ(translate::kUnknownLanguageCode, original_language);
    }
  }

  // Do not create the after translate infobar if we are auto translating.
  if (((step == translate::TRANSLATE_STEP_AFTER_TRANSLATE) ||
       (step == translate::TRANSLATE_STEP_TRANSLATING)) &&
      translate_manager->GetLanguageState().InTranslateNavigation()) {
    return;
  }

  // Find any existing translate infobar delegate.
  infobars::InfoBar* old_infobar = NULL;
  TranslateInfoBarDelegate* old_delegate = NULL;
  for (size_t i = 0; i < infobar_manager->infobar_count(); ++i) {
    old_infobar = infobar_manager->infobar_at(i);
    old_delegate = old_infobar->delegate()->AsTranslateInfoBarDelegate();
    if (old_delegate) {
      if (!replace_existing_infobar)
        return;
      break;
    }
  }

  // Try to reuse existing translate infobar delegate.
  if (old_delegate && old_delegate->observer_) {
    old_delegate->observer_->OnTranslateStepChanged(step, error_type);
    return;
  }

  // Add the new delegate.
  TranslateClient* translate_client = translate_manager->translate_client();
  std::unique_ptr<infobars::InfoBar> infobar(translate_client->CreateInfoBar(
      base::WrapUnique(new TranslateInfoBarDelegate(
          translate_manager, is_off_the_record, step, original_language,
          target_language, error_type, triggered_from_menu))));
  if (old_delegate)
    infobar_manager->ReplaceInfoBar(old_infobar, std::move(infobar));
  else
    infobar_manager->AddInfoBar(std::move(infobar));
}

void TranslateInfoBarDelegate::SetObserver(Observer* observer) {
  observer_ = observer;
}

void TranslateInfoBarDelegate::UpdateOriginalLanguage(
    const std::string& language_code) {
  ui_delegate_.UpdateOriginalLanguage(language_code);
}

void TranslateInfoBarDelegate::UpdateTargetLanguage(
    const std::string& language_code) {
  ui_delegate_.UpdateTargetLanguage(language_code);
}

void TranslateInfoBarDelegate::Translate() {
  ui_delegate_.Translate();
}

void TranslateInfoBarDelegate::RevertTranslation() {
  ui_delegate_.RevertTranslation();
  infobar()->RemoveSelf();
}

void TranslateInfoBarDelegate::RevertWithoutClosingInfobar() {
  ui_delegate_.RevertTranslation();
}

void TranslateInfoBarDelegate::ReportLanguageDetectionError() {
  if (translate_manager_)
    translate_manager_->ReportLanguageDetectionError();
}

void TranslateInfoBarDelegate::TranslationDeclined() {
  ui_delegate_.TranslationDeclined(true);
}

bool TranslateInfoBarDelegate::IsTranslatableLanguageByPrefs() {
  TranslateClient* client = translate_manager_->translate_client();
  std::unique_ptr<TranslatePrefs> translate_prefs(client->GetTranslatePrefs());
  TranslateAcceptLanguages* accept_languages =
      client->GetTranslateAcceptLanguages();
  return translate_prefs->CanTranslateLanguage(accept_languages,
                                               original_language_code());
}

void TranslateInfoBarDelegate::ToggleTranslatableLanguageByPrefs() {
  if (ui_delegate_.IsLanguageBlocked()) {
    ui_delegate_.SetLanguageBlocked(false);
  } else {
    ui_delegate_.SetLanguageBlocked(true);
    infobar()->RemoveSelf();
  }
}

bool TranslateInfoBarDelegate::IsSiteBlacklisted() {
  return ui_delegate_.IsSiteBlacklisted();
}

void TranslateInfoBarDelegate::ToggleSiteBlacklist() {
  if (ui_delegate_.IsSiteBlacklisted()) {
    ui_delegate_.SetSiteBlacklist(false);
  } else {
    ui_delegate_.SetSiteBlacklist(true);
    infobar()->RemoveSelf();
  }
}

bool TranslateInfoBarDelegate::ShouldAlwaysTranslate() {
  return ui_delegate_.ShouldAlwaysTranslate();
}

void TranslateInfoBarDelegate::ToggleAlwaysTranslate() {
  ui_delegate_.SetAlwaysTranslate(!ui_delegate_.ShouldAlwaysTranslate());
}

void TranslateInfoBarDelegate::AlwaysTranslatePageLanguage() {
  DCHECK(!ui_delegate_.ShouldAlwaysTranslate());
  ui_delegate_.SetAlwaysTranslate(true);
  Translate();
}

void TranslateInfoBarDelegate::NeverTranslatePageLanguage() {
  DCHECK(!ui_delegate_.IsLanguageBlocked());
  ui_delegate_.SetLanguageBlocked(true);
  infobar()->RemoveSelf();
}

base::string16 TranslateInfoBarDelegate::GetMessageInfoBarText() {
  if (step_ == translate::TRANSLATE_STEP_TRANSLATING) {
    return l10n_util::GetStringFUTF16(IDS_TRANSLATE_INFOBAR_TRANSLATING_TO,
                                      target_language_name());
  }

  DCHECK_EQ(translate::TRANSLATE_STEP_TRANSLATE_ERROR, step_);
  UMA_HISTOGRAM_ENUMERATION("Translate.ShowErrorInfobar",
                            error_type_,
                            TranslateErrors::TRANSLATE_ERROR_MAX);
  ui_delegate_.OnErrorShown(error_type_);
  switch (error_type_) {
    case TranslateErrors::NETWORK:
      return l10n_util::GetStringUTF16(
          IDS_TRANSLATE_INFOBAR_ERROR_CANT_CONNECT);
    case TranslateErrors::INITIALIZATION_ERROR:
    case TranslateErrors::TRANSLATION_ERROR:
    case TranslateErrors::TRANSLATION_TIMEOUT:
    case TranslateErrors::UNEXPECTED_SCRIPT_ERROR:
    case TranslateErrors::BAD_ORIGIN:
    case TranslateErrors::SCRIPT_LOAD_ERROR:
      return l10n_util::GetStringUTF16(
          IDS_TRANSLATE_INFOBAR_ERROR_CANT_TRANSLATE);
    case TranslateErrors::UNKNOWN_LANGUAGE:
      return l10n_util::GetStringUTF16(
          IDS_TRANSLATE_INFOBAR_UNKNOWN_PAGE_LANGUAGE);
    case TranslateErrors::UNSUPPORTED_LANGUAGE:
      return l10n_util::GetStringFUTF16(
          IDS_TRANSLATE_INFOBAR_UNSUPPORTED_PAGE_LANGUAGE,
          target_language_name());
    case TranslateErrors::IDENTICAL_LANGUAGES:
      return l10n_util::GetStringFUTF16(
          IDS_TRANSLATE_INFOBAR_ERROR_SAME_LANGUAGE, target_language_name());
    default:
      NOTREACHED();
      return base::string16();
  }
}

base::string16 TranslateInfoBarDelegate::GetMessageInfoBarButtonText() {
  if (step_ != translate::TRANSLATE_STEP_TRANSLATE_ERROR) {
    DCHECK_EQ(translate::TRANSLATE_STEP_TRANSLATING, step_);
  } else if ((error_type_ != TranslateErrors::IDENTICAL_LANGUAGES) &&
             (error_type_ != TranslateErrors::UNKNOWN_LANGUAGE)) {
    return l10n_util::GetStringUTF16(
        (error_type_ == TranslateErrors::UNSUPPORTED_LANGUAGE) ?
        IDS_TRANSLATE_INFOBAR_REVERT : IDS_TRANSLATE_INFOBAR_RETRY);
  }
  return base::string16();
}

void TranslateInfoBarDelegate::MessageInfoBarButtonPressed() {
  DCHECK_EQ(translate::TRANSLATE_STEP_TRANSLATE_ERROR, step_);
  if (error_type_ == TranslateErrors::UNSUPPORTED_LANGUAGE) {
    RevertTranslation();
    return;
  }
  // This is the "Try again..." case.
  DCHECK(translate_manager_);
  translate_manager_->TranslatePage(
      original_language_code(), target_language_code(), false);
}

bool TranslateInfoBarDelegate::ShouldShowMessageInfoBarButton() {
  return !GetMessageInfoBarButtonText().empty();
}

bool TranslateInfoBarDelegate::ShouldShowAlwaysTranslateShortcut() {
#if defined(OS_IOS)
  // On mobile, the option to always translate is shown after the translation.
  DCHECK_EQ(translate::TRANSLATE_STEP_AFTER_TRANSLATE, step_);
#else
  // On desktop, the option to always translate is shown before the translation.
  DCHECK_EQ(translate::TRANSLATE_STEP_BEFORE_TRANSLATE, step_);
#endif
  return ui_delegate_.ShouldShowAlwaysTranslateShortcut();
}

bool TranslateInfoBarDelegate::ShouldShowNeverTranslateShortcut() {
  DCHECK_EQ(translate::TRANSLATE_STEP_BEFORE_TRANSLATE, step_);
  return ui_delegate_.ShouldShowNeverTranslateShortcut();
}

#if defined(OS_IOS)
void TranslateInfoBarDelegate::ShowNeverTranslateInfobar() {
  Create(true, translate_manager_, infobar()->owner(), is_off_the_record_,
         translate::TRANSLATE_STEP_NEVER_TRANSLATE, original_language_code(),
         target_language_code(), TranslateErrors::NONE, false);
}
#endif

int TranslateInfoBarDelegate::GetTranslationAcceptedCount() {
  return prefs_->GetTranslationAcceptedCount(original_language_code());
}

int TranslateInfoBarDelegate::GetTranslationDeniedCount() {
  return prefs_->GetTranslationDeniedCount(original_language_code());
}

void TranslateInfoBarDelegate::ResetTranslationAcceptedCount() {
  prefs_->ResetTranslationAcceptedCount(original_language_code());
}

void TranslateInfoBarDelegate::ResetTranslationDeniedCount() {
  prefs_->ResetTranslationDeniedCount(original_language_code());
}

#if defined(OS_ANDROID)
int TranslateInfoBarDelegate::GetTranslationAutoAlwaysCount() {
  return prefs_->GetTranslationAutoAlwaysCount(original_language_code());
}

int TranslateInfoBarDelegate::GetTranslationAutoNeverCount() {
  return prefs_->GetTranslationAutoNeverCount(original_language_code());
}

void TranslateInfoBarDelegate::IncrementTranslationAutoAlwaysCount() {
  prefs_->IncrementTranslationAutoAlwaysCount(original_language_code());
}

void TranslateInfoBarDelegate::IncrementTranslationAutoNeverCount() {
  prefs_->IncrementTranslationAutoNeverCount(original_language_code());
}
#endif

// static
void TranslateInfoBarDelegate::GetAfterTranslateStrings(
    std::vector<base::string16>* strings,
    bool* swap_languages,
    bool autodetermined_source_language) {
  DCHECK(strings);

  if (autodetermined_source_language) {
    size_t offset;
    base::string16 text = l10n_util::GetStringFUTF16(
        IDS_TRANSLATE_INFOBAR_AFTER_MESSAGE_AUTODETERMINED_SOURCE_LANGUAGE,
        base::string16(),
        &offset);

    strings->push_back(text.substr(0, offset));
    strings->push_back(text.substr(offset));
    return;
  }
  DCHECK(swap_languages);

  std::vector<size_t> offsets;
  base::string16 text = l10n_util::GetStringFUTF16(
      IDS_TRANSLATE_INFOBAR_AFTER_MESSAGE, base::string16(), base::string16(),
      &offsets);
  DCHECK_EQ(2U, offsets.size());

  *swap_languages = (offsets[0] > offsets[1]);
  if (*swap_languages)
    std::swap(offsets[0], offsets[1]);

  strings->push_back(text.substr(0, offsets[0]));
  strings->push_back(text.substr(offsets[0], offsets[1] - offsets[0]));
  strings->push_back(text.substr(offsets[1]));
}

TranslateDriver* TranslateInfoBarDelegate::GetTranslateDriver() {
  if (!translate_manager_)
    return NULL;

  return translate_manager_->translate_client()->GetTranslateDriver();
}

TranslateInfoBarDelegate::TranslateInfoBarDelegate(
    const base::WeakPtr<TranslateManager>& translate_manager,
    bool is_off_the_record,
    translate::TranslateStep step,
    const std::string& original_language,
    const std::string& target_language,
    TranslateErrors::Type error_type,
    bool triggered_from_menu)
    : infobars::InfoBarDelegate(),
      is_off_the_record_(is_off_the_record),
      step_(step),
      ui_delegate_(translate_manager, original_language, target_language),
      translate_manager_(translate_manager),
      error_type_(error_type),
      prefs_(translate_manager->translate_client()->GetTranslatePrefs()),
      triggered_from_menu_(triggered_from_menu),
      observer_(nullptr) {
  DCHECK_NE((step_ == translate::TRANSLATE_STEP_TRANSLATE_ERROR),
            (error_type_ == TranslateErrors::NONE));
  DCHECK(translate_manager_);
}

int TranslateInfoBarDelegate::GetIconId() const {
  return translate_manager_->translate_client()->GetInfobarIconID();
}

void TranslateInfoBarDelegate::InfoBarDismissed() {
  bool declined = observer_
                      ? observer_->IsDeclinedByUser()
                      : (step_ == translate::TRANSLATE_STEP_BEFORE_TRANSLATE);

  if (declined) {
    // The user closed the infobar without clicking the translate button.
    TranslationDeclined();
    UMA_HISTOGRAM_BOOLEAN("Translate.DeclineTranslateCloseInfobar", true);
  }
}

TranslateInfoBarDelegate*
    TranslateInfoBarDelegate::AsTranslateInfoBarDelegate() {
  return this;
}

}  // namespace translate
