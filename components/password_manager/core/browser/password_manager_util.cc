// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_manager_util.h"

#include <algorithm>

#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/common/password_form.h"
#include "components/autofill/core/common/password_generation_util.h"
#include "components/password_manager/core/browser/log_manager.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_store_consumer.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync/driver/sync_service.h"

using autofill::PasswordForm;

namespace password_manager_util {
namespace {

// Clears username/password on the blacklisted credentials.
class BlacklistedCredentialsCleaner
    : public password_manager::PasswordStoreConsumer {
 public:
  BlacklistedCredentialsCleaner(password_manager::PasswordStore* store,
                                PrefService* prefs)
      : store_(store), prefs_(prefs) {
    store_->GetBlacklistLogins(this);
  }
  ~BlacklistedCredentialsCleaner() override = default;

  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results) override {
    bool cleaned_something = false;
    for (const auto& form : results) {
      DCHECK(form->blacklisted_by_user);
      if (!form->username_value.empty() || !form->password_value.empty()) {
        cleaned_something = true;
        store_->RemoveLogin(*form);
        form->username_value.clear();
        form->password_value.clear();
        store_->AddLogin(*form);
      }
    }

    // Update the pref if no forms were handled. The password store is async,
    // therefore, one can't be sure that the changes applied cleanly.
    if (!cleaned_something) {
      prefs_->SetBoolean(
          password_manager::prefs::kBlacklistedCredentialsStripped, true);
    }
    delete this;
  }

 private:
  password_manager::PasswordStore* store_;
  PrefService* prefs_;

  DISALLOW_COPY_AND_ASSIGN(BlacklistedCredentialsCleaner);
};

void StartCleaningBlacklisted(
    const scoped_refptr<password_manager::PasswordStore>& store,
    PrefService* prefs) {
  // The object will delete itself once the credentials are retrieved.
  new BlacklistedCredentialsCleaner(store.get(), prefs);
}

// Return true if
// 1.|lhs| is non-PSL match, |rhs| is PSL match or
// 2.|lhs| and |rhs| have the same value of |is_public_suffix_match|, and |lhs|
// is preferred while |rhs| is not preferred.
bool IsBetterMatch(const PasswordForm* lhs, const PasswordForm* rhs) {
  return std::make_pair(!lhs->is_public_suffix_match, lhs->preferred) >
         std::make_pair(!rhs->is_public_suffix_match, rhs->preferred);
}

}  // namespace

password_manager::PasswordSyncState GetPasswordSyncState(
    const syncer::SyncService* sync_service) {
  if (sync_service && sync_service->IsFirstSetupComplete() &&
      sync_service->IsSyncActive() &&
      sync_service->GetActiveDataTypes().Has(syncer::PASSWORDS)) {
    return sync_service->IsUsingSecondaryPassphrase()
               ? password_manager::SYNCING_WITH_CUSTOM_PASSPHRASE
               : password_manager::SYNCING_NORMAL_ENCRYPTION;
  }
  return password_manager::NOT_SYNCING_PASSWORDS;
}

void FindDuplicates(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* forms,
    std::vector<std::unique_ptr<autofill::PasswordForm>>* duplicates,
    std::vector<std::vector<autofill::PasswordForm*>>* tag_groups) {
  if (forms->empty())
    return;

  // Linux backends used to treat the first form as a prime oneamong the
  // duplicates. Therefore, the caller should try to preserve it.
  std::stable_sort(forms->begin(), forms->end(), autofill::LessThanUniqueKey());

  std::vector<std::unique_ptr<autofill::PasswordForm>> unique_forms;
  unique_forms.push_back(std::move(forms->front()));
  if (tag_groups) {
    tag_groups->clear();
    tag_groups->push_back(std::vector<autofill::PasswordForm*>());
    tag_groups->front().push_back(unique_forms.front().get());
  }
  for (auto it = forms->begin() + 1; it != forms->end(); ++it) {
    if (ArePasswordFormUniqueKeyEqual(**it, *unique_forms.back())) {
      if (tag_groups)
        tag_groups->back().push_back(it->get());
      duplicates->push_back(std::move(*it));
    } else {
      if (tag_groups)
        tag_groups->push_back(
            std::vector<autofill::PasswordForm*>(1, it->get()));
      unique_forms.push_back(std::move(*it));
    }
  }
  forms->swap(unique_forms);
}

