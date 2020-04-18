// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_FACTORY_UTIL_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_FACTORY_UTIL_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/login_database.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/model/syncable_service.h"
#include "net/url_request/url_request_context_getter.h"

namespace password_manager {

// Activates or deactivates affiliation-based matching for |password_store|,
// depending on whether or not the |sync_service| is syncing passwords stored
// therein. The AffiliationService will use |request_context_getter| to fetch
// affiliation information. This function should be called whenever there is a
// possibility that syncing passwords has just started or ended.
void ToggleAffiliationBasedMatchingBasedOnPasswordSyncedState(
    PasswordStore* password_store,
    syncer::SyncService* sync_service,
    net::URLRequestContextGetter* request_context_getter,
    const base::FilePath& profile_path);

// Creates a LoginDatabase. Looks in |profile_path| for the database file.
// Does not call LoginDatabase::Init() -- to avoid UI jank, that needs to be
// called by PasswordStore::Init() on the background thread.
std::unique_ptr<LoginDatabase> CreateLoginDatabase(
    const base::FilePath& profile_path);

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_STORE_FACTORY_UTIL_H_
