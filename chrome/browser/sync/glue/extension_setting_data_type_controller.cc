// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/extension_setting_data_type_controller.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sync/driver/generic_change_processor.h"
#include "components/sync/model/syncable_service.h"
#include "extensions/browser/api/storage/backend_task_runner.h"
#include "extensions/browser/extension_system.h"

namespace browser_sync {

ExtensionSettingDataTypeController::ExtensionSettingDataTypeController(
    syncer::ModelType type,
    const base::Closure& dump_stack,
    syncer::SyncClient* sync_client,
    Profile* profile)
    : AsyncDirectoryTypeController(type,
                                   dump_stack,
                                   sync_client,
                                   syncer::GROUP_FILE,
                                   extensions::GetBackendTaskRunner()),
      profile_(profile) {
  DCHECK(type == syncer::EXTENSION_SETTINGS || type == syncer::APP_SETTINGS);
}

ExtensionSettingDataTypeController::~ExtensionSettingDataTypeController() {}

bool ExtensionSettingDataTypeController::StartModels() {
  DCHECK(CalledOnValidThread());
  extensions::ExtensionSystem::Get(profile_)->InitForRegularProfile(
      true /* extensions_enabled */);
  return true;
}

}  // namespace browser_sync
