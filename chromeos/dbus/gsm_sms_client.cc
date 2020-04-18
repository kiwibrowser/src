// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chromeos/dbus/gsm_sms_client.h"

#include <stdint.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "dbus/values_util.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

// A class actually making method calls for SMS services, used by
// GsmSMSClientImpl.
class SMSProxy {
 public:
  typedef GsmSMSClient::SmsReceivedHandler SmsReceivedHandler;

  SMSProxy(dbus::Bus* bus,
           const std::string& service_name,
           const dbus::ObjectPath& object_path)
      : proxy_(bus->GetObjectProxy(service_name, object_path)),
        weak_ptr_factory_(this) {
    proxy_->ConnectToSignal(
        modemmanager::kModemManagerSMSInterface,
        modemmanager::kSMSReceivedSignal,
        base::Bind(&SMSProxy::OnSmsReceived, weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&SMSProxy::OnSignalConnected,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  // Sets SmsReceived signal handler.
  void SetSmsReceivedHandler(const SmsReceivedHandler& handler) {
    DCHECK(sms_received_handler_.is_null());
    sms_received_handler_ = handler;
  }

  // Resets SmsReceived signal handler.
  void ResetSmsReceivedHandler() {
    sms_received_handler_.Reset();
  }

  // Calls Delete method.
  void Delete(uint32_t index, VoidDBusMethodCallback callback) {
    dbus::MethodCall method_call(modemmanager::kModemManagerSMSInterface,
                                 modemmanager::kSMSDeleteFunction);
    dbus::MessageWriter writer(&method_call);
    writer.AppendUint32(index);
    proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SMSProxy::OnDelete, weak_ptr_factory_.GetWeakPtr(),
                       std::move(callback)));
  }

  // Calls Get method.
  void Get(uint32_t index, DBusMethodCallback<base::DictionaryValue> callback) {
    dbus::MethodCall method_call(modemmanager::kModemManagerSMSInterface,
                                 modemmanager::kSMSGetFunction);
    dbus::MessageWriter writer(&method_call);
    writer.AppendUint32(index);
    proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SMSProxy::OnGet, weak_ptr_factory_.GetWeakPtr(),
                       std::move(callback)));
  }

  // Calls List method.
  void List(DBusMethodCallback<base::ListValue> callback) {
    dbus::MethodCall method_call(modemmanager::kModemManagerSMSInterface,
                                 modemmanager::kSMSListFunction);
    proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SMSProxy::OnList, weak_ptr_factory_.GetWeakPtr(),
                       std::move(callback)));
  }

 private:
  // Handles SmsReceived signal.
  void OnSmsReceived(dbus::Signal* signal) {
    uint32_t index = 0;
    bool complete = false;
    dbus::MessageReader reader(signal);
    if (!reader.PopUint32(&index) ||
        !reader.PopBool(&complete)) {
      LOG(ERROR) << "Invalid signal: " << signal->ToString();
      return;
    }
    if (!sms_received_handler_.is_null())
      sms_received_handler_.Run(index, complete);
  }

  // Handles the result of signal connection setup.
  void OnSignalConnected(const std::string& interface,
                         const std::string& signal,
                         bool succeeded) {
    LOG_IF(ERROR, !succeeded) << "Connect to " << interface << " " <<
        signal << " failed.";
  }

  // Handles responses of Delete method calls.
  void OnDelete(VoidDBusMethodCallback callback, dbus::Response* response) {
    std::move(callback).Run(response);
  }

  // Handles responses of Get method calls.
  void OnGet(DBusMethodCallback<base::DictionaryValue> callback,
             dbus::Response* response) {
    if (!response) {
      std::move(callback).Run(base::nullopt);
      return;
    }
    dbus::MessageReader reader(response);
    auto value = base::DictionaryValue::From(dbus::PopDataAsValue(&reader));
    if (!value) {
      LOG(WARNING) << "Invalid response: " << response->ToString();
      std::move(callback).Run(base::nullopt);
      return;
    }
    std::move(callback).Run(std::move(*value));
  }

  // Handles responses of List method calls.
  void OnList(DBusMethodCallback<base::ListValue> callback,
              dbus::Response* response) {
    if (!response) {
      std::move(callback).Run(base::nullopt);
      return;
    }
    dbus::MessageReader reader(response);
    auto value = base::ListValue::From(dbus::PopDataAsValue(&reader));
    if (!value) {
      LOG(WARNING) << "Invalid response: " << response->ToString();
      std::move(callback).Run(base::nullopt);
      return;
    }
    std::move(callback).Run(std::move(*value));
  }

  dbus::ObjectProxy* proxy_;
  SmsReceivedHandler sms_received_handler_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<SMSProxy> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SMSProxy);
};

// The GsmSMSClient implementation.
class GsmSMSClientImpl : public GsmSMSClient {
 public:
  GsmSMSClientImpl() : bus_(NULL) {}

  // GsmSMSClient override.
  void SetSmsReceivedHandler(const std::string& service_name,
                             const dbus::ObjectPath& object_path,
                             const SmsReceivedHandler& handler) override {
    GetProxy(service_name, object_path)->SetSmsReceivedHandler(handler);
  }

  // GsmSMSClient override.
  void ResetSmsReceivedHandler(const std::string& service_name,
                               const dbus::ObjectPath& object_path) override {
    GetProxy(service_name, object_path)->ResetSmsReceivedHandler();
  }

  // GsmSMSClient override.
  void Delete(const std::string& service_name,
              const dbus::ObjectPath& object_path,
              uint32_t index,
              VoidDBusMethodCallback callback) override {
    GetProxy(service_name, object_path)->Delete(index, std::move(callback));
  }

  // GsmSMSClient override.
  void Get(const std::string& service_name,
           const dbus::ObjectPath& object_path,
           uint32_t index,
           DBusMethodCallback<base::DictionaryValue> callback) override {
    GetProxy(service_name, object_path)->Get(index, std::move(callback));
  }

  // GsmSMSClient override.
  void List(const std::string& service_name,
            const dbus::ObjectPath& object_path,
            DBusMethodCallback<base::ListValue> callback) override {
    GetProxy(service_name, object_path)->List(std::move(callback));
  }

  // GsmSMSClient override.
  void RequestUpdate(const std::string& service_name,
                     const dbus::ObjectPath& object_path) override {}

 protected:
  void Init(dbus::Bus* bus) override { bus_ = bus; }

 private:
  using ProxyMap =
      std::map<std::pair<std::string, std::string>, std::unique_ptr<SMSProxy>>;

  // Returns a SMSProxy for the given service name and object path.
  SMSProxy* GetProxy(const std::string& service_name,
                     const dbus::ObjectPath& object_path) {
    const ProxyMap::key_type key(service_name, object_path.value());
    ProxyMap::const_iterator it = proxies_.find(key);
    if (it != proxies_.end())
      return it->second.get();

    // There is no proxy for the service_name and object_path, create it.
    std::unique_ptr<SMSProxy> proxy(
        new SMSProxy(bus_, service_name, object_path));
    SMSProxy* proxy_ptr = proxy.get();
    proxies_[key] = std::move(proxy);
    return proxy_ptr;
  }

  dbus::Bus* bus_;
  ProxyMap proxies_;

  DISALLOW_COPY_AND_ASSIGN(GsmSMSClientImpl);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// GsmSMSClient

GsmSMSClient::GsmSMSClient() = default;

GsmSMSClient::~GsmSMSClient() = default;

// static
GsmSMSClient* GsmSMSClient::Create() {
  return new GsmSMSClientImpl();
}

}  // namespace chromeos
