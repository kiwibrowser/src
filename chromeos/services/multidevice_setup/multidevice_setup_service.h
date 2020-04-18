// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_MULTIDEVICE_SETUP_MULTIDEVICE_SETUP_SERVICE_H_
#define CHROMEOS_SERVICES_MULTIDEVICE_SETUP_MULTIDEVICE_SETUP_SERVICE_H_

#include <memory>

#include "chromeos/services/multidevice_setup/public/mojom/multidevice_setup.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace chromeos {

namespace multidevice_setup {

class MultiDeviceSetupImpl;

// Service which provides an implementation for mojom::MultiDeviceSetup. This
// service creates one implementation and shares it among all connection
// requests.
class MultiDeviceSetupService : public service_manager::Service {
 public:
  MultiDeviceSetupService();
  ~MultiDeviceSetupService() override;

 private:
  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  void BindRequest(mojom::MultiDeviceSetupRequest request);

  std::unique_ptr<MultiDeviceSetupImpl> multidevice_setup_impl_;

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(MultiDeviceSetupService);
};

}  // namespace multidevice_setup

}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_MULTIDEVICE_SETUP_MULTIDEVICE_SETUP_SERVICE_H_
