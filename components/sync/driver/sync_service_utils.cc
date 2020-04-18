// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_service_utils.h"

#include "components/sync/base/sync_prefs.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/engine/cycle/sync_cycle_snapshot.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace syncer {

UploadState GetUploadToGoogleState(const SyncService* sync_service,
                                   ModelType type) {
  // Note: Before configuration is done, GetPreferredDataTypes returns
  // "everything" (i.e. the default setting). If a data type is missing there,
  // it must be because the user explicitly disabled it.
  if (!sync_service || !sync_service->CanSyncStart() ||
      sync_service->IsLocalSyncEnabled() ||
      !sync_service->GetPreferredDataTypes().Has(type) ||
      sync_service->GetAuthError().IsPersistentError()) {
    return UploadState::NOT_ACTIVE;
  }
  // If the given ModelType is encrypted with a custom passphrase, we also
  // consider uploading inactive, since Google can't read the data.
  // Note that encryption is tricky: Some data types (e.g. PASSWORDS) are always
  // encrypted, but not necessarily with a custom passphrase. On the other hand,
  // some data types are never encrypted (e.g. DEVICE_INFO), even if the
  // "encrypt everything" setting is enabled.
  if (sync_service->GetEncryptedDataTypes().Has(type) &&
      sync_service->IsUsingSecondaryPassphrase()) {
    return UploadState::NOT_ACTIVE;
  }
  // TODO(crbug.com/831579): We currently need to wait for GetLastCycleSnapshot
  // to return an initialized snapshot because we don't actually know if the
  // token is valid until sync has tried it. This is bad because sync can take
  // arbitrarily long to try the token (especially if the user doesn't have
  // history sync enabled). Instead, if the identity code would persist
  // persistent auth errors, we could read those from startup.
  if (!sync_service->IsSyncActive() || !sync_service->ConfigurationDone() ||
      !sync_service->GetLastCycleSnapshot().is_initialized()) {
    return UploadState::INITIALIZING;
  }
  return UploadState::ACTIVE;
}

}  // namespace syncer
