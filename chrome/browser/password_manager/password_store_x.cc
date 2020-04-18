// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_x.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "chrome/browser/chrome_notification_types.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "components/password_manager/core/browser/password_store_change.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"

using autofill::PasswordForm;
using password_manager::PasswordStoreChange;
using password_manager::PasswordStoreChangeList;
using password_manager::PasswordStoreDefault;

namespace {

bool AddLoginToBackend(
    const std::unique_ptr<PasswordStoreX::NativeBackend>& backend,
    const PasswordForm& form,
    PasswordStoreChangeList* changes) {
  *changes = backend->AddLogin(form);
  return (!changes->empty() &&
          changes->back().type() == PasswordStoreChange::ADD);
}

bool RemoveLoginsByURLAndTimeFromBackend(
    PasswordStoreX::NativeBackend* backend,
    const base::Callback<bool(const GURL&)>& url_filter,
    base::Time delete_begin,
    base::Time delete_end,
    PasswordStoreChangeList* changes) {
  std::vector<std::unique_ptr<PasswordForm>> forms;
  if (!backend->GetAllLogins(&forms))
    return false;

  for (const auto& form : forms) {
    if (url_filter.Run(form->origin) && form->date_created >= delete_begin &&
        (delete_end.is_null() || form->date_created < delete_end) &&
        !backend->RemoveLogin(*form, changes))
      return false;
  }

  return true;
}

}  // namespace

PasswordStoreX::PasswordStoreX(
    std::unique_ptr<password_manager::LoginDatabase> login_db,
    std::unique_ptr<NativeBackend> backend)
    : PasswordStoreDefault(std::move(login_db)),
      backend_(std::move(backend)),
      migration_checked_(false),
      allow_fallback_(false) {}

PasswordStoreX::~PasswordStoreX() {}

scoped_refptr<base::SequencedTaskRunner>
PasswordStoreX::CreateBackgroundTaskRunner() const {
  scoped_refptr<base::SequencedTaskRunner> result =
      backend_ ? backend_->GetBackgroundTaskRunner() : nullptr;
  return result ? result : PasswordStoreDefault::CreateBackgroundTaskRunner();
}

PasswordStoreChangeList PasswordStoreX::AddLoginImpl(const PasswordForm& form) {
  CheckMigration();
  PasswordStoreChangeList changes;
  if (use_native_backend() && AddLoginToBackend(backend_, form, &changes)) {
    allow_fallback_ = false;
  } else if (allow_default_store()) {
    changes = PasswordStoreDefault::AddLoginImpl(form);
  }
  return changes;
}

PasswordStoreChangeList PasswordStoreX::UpdateLoginImpl(
    const PasswordForm& form) {
  CheckMigration();
  PasswordStoreChangeList changes;
  if (use_native_backend() && backend_->UpdateLogin(form, &changes)) {
    allow_fallback_ = false;
  } else if (allow_default_store()) {
    changes = PasswordStoreDefault::UpdateLoginImpl(form);
  }
  return changes;
}

PasswordStoreChangeList PasswordStoreX::RemoveLoginImpl(
    const PasswordForm& form) {
  CheckMigration();
  PasswordStoreChangeList changes;
  if (use_native_backend() && backend_->RemoveLogin(form, &changes)) {
    allow_fallback_ = false;
  } else if (allow_default_store()) {
    changes = PasswordStoreDefault::RemoveLoginImpl(form);
  }
  return changes;
}

PasswordStoreChangeList PasswordStoreX::RemoveLoginsByURLAndTimeImpl(
    const base::Callback<bool(const GURL&)>& url_filter,
    base::Time delete_begin,
    base::Time delete_end) {
  CheckMigration();
  PasswordStoreChangeList changes;

  if (use_native_backend() &&
      RemoveLoginsByURLAndTimeFromBackend(backend_.get(), url_filter,
                                          delete_begin, delete_end, &changes)) {
    LogStatsForBulkDeletion(changes.size());
    allow_fallback_ = false;
  } else if (allow_default_store()) {
    changes = PasswordStoreDefault::RemoveLoginsByURLAndTimeImpl(
        url_filter, delete_begin, delete_end);
  }

  return changes;
}

