// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/proxy_data_type_controller.h"

#include "base/values.h"
#include "components/sync/engine/model_safe_worker.h"
#include "components/sync/engine/model_type_configurer.h"
#include "components/sync/model/sync_merge_result.h"

namespace syncer {

ProxyDataTypeController::ProxyDataTypeController(ModelType type)
    : DataTypeController(type), state_(NOT_RUNNING) {
  DCHECK(ProxyTypes().Has(type));
}

ProxyDataTypeController::~ProxyDataTypeController() {}

bool ProxyDataTypeController::ShouldLoadModelBeforeConfigure() const {
  return false;
}

void ProxyDataTypeController::BeforeLoadModels(
    ModelTypeConfigurer* configurer) {
  // Proxy type doesn't need to be registered with ModelTypeRegistry as it
  // doesn't need update handler, client doesn't expect updates of this type
  // from the server. We still need to register proxy type because
  // AddClientConfigParamsToMessage decides the value of tabs_datatype_enabled
  // based on presence of proxy types in the set of enabled types.
  configurer->RegisterDirectoryDataType(type(), GROUP_PASSIVE);
}

void ProxyDataTypeController::LoadModels(
    const ModelLoadCallback& model_load_callback) {
  DCHECK(CalledOnValidThread());
  state_ = MODEL_LOADED;
  model_load_callback.Run(type(), SyncError());
}

void ProxyDataTypeController::RegisterWithBackend(
    base::Callback<void(bool)> set_downloaded,
    ModelTypeConfigurer* configurer) {}

void ProxyDataTypeController::StartAssociating(
    const StartCallback& start_callback) {
  DCHECK(CalledOnValidThread());
  SyncMergeResult local_merge_result(type());
  SyncMergeResult syncer_merge_result(type());
  state_ = RUNNING;
  start_callback.Run(DataTypeController::OK, local_merge_result,
                     syncer_merge_result);
}

void ProxyDataTypeController::Stop() {
  state_ = NOT_RUNNING;
}

DataTypeController::State ProxyDataTypeController::state() const {
  return state_;
}

void ProxyDataTypeController::ActivateDataType(
    ModelTypeConfigurer* configurer) {}

void ProxyDataTypeController::DeactivateDataType(
    ModelTypeConfigurer* configurer) {
  configurer->UnregisterDirectoryDataType(type());
}

void ProxyDataTypeController::GetAllNodes(const AllNodesCallback& callback) {
  callback.Run(type(), std::make_unique<base::ListValue>());
}

void ProxyDataTypeController::GetStatusCounters(
    const StatusCountersCallback& callback) {
  syncer::StatusCounters counters;
  callback.Run(type(), counters);
}

void ProxyDataTypeController::RecordMemoryUsageHistogram() {}

}  // namespace syncer
