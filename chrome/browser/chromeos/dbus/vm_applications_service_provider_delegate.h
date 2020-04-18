// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_VM_APPLICATIONS_SERVICE_PROVIDER_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_VM_APPLICATIONS_SERVICE_PROVIDER_DELEGATE_H_

#include "base/macros.h"
#include "chromeos/dbus/services/vm_applications_service_provider.h"

namespace chromeos {

class VmApplicationsServiceProviderDelegate
    : public VmApplicationsServiceProvider::Delegate {
 public:
  VmApplicationsServiceProviderDelegate();
  ~VmApplicationsServiceProviderDelegate() override;

  // VmApplicationsServiceProvider::Delegate:
  void UpdateApplicationList(
      const vm_tools::apps::ApplicationList& app_list) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VmApplicationsServiceProviderDelegate);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_VM_APPLICATIONS_SERVICE_PROVIDER_DELEGATE_H_
