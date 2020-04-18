// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_SYNC_DATA_TYPE_CONTROLLER_H_
#define CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_SYNC_DATA_TYPE_CONTROLLER_H_

#include "base/macros.h"
#include "components/sync/driver/async_directory_type_controller.h"
#include "components/sync/driver/data_type_controller.h"

class Profile;

namespace syncer {
class SyncClient;
}

// A DataTypeController for supervised user sync datatypes, which enables or
// disables these types based on the profile's IsSupervised state.
class SupervisedUserSyncDataTypeController
    : public syncer::AsyncDirectoryTypeController {
 public:
  // |dump_stack| is called when an unrecoverable error occurs.
  SupervisedUserSyncDataTypeController(syncer::ModelType type,
                                       const base::Closure& dump_stack,
                                       syncer::SyncClient* sync_client,
                                       Profile* profile);
  ~SupervisedUserSyncDataTypeController() override;

  // AsyncDirectoryTypeController implementation.
  bool ReadyForStart() const override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SupervisedUserSyncDataTypeController);
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_SYNC_DATA_TYPE_CONTROLLER_H_
