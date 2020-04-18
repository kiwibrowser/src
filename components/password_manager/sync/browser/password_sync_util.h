// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_SYNC_BROWSER_PASSWORD_SYNC_UTIL_H_
#define COMPONENTS_PASSWORD_MANAGER_SYNC_BROWSER_PASSWORD_SYNC_UTIL_H_

#include <string>

#include "components/autofill/core/common/password_form.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/sync/driver/sync_service.h"

namespace password_manager {
namespace sync_util {

// Returns the sync username received from |signin_manager| (if not null).
// Moreover, using |sync_service| (if not null), this function also tries to
// return an empty string if the user isn't syncing passwords, but it is not
// always possible to determine since this code can be called during sync setup
// (http://crbug.com/393626).
std::string GetSyncUsernameIfSyncingPasswords(
    const syncer::SyncService* sync_service,
    const SigninManagerBase* signin_manager);

// Returns true if |form| corresponds to the account specified by
// GetSyncUsernameIfSyncingPasswords. Returns false if
// GetSyncUsernameIfSyncingPasswords does not specify any account.
bool IsSyncAccountCredential(const autofill::PasswordForm& form,
                             const syncer::SyncService* sync_service,
                             const SigninManagerBase* signin_manager);

// If |form| doesn't match GAIA sign-on realm or enterprise-specified password
// protection URL, returns false. Otherwise, checks if the username
// in |form| matches sign-in account (no syncing passwords are required).
bool ShouldSavePasswordHash(const autofill::PasswordForm& form,
                            const SigninManagerBase* signin_manager,
                            PrefService* prefs);

}  // namespace sync_util
}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_SYNC_BROWSER_PASSWORD_SYNC_UTIL_H_
