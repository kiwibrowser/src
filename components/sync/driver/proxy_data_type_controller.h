// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_PROXY_DATA_TYPE_CONTROLLER_H__
#define COMPONENTS_SYNC_DRIVER_PROXY_DATA_TYPE_CONTROLLER_H__

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/sync/driver/data_type_controller.h"

namespace syncer {

// Implementation for proxy datatypes. These are datatype that have no
// representation in sync, and therefore no change processor or syncable
// service.
class ProxyDataTypeController : public DataTypeController {
 public:
  explicit ProxyDataTypeController(ModelType type);
  ~ProxyDataTypeController() override;

  // DataTypeController interface.
  bool ShouldLoadModelBeforeConfigure() const override;
  void BeforeLoadModels(ModelTypeConfigurer* configurer) override;
  void LoadModels(const ModelLoadCallback& model_load_callback) override;
  void RegisterWithBackend(base::Callback<void(bool)> set_downloaded,
                           ModelTypeConfigurer* configurer) override;
  void StartAssociating(const StartCallback& start_callback) override;
  void Stop() override;
  State state() const override;
  void ActivateDataType(ModelTypeConfigurer* configurer) override;
  void DeactivateDataType(ModelTypeConfigurer* configurer) override;
  void GetAllNodes(const AllNodesCallback& callback) override;
  void GetStatusCounters(const StatusCountersCallback& callback) override;
  void RecordMemoryUsageHistogram() override;

 private:
  State state_;

  DISALLOW_COPY_AND_ASSIGN(ProxyDataTypeController);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_PROXY_DATA_TYPE_CONTROLLER_H__
