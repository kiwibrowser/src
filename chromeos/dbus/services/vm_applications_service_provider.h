// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SERVICES_VM_APPLICATIONS_SERVICE_PROVIDER_H_
#define CHROMEOS_DBUS_SERVICES_VM_APPLICATIONS_SERVICE_PROVIDER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/services/cros_dbus_service.h"
#include "dbus/exported_object.h"

namespace dbus {
class MethodCall;
}  // namespace dbus

namespace vm_tools {
namespace apps {
class ApplicationList;
}  // namespace apps
}  // namespace vm_tools

namespace chromeos {

// This class exports D-Bus methods for functions that we want to be available
// to the Crostini container.
class CHROMEOS_EXPORT VmApplicationsServiceProvider
    : public CrosDBusService::ServiceProviderInterface {
 public:
  // Delegate interface providing additional resources to
  // VmApplicationsServiceProvider.
  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;

    virtual void UpdateApplicationList(
        const vm_tools::apps::ApplicationList& app_list) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  explicit VmApplicationsServiceProvider(std::unique_ptr<Delegate> delegate);
  ~VmApplicationsServiceProvider() override;

  // CrosDBusService::ServiceProviderInterface overrides:
  void Start(scoped_refptr<dbus::ExportedObject> exported_object) override;

 private:
  // Called from ExportedObject when UpdateApplicationList() is exported as a
  // D-Bus method or failed to be exported.
  void OnExported(const std::string& interface_name,
                  const std::string& method_name,
                  bool success);

  // Called on UI thread in response to a D-Bus request.
  void UpdateApplicationList(
      dbus::MethodCall* method_call,
      dbus::ExportedObject::ResponseSender response_sender);

  std::unique_ptr<Delegate> delegate_;

  base::WeakPtrFactory<VmApplicationsServiceProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VmApplicationsServiceProvider);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SERVICES_VM_APPLICATIONS_SERVICE_PROVIDER_H_
