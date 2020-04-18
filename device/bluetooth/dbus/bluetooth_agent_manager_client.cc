// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/dbus/bluetooth_agent_manager_client.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace bluez {

const char BluetoothAgentManagerClient::kNoResponseError[] =
    "org.chromium.Error.NoResponse";

// The BluetoothAgentManagerClient implementation used in production.
class BluetoothAgentManagerClientImpl : public BluetoothAgentManagerClient {
 public:
  BluetoothAgentManagerClientImpl() : weak_ptr_factory_(this) {}

  ~BluetoothAgentManagerClientImpl() override = default;

  // BluetoothAgentManagerClient override.
  void RegisterAgent(const dbus::ObjectPath& agent_path,
                     const std::string& capability,
                     const base::Closure& callback,
                     const ErrorCallback& error_callback) override {
    dbus::MethodCall method_call(
        bluetooth_agent_manager::kBluetoothAgentManagerInterface,
        bluetooth_agent_manager::kRegisterAgent);

    dbus::MessageWriter writer(&method_call);
    writer.AppendObjectPath(agent_path);
    writer.AppendString(capability);

    object_proxy_->CallMethodWithErrorCallback(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&BluetoothAgentManagerClientImpl::OnSuccess,
                       weak_ptr_factory_.GetWeakPtr(), callback),
        base::BindOnce(&BluetoothAgentManagerClientImpl::OnError,
                       weak_ptr_factory_.GetWeakPtr(), error_callback));
  }

  // BluetoothAgentManagerClient override.
  void UnregisterAgent(const dbus::ObjectPath& agent_path,
                       const base::Closure& callback,
                       const ErrorCallback& error_callback) override {
    dbus::MethodCall method_call(
        bluetooth_agent_manager::kBluetoothAgentManagerInterface,
        bluetooth_agent_manager::kUnregisterAgent);

    dbus::MessageWriter writer(&method_call);
    writer.AppendObjectPath(agent_path);

    object_proxy_->CallMethodWithErrorCallback(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&BluetoothAgentManagerClientImpl::OnSuccess,
                       weak_ptr_factory_.GetWeakPtr(), callback),
        base::BindOnce(&BluetoothAgentManagerClientImpl::OnError,
                       weak_ptr_factory_.GetWeakPtr(), error_callback));
  }

  // BluetoothAgentManagerClient override.
  void RequestDefaultAgent(const dbus::ObjectPath& agent_path,
                           const base::Closure& callback,
                           const ErrorCallback& error_callback) override {
    dbus::MethodCall method_call(
        bluetooth_agent_manager::kBluetoothAgentManagerInterface,
        bluetooth_agent_manager::kRequestDefaultAgent);

    dbus::MessageWriter writer(&method_call);
    writer.AppendObjectPath(agent_path);

    object_proxy_->CallMethodWithErrorCallback(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&BluetoothAgentManagerClientImpl::OnSuccess,
                       weak_ptr_factory_.GetWeakPtr(), callback),
        base::BindOnce(&BluetoothAgentManagerClientImpl::OnError,
                       weak_ptr_factory_.GetWeakPtr(), error_callback));
  }

 protected:
  void Init(dbus::Bus* bus,
            const std::string& bluetooth_service_name) override {
    DCHECK(bus);
    object_proxy_ = bus->GetObjectProxy(
        bluetooth_service_name,
        dbus::ObjectPath(
            bluetooth_agent_manager::kBluetoothAgentManagerServicePath));
  }

 private:
  // Called when a response for successful method call is received.
  void OnSuccess(const base::Closure& callback, dbus::Response* response) {
    DCHECK(response);
    callback.Run();
  }

  // Called when a response for a failed method call is received.
  void OnError(const ErrorCallback& error_callback,
               dbus::ErrorResponse* response) {
    // Error response has optional error message argument.
    std::string error_name;
    std::string error_message;
    if (response) {
      dbus::MessageReader reader(response);
      error_name = response->GetErrorName();
      reader.PopString(&error_message);
    } else {
      error_name = kNoResponseError;
      error_message = "";
    }
    error_callback.Run(error_name, error_message);
  }

  dbus::ObjectProxy* object_proxy_;

  // Weak pointer factory for generating 'this' pointers that might live longer
  // than we do.
  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<BluetoothAgentManagerClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothAgentManagerClientImpl);
};

BluetoothAgentManagerClient::BluetoothAgentManagerClient() = default;

BluetoothAgentManagerClient::~BluetoothAgentManagerClient() = default;

BluetoothAgentManagerClient* BluetoothAgentManagerClient::Create() {
  return new BluetoothAgentManagerClientImpl();
}

}  // namespace bluez
