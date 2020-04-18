// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_autofill_manager.h"

#include <stddef.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/suggestion.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/autofill_data_validation.h"
#include "components/autofill/core/common/autofill_util.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_driver.h"
#include "components/password_manager/core/browser/password_manager_metrics_recorder.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/security_state/core/security_state.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif

namespace password_manager {

namespace {

// Returns |username| unless it is empty. For an empty |username| returns a
// localised string saying this username is empty. Use this for displaying the
// usernames to the user.
base::string16 ReplaceEmptyUsername(const base::string16& username) {
  if (username.empty())
    return l10n_util::GetStringUTF16(IDS_PASSWORD_MANAGER_EMPTY_LOGIN);
  return username;
}

// Returns the prettified version of |signon_realm| to be displayed on the UI.
base::string16 GetHumanReadableRealm(const std::string& signon_realm) {
  // For Android application realms, remove the hash component. Otherwise, make
  // no changes.
  FacetURI maybe_facet_uri(FacetURI::FromPotentiallyInvalidSpec(signon_realm));
  if (maybe_facet_uri.IsValidAndroidFacetURI())
    return base::UTF8ToUTF16("android://" +
                             maybe_facet_uri.android_package_name() + "/");
  return base::UTF8ToUTF16(signon_realm);
}

// If |suggestion| was made for an empty username, then return the empty
// string, otherwise return |suggestion|.
base::string16 GetUsernameFromSuggestion(const base::string16& suggestion) {
  return suggestion ==
                 l10n_util::GetStringUTF16(IDS_PASSWORD_MANAGER_EMPTY_LOGIN)
             ? base::string16()
             : suggestion;
}

// If |field_suggestion| matches |field_content|, creates a Suggestion out of it
// and appends to |suggestions|.
void AppendSuggestionIfMatching(
    const base::string16& field_suggestion,
    const base::string16& field_contents,
    const std::string& signon_realm,
    bool show_all,
    bool is_password_field,
    std::vector<autofill::Suggestion>* suggestions) {
  base::string16 lower_suggestion = base::i18n::ToLower(field_suggestion);
  base::string16 lower_contents = base::i18n::ToLower(field_contents);
  bool prefix_matched_suggestion =
      show_all || base::StartsWith(lower_suggestion, lower_contents,
                                   base::CompareCase::SENSITIVE);
  if (prefix_matched_suggestion ||
      autofill::FieldIsSuggestionSubstringStartingOnTokenBoundary(
          lower_suggestion, lower_contents, true)) {
    autofill::Suggestion suggestion(ReplaceEmptyUsername(field_suggestion));
    suggestion.label = GetHumanReadableRealm(signon_realm);
    suggestion.frontend_id = is_password_field
                                 ? autofill::POPUP_ITEM_ID_PASSWORD_ENTRY
                                 : autofill::POPUP_ITEM_ID_USERNAME_ENTRY;
    suggestion.match = prefix_matched_suggestion
                           ? autofill::Suggestion::PREFIX_MATCH
                           : autofill::Suggestion::SUBSTRING_MATCH;
    suggestions->push_back(suggestion);
  }
}

// This function attempts to fill |suggestions| and |realms| form |fill_data|
// based on |current_username|. Unless |show_all| is true, it only picks
// suggestions where the username has |current_username| as a prefix.
void GetSuggestions(const autofill::PasswordFormFillData& fill_data,
                    const base::string16& current_username,
                    std::vector<autofill::Suggestion>* suggestions,
                    bool show_all,
                    bool is_password_field) {
  AppendSuggestionIfMatching(fill_data.username_field.value, current_username,
                             fill_data.preferred_realm, show_all,
                             is_password_field, suggestions);

  for (const auto& login : fill_data.additional_logins) {
    AppendSuggestionIfMatching(login.first, current_username,
                               login.second.realm, show_all, is_password_field,
                               suggestions);
  }

  // Prefix matches should precede other token matches.
  if (autofill::IsFeatureSubstringMatchEnabled()) {
    std::sort(suggestions->begin(), suggestions->end(),
              [](const autofill::Suggestion& a, const autofill::Suggestion& b) {
                return a.match < b.match;
              });
  }
}

bool ShouldShowManualFallbackForPreLollipop(syncer::SyncService* sync_service) {
#if defined(OS_ANDROID)
  return ((base::android::BuildInfo::GetInstance()->sdk_int() >=
           base::android::SDK_VERSION_LOLLIPOP) ||
          (password_manager_util::GetPasswordSyncState(sync_service) ==
           SYNCING_NORMAL_ENCRYPTION));
#else
  return true;
#endif
}

void AddSimpleSuggestionWithSeparatorOnTop(
    int value,
    int frontend_id,
    std::vector<autofill::Suggestion>* suggestions) {
#if !defined(OS_ANDROID)
  suggestions->push_back(autofill::Suggestion());
  suggestions->back().frontend_id = autofill::POPUP_ITEM_ID_SEPARATOR;
#endif

  autofill::Suggestion suggestion(l10n_util::GetStringUTF8(value),
                                  std::string(), std::string(), frontend_id);
  suggestions->push_back(suggestion);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// PasswordAutofillManager, public:

PasswordAutofillManager::PasswordAutofillManager(
    PasswordManagerDriver* password_manager_driver,
    autofill::AutofillClient* autofill_client,
    PasswordManagerClient* password_client)
    : form_data_key_(-1),
      password_manager_driver_(password_manager_driver),
      autofill_client_(autofill_client),
      password_client_(password_client),
      weak_ptr_factory_(this) {}

PasswordAutofillManager::~PasswordAutofillManager() {
  if (deletion_callback_)
    std::move(deletion_callback_).Run();
}

bool PasswordAutofillManager::FillSuggestion(int key,
                                             const base::string16& username) {
  autofill::PasswordFormFillData fill_data;
  autofill::PasswordAndRealm password_and_realm;
  if (FindLoginInfo(key, &fill_data) &&
      GetPasswordAndRealmForUsername(
          username, fill_data, &password_and_realm)) {
    bool is_android_credential = FacetURI::FromPotentiallyInvalidSpec(
        password_and_realm.realm).IsValidAndroidFacetURI();
    metrics_util::LogFilledCredentialIsFromAndroidApp(is_android_credential);
    password_manager_driver_->FillSuggestion(
        username, password_and_realm.password);
    return true;
  }
  return false;
}

bool PasswordAutofillManager::PreviewSuggestion(
    int key,
    const base::string16& username) {
  autofill::PasswordFormFillData fill_data;
  autofill::PasswordAndRealm password_and_realm;
  if (FindLoginInfo(key, &fill_data) &&
      GetPasswordAndRealmForUsername(
          username, fill_data, &password_and_realm)) {
    password_manager_driver_->PreviewSuggestion(
        username, password_and_realm.password);
    return true;
  }
  return false;
}

void PasswordAutofillManager::OnAddPasswordFormMapping(
    int key,
    const autofill::PasswordFormFillData& fill_data) {
  if (!autofill::IsValidPasswordFormFillData(fill_data))
    return;

  login_to_password_info_[key] = fill_data;
}

void PasswordAutofillManager::OnShowPasswordSuggestions(
    int key,
    base::i18n::TextDirection text_direction,
    const base::string16& typed_username,
    int options,
    const gfx::RectF& bounds) {
  std::vector<autofill::Suggestion> suggestions;
  LoginToPasswordInfoMap::const_iterator fill_data_it =
      login_to_password_info_.find(key);
  if (fill_data_it == login_to_password_info_.end()) {
    // Probably a compromised renderer.
    NOTREACHED();
    return;
  }
  GetSuggestions(fill_data_it->second, typed_username, &suggestions,
                 (options & autofill::SHOW_ALL) != 0,
                 (options & autofill::IS_PASSWORD_FIELD) != 0);

  form_data_key_ = key;

  if (suggestions.empty()) {
    autofill_client_->HideAutofillPopup();
    return;
  }

  if (options & autofill::IS_PASSWORD_FIELD) {
    autofill::Suggestion password_field_suggestions(l10n_util::GetStringUTF16(
        IDS_AUTOFILL_PASSWORD_FIELD_SUGGESTIONS_TITLE));
    password_field_suggestions.frontend_id = autofill::POPUP_ITEM_ID_TITLE;
    suggestions.insert(suggestions.begin(), password_field_suggestions);
  }

  GURL origin = (fill_data_it->second).origin;

  if (ShouldShowManualFallbackForPreLollipop(
          autofill_client_->GetSyncService())) {
    if (base::FeatureList::IsEnabled(
            password_manager::features::kManualFallbacksFilling) &&
        (options & autofill::IS_PASSWORD_FIELD) && password_client_ &&
        password_client_->IsFillingFallbackEnabledForCurrentPage()) {
      AddSimpleSuggestionWithSeparatorOnTop(
          IDS_AUTOFILL_SHOW_ALL_SAVED_FALLBACK,
          autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY, &suggestions);

      show_all_saved_passwords_shown_context_ =
          metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_PASSWORD;
      metrics_util::LogContextOfShowAllSavedPasswordsShown(
          show_all_saved_passwords_shown_context_);
    }
    if (base::FeatureList::IsEnabled(
            password_manager::features::kEnableManualFallbacksGeneration) &&
        password_manager_util::GetPasswordSyncState(
            autofill_client_->GetSyncService()) == SYNCING_NORMAL_ENCRYPTION) {
      AddSimpleSuggestionWithSeparatorOnTop(
          IDS_AUTOFILL_GENERATE_PASSWORD_FALLBACK,
          autofill::POPUP_ITEM_ID_GENERATE_PASSWORD_ENTRY, &suggestions);
    }
  }

  autofill_client_->ShowAutofillPopup(bounds,
                                      text_direction,
                                      suggestions,
                                      weak_ptr_factory_.GetWeakPtr());
}

void PasswordAutofillManager::OnShowManualFallbackSuggestion(
    base::i18n::TextDirection text_direction,
    const gfx::RectF& bounds) {
  // https://crbug.com/699197
  // CroS SimpleWebviewDialog used for the captive portal dialog is a special
  // case because it doesn't instantiate many helper classes. |autofill_client_|
  // is NULL too.
  if (!autofill_client_ || !ShouldShowManualFallbackForPreLollipop(
                               autofill_client_->GetSyncService()))
    return;
  if (!password_client_ ||
      !password_client_->IsFillingFallbackEnabledForCurrentPage())
    return;
  std::vector<autofill::Suggestion> suggestions;
  autofill::Suggestion all_saved_passwords(
      l10n_util::GetStringUTF8(IDS_AUTOFILL_SHOW_ALL_SAVED_FALLBACK),
      std::string(), std::string(),
      autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY);
  suggestions.push_back(all_saved_passwords);

  show_all_saved_passwords_shown_context_ =
      metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_MANUAL_FALLBACK;
  metrics_util::LogContextOfShowAllSavedPasswordsShown(
      show_all_saved_passwords_shown_context_);

  if (base::FeatureList::IsEnabled(
          password_manager::features::kEnableManualFallbacksGeneration) &&
      password_manager_util::GetPasswordSyncState(
          autofill_client_->GetSyncService()) == SYNCING_NORMAL_ENCRYPTION) {
    AddSimpleSuggestionWithSeparatorOnTop(
        IDS_AUTOFILL_GENERATE_PASSWORD_FALLBACK,
        autofill::POPUP_ITEM_ID_GENERATE_PASSWORD_ENTRY, &suggestions);
  }
  autofill_client_->ShowAutofillPopup(bounds, text_direction, suggestions,
                                      weak_ptr_factory_.GetWeakPtr());
}

void PasswordAutofillManager::DidNavigateMainFrame() {
  login_to_password_info_.clear();
  did_show_form_not_secure_warning_ = false;
}

bool PasswordAutofillManager::FillSuggestionForTest(
    int key,
    const base::string16& username) {
  return FillSuggestion(key, username);
}

bool PasswordAutofillManager::PreviewSuggestionForTest(
    int key,
    const base::string16& username) {
  return PreviewSuggestion(key, username);
}

void PasswordAutofillManager::OnPopupShown() {
}

void PasswordAutofillManager::OnPopupHidden() {
}

void PasswordAutofillManager::DidSelectSuggestion(const base::string16& value,
                                                  int identifier) {
  ClearPreviewedForm();
  if (identifier == autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY ||
      identifier == autofill::POPUP_ITEM_ID_GENERATE_PASSWORD_ENTRY)
    return;
  bool success =
      PreviewSuggestion(form_data_key_, GetUsernameFromSuggestion(value));
  DCHECK(success);
}

void PasswordAutofillManager::DidAcceptSuggestion(const base::string16& value,
                                                  int identifier,
                                                  int position) {
  autofill_client_->ExecuteCommand(identifier);
  if (identifier == autofill::POPUP_ITEM_ID_GENERATE_PASSWORD_ENTRY) {
    password_manager_driver_->UserSelectedManualGenerationOption();
  } else if (identifier != autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY) {
    bool success =
        FillSuggestion(form_data_key_, GetUsernameFromSuggestion(value));
    DCHECK(success);
  }

  if (identifier == autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY) {
    DCHECK_NE(show_all_saved_passwords_shown_context_,
              metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_NONE);

    metrics_util::LogContextOfShowAllSavedPasswordsAccepted(
        show_all_saved_passwords_shown_context_);

    if (password_client_) {
      using UserAction =
          password_manager::PasswordManagerMetricsRecorder::PageLevelUserAction;
      switch (show_all_saved_passwords_shown_context_) {
        case metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_PASSWORD:
          password_client_->GetMetricsRecorder().RecordPageLevelUserAction(
              UserAction::kShowAllPasswordsWhileSomeAreSuggested);
          break;
        case metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_MANUAL_FALLBACK:
          password_client_->GetMetricsRecorder().RecordPageLevelUserAction(
              UserAction::kShowAllPasswordsWhileNoneAreSuggested);
          break;
        case metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_CONTEXT_MENU:
        case metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_NONE:
        case metrics_util::SHOW_ALL_SAVED_PASSWORDS_CONTEXT_COUNT:
          NOTREACHED();
      }
    }
  }

  autofill_client_->HideAutofillPopup();
}

bool PasswordAutofillManager::GetDeletionConfirmationText(
    const base::string16& value,
    int identifier,
    base::string16* title,
    base::string16* body) {
  return false;
}

bool PasswordAutofillManager::RemoveSuggestion(const base::string16& value,
                                               int identifier) {
  // Password suggestions cannot be deleted this way.
  // See http://crbug.com/329038#c15
  return false;
}

void PasswordAutofillManager::ClearPreviewedForm() {
  password_manager_driver_->ClearPreviewedForm();
}

autofill::PopupType PasswordAutofillManager::GetPopupType() const {
  return autofill::PopupType::kPasswords;
}

autofill::AutofillDriver* PasswordAutofillManager::GetAutofillDriver() {
  return password_manager_driver_->GetAutofillDriver();
}

void PasswordAutofillManager::RegisterDeletionCallback(
    base::OnceClosure deletion_callback) {
  deletion_callback_ = std::move(deletion_callback);
}

////////////////////////////////////////////////////////////////////////////////
// PasswordAutofillManager, private:

bool PasswordAutofillManager::GetPasswordAndRealmForUsername(
    const base::string16& current_username,
    const autofill::PasswordFormFillData& fill_data,
    autofill::PasswordAndRealm* password_and_realm) {
  // TODO(dubroy): When password access requires some kind of authentication
  // (e.g. Keychain access on Mac OS), use |password_manager_client_| here to
  // fetch the actual password. See crbug.com/178358 for more context.

  // Look for any suitable matches to current field text.
  if (fill_data.username_field.value == current_username) {
    password_and_realm->password = fill_data.password_field.value;
    password_and_realm->realm = fill_data.preferred_realm;
    return true;
  }

  // Scan additional logins for a match.
  for (autofill::PasswordFormFillData::LoginCollection::const_iterator iter =
           fill_data.additional_logins.begin();
       iter != fill_data.additional_logins.end(); ++iter) {
    if (iter->first == current_username) {
      *password_and_realm = iter->second;
      return true;
    }
  }

  return false;
}

bool PasswordAutofillManager::FindLoginInfo(
    int key,
    autofill::PasswordFormFillData* found_password) {
  LoginToPasswordInfoMap::iterator iter = login_to_password_info_.find(key);
  if (iter == login_to_password_info_.end())
    return false;

  *found_password = iter->second;
  return true;
}

}  //  namespace password_manager
