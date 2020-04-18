// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/webdata/autofill_profile_data_type_controller.h"

#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/experiments.h"
#include "components/sync/driver/sync_client.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/model/sync_error.h"
#include "components/sync/model/syncable_service.h"

namespace browser_sync {

AutofillProfileDataTypeController::AutofillProfileDataTypeController(
    scoped_refptr<base::SingleThreadTaskRunner> db_thread,
    const base::Closure& dump_stack,
    syncer::SyncClient* sync_client,
    const scoped_refptr<autofill::AutofillWebDataService>& web_data_service)
    : AsyncDirectoryTypeController(syncer::AUTOFILL_PROFILE,
                                   dump_stack,
                                   sync_client,
                                   syncer::GROUP_DB,
                                   std::move(db_thread)),
      sync_client_(sync_client),
      web_data_service_(web_data_service),
      callback_registered_(false),
      currently_enabled_(IsEnabled()) {
  pref_registrar_.Init(sync_client_->GetPrefService());
  pref_registrar_.Add(
      autofill::prefs::kAutofillEnabled,
      base::Bind(&AutofillProfileDataTypeController::OnUserPrefChanged,
                 base::AsWeakPtr(this)));
}

void AutofillProfileDataTypeController::WebDatabaseLoaded() {
  DCHECK(CalledOnValidThread());
  OnModelLoaded();
}

void AutofillProfileDataTypeController::OnPersonalDataChanged() {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(state(), MODEL_STARTING);

  sync_client_->GetPersonalDataManager()->RemoveObserver(this);

  if (!web_data_service_)
    return;

  if (web_data_service_->IsDatabaseLoaded()) {
    OnModelLoaded();
  } else if (!callback_registered_) {
    web_data_service_->RegisterDBLoadedCallback(
        base::Bind(&AutofillProfileDataTypeController::WebDatabaseLoaded,
                   base::AsWeakPtr(this)));
    callback_registered_ = true;
  }
}

AutofillProfileDataTypeController::~AutofillProfileDataTypeController() {}

bool AutofillProfileDataTypeController::StartModels() {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(state(), MODEL_STARTING);

  if (!IsEnabled()) {
    DisableForPolicy();
    return false;
  }

  // Waiting for the personal data is subtle:  we do this as the PDM resets
  // its cache of unique IDs once it gets loaded. If we were to proceed with
  // association, the local ids in the mappings would wind up colliding.
  autofill::PersonalDataManager* personal_data =
      sync_client_->GetPersonalDataManager();
  if (!personal_data->IsDataLoaded()) {
    personal_data->AddObserver(this);
    return false;
  }

  if (!web_data_service_)
    return false;

  if (web_data_service_->IsDatabaseLoaded())
    return true;

  if (!callback_registered_) {
    web_data_service_->RegisterDBLoadedCallback(
        base::Bind(&AutofillProfileDataTypeController::WebDatabaseLoaded,
                   base::AsWeakPtr(this)));
    callback_registered_ = true;
  }

  return false;
}

void AutofillProfileDataTypeController::StopModels() {
  DCHECK(CalledOnValidThread());
  sync_client_->GetPersonalDataManager()->RemoveObserver(this);
}

bool AutofillProfileDataTypeController::ReadyForStart() const {
  DCHECK(CalledOnValidThread());
  return currently_enabled_;
}

void AutofillProfileDataTypeController::OnUserPrefChanged() {
  DCHECK(CalledOnValidThread());

  bool new_enabled = IsEnabled();
  if (currently_enabled_ == new_enabled)
    return;  // No change to sync state.
  currently_enabled_ = new_enabled;

  if (currently_enabled_) {
    // The preference was just enabled. Trigger a reconfiguration. This will do
    // nothing if the type isn't preferred.
    syncer::SyncService* sync_service = sync_client_->GetSyncService();
    sync_service->ReenableDatatype(type());
  } else {
    DisableForPolicy();
  }
}

bool AutofillProfileDataTypeController::IsEnabled() {
  DCHECK(CalledOnValidThread());

  // Require the user-visible pref to be enabled to sync Autofill Profile data.
  PrefService* ps = sync_client_->GetPrefService();
  return ps->GetBoolean(autofill::prefs::kAutofillEnabled);
}

void AutofillProfileDataTypeController::DisableForPolicy() {
  if (state() != NOT_RUNNING && state() != STOPPING) {
    CreateErrorHandler()->OnUnrecoverableError(
        syncer::SyncError(FROM_HERE, syncer::SyncError::DATATYPE_POLICY_ERROR,
                          "Profile syncing is disabled by policy.", type()));
  }
}

}  // namespace browser_sync