PasswordStoreChangeList PasswordStoreX::RemoveLoginsCreatedBetweenImpl(
    base::Time delete_begin,
    base::Time delete_end) {
  CheckMigration();
  PasswordStoreChangeList changes;
  if (use_native_backend() &&
      backend_->RemoveLoginsCreatedBetween(
          delete_begin, delete_end, &changes)) {
    LogStatsForBulkDeletion(changes.size());
    allow_fallback_ = false;
  } else if (allow_default_store()) {
    changes = PasswordStoreDefault::RemoveLoginsCreatedBetweenImpl(delete_begin,
                                                                   delete_end);
  }
  return changes;
}

PasswordStoreChangeList PasswordStoreX::RemoveLoginsSyncedBetweenImpl(
    base::Time delete_begin,
    base::Time delete_end) {
  CheckMigration();
  PasswordStoreChangeList changes;
  if (use_native_backend() &&
      backend_->RemoveLoginsSyncedBetween(delete_begin, delete_end, &changes)) {
    LogStatsForBulkDeletionDuringRollback(changes.size());
    allow_fallback_ = false;
  } else if (allow_default_store()) {
    changes = PasswordStoreDefault::RemoveLoginsSyncedBetweenImpl(delete_begin,
                                                                  delete_end);
  }
  return changes;
}

PasswordStoreChangeList PasswordStoreX::DisableAutoSignInForOriginsImpl(
    const base::Callback<bool(const GURL&)>& origin_filter) {
  CheckMigration();
  PasswordStoreChangeList changes;
  if (use_native_backend() &&
      backend_->DisableAutoSignInForOrigins(origin_filter, &changes)) {
    allow_fallback_ = false;
  } else if (allow_default_store()) {
    changes =
        PasswordStoreDefault::DisableAutoSignInForOriginsImpl(origin_filter);
  }
  return changes;
}

namespace {

// Sorts |list| by origin, like the ORDER BY clause in login_database.cc.
void SortLoginsByOrigin(std::vector<std::unique_ptr<PasswordForm>>* list) {
  std::sort(list->begin(), list->end(),
            [](const std::unique_ptr<PasswordForm>& a,
               const std::unique_ptr<PasswordForm>& b) {
              return a->origin < b->origin;
            });
}

}  // anonymous namespace

std::vector<std::unique_ptr<PasswordForm>> PasswordStoreX::FillMatchingLogins(
    const FormDigest& form) {
  CheckMigration();
  std::vector<std::unique_ptr<PasswordForm>> matched_forms;
  if (use_native_backend() && backend_->GetLogins(form, &matched_forms)) {
    SortLoginsByOrigin(&matched_forms);
    // The native backend may succeed and return no data even while locked, if
    // the query did not match anything stored. So we continue to allow fallback
    // until we perform a write operation, or until a read returns actual data.
    if (!matched_forms.empty())
      allow_fallback_ = false;
    return matched_forms;
  }
  if (allow_default_store())
    return PasswordStoreDefault::FillMatchingLogins(form);
  return std::vector<std::unique_ptr<PasswordForm>>();
}

std::vector<std::unique_ptr<PasswordForm>>
PasswordStoreX::FillLoginsForSameOrganizationName(
    const std::string& signon_realm) {
  // Not available on X.
  return std::vector<std::unique_ptr<PasswordForm>>();
}

bool PasswordStoreX::FillAutofillableLogins(
    std::vector<std::unique_ptr<PasswordForm>>* forms) {
  CheckMigration();
  if (use_native_backend() && backend_->GetAutofillableLogins(forms)) {
    SortLoginsByOrigin(forms);
    // See GetLoginsImpl() for why we disallow fallback conditionally here.
    if (!forms->empty())
      allow_fallback_ = false;
    return true;
  }
  if (allow_default_store())
    return PasswordStoreDefault::FillAutofillableLogins(forms);
  return false;
}