void TrimUsernameOnlyCredentials(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* android_credentials) {
  // Remove username-only credentials which are not federated.
  base::EraseIf(*android_credentials,
                [](const std::unique_ptr<autofill::PasswordForm>& form) {
                  return form->scheme ==
                             autofill::PasswordForm::SCHEME_USERNAME_ONLY &&
                         form->federation_origin.unique();
                });

  // Set "skip_zero_click" on federated credentials.
  std::for_each(
      android_credentials->begin(), android_credentials->end(),
      [](const std::unique_ptr<autofill::PasswordForm>& form) {
        if (form->scheme == autofill::PasswordForm::SCHEME_USERNAME_ONLY)
          form->skip_zero_click = true;
      });
}

bool IsLoggingActive(const password_manager::PasswordManagerClient* client) {
  const password_manager::LogManager* log_manager = client->GetLogManager();
  return log_manager && log_manager->IsLoggingActive();
}

bool ManualPasswordGenerationEnabled(syncer::SyncService* sync_service) {
  if (!(base::FeatureList::IsEnabled(
            password_manager::features::kEnableManualPasswordGeneration) &&
        (password_manager_util::GetPasswordSyncState(sync_service) ==
         password_manager::SYNCING_NORMAL_ENCRYPTION))) {
    return false;
  }
  LogPasswordGenerationEvent(
      autofill::password_generation::PASSWORD_GENERATION_CONTEXT_MENU_SHOWN);
  return true;
}

bool ShowAllSavedPasswordsContextMenuEnabled() {
  if (!base::FeatureList::IsEnabled(
          password_manager::features::
              kEnableShowAllSavedPasswordsContextMenu)) {
    return false;
  }
  LogContextOfShowAllSavedPasswordsShown(
      password_manager::metrics_util::
          SHOW_ALL_SAVED_PASSWORDS_CONTEXT_CONTEXT_MENU);
  return true;
}

void UserTriggeredShowAllSavedPasswordsFromContextMenu(
    autofill::AutofillClient* autofill_client) {
  if (!autofill_client)
    return;
  autofill_client->ExecuteCommand(
      autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY);
  password_manager::metrics_util::LogContextOfShowAllSavedPasswordsAccepted(
      password_manager::metrics_util::
          SHOW_ALL_SAVED_PASSWORDS_CONTEXT_CONTEXT_MENU);
}

void UserTriggeredManualGenerationFromContextMenu(
    password_manager::PasswordManagerClient* password_manager_client) {
  password_manager_client->GeneratePassword();
  LogPasswordGenerationEvent(
      autofill::password_generation::PASSWORD_GENERATION_CONTEXT_MENU_PRESSED);
}

void CleanUserDataInBlacklistedCredentials(
    password_manager::PasswordStore* store,
    PrefService* prefs,
    int delay_in_seconds) {
  bool need_to_clean = !prefs->GetBoolean(
      password_manager::prefs::kBlacklistedCredentialsStripped);
  UMA_HISTOGRAM_BOOLEAN("PasswordManager.BlacklistedSites.NeedToBeCleaned",
                        need_to_clean);
  if (need_to_clean) {
    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&StartCleaningBlacklisted, base::WrapRefCounted(store),
                       prefs),
        base::TimeDelta::FromSeconds(delay_in_seconds));
  }
}

void FindBestMatches(
    std::vector<const PasswordForm*> matches,
    std::map<base::string16, const PasswordForm*>* best_matches,
    std::vector<const PasswordForm*>* not_best_matches,
    const PasswordForm** preferred_match) {
  DCHECK(std::all_of(
      matches.begin(), matches.end(),
      [](const PasswordForm* match) { return !match->blacklisted_by_user; }));
  DCHECK(best_matches);
  DCHECK(not_best_matches);
  DCHECK(preferred_match);

  *preferred_match = nullptr;
  best_matches->clear();
  not_best_matches->clear();

  if (matches.empty())
    return;

  // Sort matches using IsBetterMatch predicate.
  std::sort(matches.begin(), matches.end(), IsBetterMatch);
  for (const auto* match : matches) {
    const base::string16& username = match->username_value;
    // The first match for |username| in the sorted array is best match.
    if (best_matches->find(username) == best_matches->end())
      best_matches->insert(std::make_pair(username, match));
    else
      not_best_matches->push_back(match);
  }

  *preferred_match = *matches.begin();
}

}  // namespace password_manager_util
