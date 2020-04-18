// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/device_sync/device_sync_service.h"

#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/services/device_sync/device_sync_impl.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace chromeos {

namespace device_sync {

DeviceSyncService::DeviceSyncService(
    identity::IdentityManager* identity_manager,
    gcm::GCMDriver* gcm_driver,
    cryptauth::GcmDeviceInfoProvider* gcm_device_info_provider,
    scoped_refptr<net::URLRequestContextGetter> url_request_context)
    : identity_manager_(identity_manager),
      gcm_driver_(gcm_driver),
      gcm_device_info_provider_(gcm_device_info_provider),
      url_request_context_(url_request_context) {}

DeviceSyncService::~DeviceSyncService() = default;

void DeviceSyncService::OnStart() {
  PA_LOG(INFO) << "DeviceSyncService::OnStart()";

  // context() cannot be invoked until after the constructor is run, so
  // |device_sync_impl_| cannot be initialized until OnStart().
  device_sync_impl_ = DeviceSyncImpl::Factory::NewInstance(
      identity_manager_, gcm_driver_, context()->connector(),
      gcm_device_info_provider_, url_request_context_);

  registry_.AddInterface(base::Bind(&DeviceSyncImpl::BindRequest,
                                    base::Unretained(device_sync_impl_.get())));
}

void DeviceSyncService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  PA_LOG(INFO) << "DeviceSyncService::OnBindInterface() from interface "
               << interface_name << ".";
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace device_sync

}  // namespace chromeos
