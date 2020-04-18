// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_api_component_factory_mock.h"

#include <utility>

#include "components/sync/device_info/local_device_info_provider_mock.h"
#include "components/sync/driver/model_associator.h"
#include "components/sync/model/change_processor.h"

using testing::_;
using testing::InvokeWithoutArgs;
using testing::Return;

namespace syncer {

SyncApiComponentFactoryMock::SyncApiComponentFactoryMock() = default;

SyncApiComponentFactoryMock::SyncApiComponentFactoryMock(
    AssociatorInterface* model_associator,
    ChangeProcessor* change_processor)
    : model_associator_(model_associator),
      change_processor_(change_processor) {}

SyncApiComponentFactoryMock::~SyncApiComponentFactoryMock() {}

SyncApiComponentFactory::SyncComponents
SyncApiComponentFactoryMock::CreateBookmarkSyncComponents(
    SyncService* sync_service,
    std::unique_ptr<DataTypeErrorHandler> error_handler) {
  return MakeSyncComponents();
}

SyncApiComponentFactory::SyncComponents
SyncApiComponentFactoryMock::MakeSyncComponents() {
  return SyncApiComponentFactory::SyncComponents(model_associator_.release(),
                                                 change_processor_.release());
}

std::unique_ptr<LocalDeviceInfoProvider>
SyncApiComponentFactoryMock::CreateLocalDeviceInfoProvider() {
  if (local_device_)
    return std::move(local_device_);
  return std::make_unique<LocalDeviceInfoProviderMock>();
}

void SyncApiComponentFactoryMock::SetLocalDeviceInfoProvider(
    std::unique_ptr<LocalDeviceInfoProvider> local_device) {
  local_device_ = std::move(local_device);
}

}  // namespace syncer
