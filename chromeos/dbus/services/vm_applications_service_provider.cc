// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/services/vm_applications_service_provider.h"

#include "base/bind.h"
#include "chromeos/dbus/vm_applications/apps.pb.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

VmApplicationsServiceProvider::VmApplicationsServiceProvider(
    std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)), weak_ptr_factory_(this) {}

VmApplicationsServiceProvider::~VmApplicationsServiceProvider() = default;

void VmApplicationsServiceProvider::Start(
    scoped_refptr<dbus::ExportedObject> exported_object) {
  exported_object->ExportMethod(
      vm_tools::apps::kVmApplicationsServiceInterface,
      vm_tools::apps::kVmApplicationsServiceUpdateApplicationListMethod,
      base::BindRepeating(&VmApplicationsServiceProvider::UpdateApplicationList,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&VmApplicationsServiceProvider::OnExported,
                          weak_ptr_factory_.GetWeakPtr()));
}

void VmApplicationsServiceProvider::OnExported(
    const std::string& interface_name,
    const std::string& method_name,
    bool success) {
  LOG_IF(ERROR, !success) << "Failed to export " << interface_name << "."
                          << method_name;
}

void VmApplicationsServiceProvider::UpdateApplicationList(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  dbus::MessageReader reader(method_call);

  vm_tools::apps::ApplicationList request;

  if (!reader.PopArrayOfBytesAsProto(&request)) {
    constexpr char error_message[] =
        "Unable to parse ApplicationList from message";
    LOG(ERROR) << error_message;
    response_sender.Run(dbus::ErrorResponse::FromMethodCall(
        method_call, DBUS_ERROR_INVALID_ARGS, error_message));
    return;
  }

  delegate_->UpdateApplicationList(request);
  response_sender.Run(dbus::Response::FromMethodCall(method_call));
}

}  // namespace chromeos