bool PasswordStoreX::FillBlacklistLogins(
    std::vector<std::unique_ptr<PasswordForm>>* forms) {
  CheckMigration();
  if (use_native_backend() && backend_->GetBlacklistLogins(forms)) {
    // See GetLoginsImpl() for why we disallow fallback conditionally here.
    SortLoginsByOrigin(forms);
    if (!forms->empty())
      allow_fallback_ = false;
    return true;
  }
  if (allow_default_store())
    return PasswordStoreDefault::FillBlacklistLogins(forms);
  return false;
}

void PasswordStoreX::CheckMigration() {
  DCHECK(background_task_runner()->RunsTasksInCurrentSequence());
  if (migration_checked_ || !backend_.get())
    return;
  migration_checked_ = true;
  ssize_t migrated = MigrateLogins();
  if (migrated > 0) {
    VLOG(1) << "Migrated " << migrated << " passwords to native store.";
  } else if (migrated == 0) {
    // As long as we are able to migrate some passwords, we know the native
    // store is working. But if there is nothing to migrate, the "migration"
    // can succeed even when the native store would fail. In this case we
    // allow a later fallback to the default store. Once any later operation
    // succeeds on the native store, we will no longer allow fallback.
    allow_fallback_ = true;
  } else {
    LOG(WARNING) << "Native password store migration failed! " <<
                 "Falling back on default (unencrypted) store.";
    backend_.reset();
  }
}

bool PasswordStoreX::allow_default_store() {
  if (allow_fallback_) {
    LOG(WARNING) << "Native password store failed! " <<
                 "Falling back on default (unencrypted) store.";
    backend_.reset();
    // Don't warn again. We'll use the default store because backend_ is NULL.
    allow_fallback_ = false;
  }
  return !backend_.get();
}

ssize_t PasswordStoreX::MigrateLogins() {
  DCHECK(backend_.get());
  std::vector<std::unique_ptr<PasswordForm>> forms;
  std::vector<std::unique_ptr<PasswordForm>> blacklist_forms;
  bool ok = PasswordStoreDefault::FillAutofillableLogins(&forms) &&
            PasswordStoreDefault::FillBlacklistLogins(&blacklist_forms);
  const size_t autofillable_forms_count = forms.size();
  forms.resize(autofillable_forms_count + blacklist_forms.size());
  std::move(blacklist_forms.begin(), blacklist_forms.end(),
            forms.begin() + autofillable_forms_count);
  if (ok) {
    // We add all the passwords (and blacklist entries) to the native backend
    // before attempting to remove any from the login database, to make sure we
    // don't somehow end up with some of the passwords in one store and some in
    // another. We'll always have at least one intact store this way.
    for (size_t i = 0; i < forms.size(); ++i) {
      PasswordStoreChangeList changes;
      if (!AddLoginToBackend(backend_, *forms[i], &changes)) {
        ok = false;
        break;
      }
    }
    if (ok) {
      for (size_t i = 0; i < forms.size(); ++i) {
        // If even one of these calls to RemoveLoginImpl() succeeds, then we
        // should prefer the native backend to the now-incomplete login
        // database. Thus we want to return a success status even in the case
        // where some fail. The only real problem with this is that we might
        // leave passwords in the login database and never come back to clean
        // them out if any of these calls do fail.
        PasswordStoreDefault::RemoveLoginImpl(*forms[i]);
      }
      // Finally, delete the database file itself. We remove the passwords from
      // it before deleting the file just in case there is some problem deleting
      // the file (e.g. directory is not writable, but file is), which would
      // otherwise cause passwords to re-migrate next (or maybe every) time.
      DeleteAndRecreateDatabaseFile();
    }
  }
  ssize_t result = ok ? forms.size() : -1;
  return result;
}
