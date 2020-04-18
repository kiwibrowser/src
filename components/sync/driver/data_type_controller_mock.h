// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_DATA_TYPE_CONTROLLER_MOCK_H__
#define COMPONENTS_SYNC_DRIVER_DATA_TYPE_CONTROLLER_MOCK_H__

#include "components/sync/driver/data_type_controller.h"
#include "components/sync/model/sync_error.h"
#include "components/sync/model/sync_merge_result.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace syncer {

class StartCallbackMock {
 public:
  StartCallbackMock();
  virtual ~StartCallbackMock();

  MOCK_METHOD3(Run,
               void(DataTypeController::ConfigureResult result,
                    const SyncMergeResult& local_merge_result,
                    const SyncMergeResult& syncer_merge_result));
};

class ModelLoadCallbackMock {
 public:
  ModelLoadCallbackMock();
  virtual ~ModelLoadCallbackMock();

  MOCK_METHOD2(Run, void(ModelType, const SyncError&));
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_DATA_TYPE_CONTROLLER_MOCK_H__
