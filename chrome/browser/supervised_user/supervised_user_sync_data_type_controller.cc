// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_sync_data_type_controller.h"

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/profiles/profile.h"

SupervisedUserSyncDataTypeController::SupervisedUserSyncDataTypeController(
    syncer::ModelType type,
    const base::Closure& dump_stack,
    syncer::SyncClient* sync_client,
    Profile* profile)
    : syncer::AsyncDirectoryTypeController(type,
                                           dump_stack,
                                           sync_client,
                                           syncer::GROUP_UI,
                                           base::ThreadTaskRunnerHandle::Get()),
      profile_(profile) {
  DCHECK(type == syncer::SUPERVISED_USER_SETTINGS ||
         type == syncer::SUPERVISED_USER_WHITELISTS);
}

SupervisedUserSyncDataTypeController::~SupervisedUserSyncDataTypeController() {}

bool SupervisedUserSyncDataTypeController::ReadyForStart() const {
  DCHECK(CalledOnValidThread());
  return profile_->IsSupervised();
}
