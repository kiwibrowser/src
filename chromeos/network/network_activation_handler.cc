// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_activation_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_service_client.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_handler.h"
#include "dbus/object_proxy.h"

namespace chromeos {

// static
const char NetworkActivationHandler::kErrorShillError[] = "shill-error";

NetworkActivationHandler::NetworkActivationHandler() = default;
NetworkActivationHandler::~NetworkActivationHandler() = default;

void NetworkActivationHandler::Activate(
    const std::string& service_path,
    const std::string& carrier,
    const base::Closure& success_callback,
    const network_handler::ErrorCallback& error_callback) {
  NET_LOG_USER("ActivateNetwork", service_path);
  CallShillActivate(service_path, carrier, success_callback, error_callback);
}

void NetworkActivationHandler::CompleteActivation(
    const std::string& service_path,
    const base::Closure& success_callback,
    const network_handler::ErrorCallback& error_callback) {
  NET_LOG_USER("CompleteActivation", service_path);
  CallShillCompleteActivation(service_path, success_callback, error_callback);
}

void NetworkActivationHandler::CallShillActivate(
    const std::string& service_path,
    const std::string& carrier,
    const base::Closure& success_callback,
    const network_handler::ErrorCallback& error_callback) {
  NET_LOG_USER("Activation Request", service_path + ": '" + carrier + "'");
  DBusThreadManager::Get()->GetShillServiceClient()->ActivateCellularModem(
      dbus::ObjectPath(service_path),
      carrier,
      base::Bind(&NetworkActivationHandler::HandleShillSuccess,
                 AsWeakPtr(), service_path, success_callback),
      base::Bind(&network_handler::ShillErrorCallbackFunction,
                 kErrorShillError, service_path, error_callback));
}

void NetworkActivationHandler::CallShillCompleteActivation(
    const std::string& service_path,
    const base::Closure& success_callback,
    const network_handler::ErrorCallback& error_callback) {
  NET_LOG_USER("CompleteActivation Request", service_path);
  DBusThreadManager::Get()->GetShillServiceClient()->CompleteCellularActivation(
      dbus::ObjectPath(service_path),
      base::Bind(&NetworkActivationHandler::HandleShillSuccess,
                 AsWeakPtr(), service_path, success_callback),
      base::Bind(&network_handler::ShillErrorCallbackFunction,
                 kErrorShillError, service_path, error_callback));
}

void NetworkActivationHandler::HandleShillSuccess(
    const std::string& service_path,
    const base::Closure& success_callback) {
  if (!success_callback.is_null())
    success_callback.Run();
}

}  // namespace chromeos
