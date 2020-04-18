// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/services/virtual_file_request_service_provider.h"

#include <utility>

#include "base/bind.h"
#include "dbus/message.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

VirtualFileRequestServiceProvider::VirtualFileRequestServiceProvider(
    std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)), weak_ptr_factory_(this) {}

VirtualFileRequestServiceProvider::~VirtualFileRequestServiceProvider() =
    default;

void VirtualFileRequestServiceProvider::Start(
    scoped_refptr<dbus::ExportedObject> exported_object) {
  exported_object->ExportMethod(
      kVirtualFileRequestServiceInterface,
      kVirtualFileRequestServiceHandleReadRequestMethod,
      base::Bind(&VirtualFileRequestServiceProvider::HandleReadRequest,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind([](const std::string& interface_name,
                    const std::string& method_name, bool success) {
        LOG_IF(ERROR, !success)
            << "Failed to export " << interface_name << "." << method_name;
      }));
  exported_object->ExportMethod(
      kVirtualFileRequestServiceInterface,
      kVirtualFileRequestServiceHandleIdReleasedMethod,
      base::Bind(&VirtualFileRequestServiceProvider::HandleIdReleased,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind([](const std::string& interface_name,
                    const std::string& method_name, bool success) {
        LOG_IF(ERROR, !success)
            << "Failed to export " << interface_name << "." << method_name;
      }));
}

void VirtualFileRequestServiceProvider::HandleReadRequest(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  std::string id;
  int64_t offset = 0;
  int64_t size = 0;
  base::ScopedFD pipe_write_end;
  dbus::MessageReader reader(method_call);
  if (!reader.PopString(&id) || !reader.PopInt64(&offset) ||
      !reader.PopInt64(&size) || !reader.PopFileDescriptor(&pipe_write_end)) {
    response_sender.Run(dbus::ErrorResponse::FromMethodCall(
        method_call, DBUS_ERROR_INVALID_ARGS, std::string()));
    return;
  }
  if (!delegate_->HandleReadRequest(id, offset, size,
                                    std::move(pipe_write_end))) {
    response_sender.Run(dbus::ErrorResponse::FromMethodCall(
        method_call, DBUS_ERROR_FAILED, std::string()));
    return;
  }
  response_sender.Run(dbus::Response::FromMethodCall(method_call));
}

void VirtualFileRequestServiceProvider::HandleIdReleased(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  std::string id;
  dbus::MessageReader reader(method_call);
  if (!reader.PopString(&id)) {
    response_sender.Run(dbus::ErrorResponse::FromMethodCall(
        method_call, DBUS_ERROR_INVALID_ARGS, std::string()));
    return;
  }
  if (!delegate_->HandleIdReleased(id)) {
    response_sender.Run(dbus::ErrorResponse::FromMethodCall(
        method_call, DBUS_ERROR_FAILED, std::string()));
    return;
  }
  response_sender.Run(dbus::Response::FromMethodCall(method_call));
}

}  // namespace chromeos
