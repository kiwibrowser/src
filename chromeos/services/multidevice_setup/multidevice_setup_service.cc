// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/multidevice_setup/multidevice_setup_service.h"

#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/services/multidevice_setup/multidevice_setup_impl.h"

namespace chromeos {

namespace multidevice_setup {

MultiDeviceSetupService::MultiDeviceSetupService()
    : multidevice_setup_impl_(std::make_unique<MultiDeviceSetupImpl>()) {}

MultiDeviceSetupService::~MultiDeviceSetupService() = default;

void MultiDeviceSetupService::OnStart() {
  PA_LOG(INFO) << "MultiDeviceSetupService::OnStart()";
  registry_.AddInterface(base::Bind(&MultiDeviceSetupService::BindRequest,
                                    base::Unretained(this)));
}

void MultiDeviceSetupService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  PA_LOG(INFO) << "MultiDeviceSetupService::OnBindInterface() from interface "
               << interface_name << ".";
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void MultiDeviceSetupService::BindRequest(
    mojom::MultiDeviceSetupRequest request) {
  multidevice_setup_impl_->BindRequest(std::move(request));
}

}  // namespace multidevice_setup

}  // namespace chromeos
