// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/theme_data_type_controller.h"

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_system.h"

namespace browser_sync {

ThemeDataTypeController::ThemeDataTypeController(
    const base::Closure& dump_stack,
    syncer::SyncClient* sync_client,
    Profile* profile)
    : AsyncDirectoryTypeController(syncer::THEMES,
                                   dump_stack,
                                   sync_client,
                                   syncer::GROUP_UI,
                                   base::ThreadTaskRunnerHandle::Get()),
      profile_(profile) {}

ThemeDataTypeController::~ThemeDataTypeController() {}

bool ThemeDataTypeController::StartModels() {
  DCHECK(CalledOnValidThread());
  extensions::ExtensionSystem::Get(profile_)->InitForRegularProfile(
      true /* extensions_enabled */);
  return true;
}

}  // namespace browser_sync
