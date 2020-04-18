// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_DBUS_BLUETOOTH_AGENT_MANAGER_CLIENT_H_
#define DEVICE_BLUETOOTH_DBUS_BLUETOOTH_AGENT_MANAGER_CLIENT_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/values.h"
#include "dbus/object_path.h"
#include "device/bluetooth/bluetooth_export.h"
#include "device/bluetooth/dbus/bluez_dbus_client.h"

namespace bluez {

// BluetoothAgentManagerClient is used to communicate with the agent manager
// object of the Bluetooth daemon.
class DEVICE_BLUETOOTH_EXPORT BluetoothAgentManagerClient
    : public BluezDBusClient {
 public:
  ~BluetoothAgentManagerClient() override;

  // The ErrorCallback is used by agent manager methods to indicate failure.
  // It receives two arguments: the name of the error in |error_name| and
  // an optional message in |error_message|.
  typedef base::Callback<void(const std::string& error_name,
                              const std::string& error_message)> ErrorCallback;

  // Registers an agent within the local process at the D-bus object path
  // |agent_path| with the remote agent manager. The agent is used for pairing
  // and for authorization of incoming connection requests. |capability|
  // specifies the input and display capabilities of the agent and should be
  // one of the constants declared in the bluetooth_agent_manager:: namespace.
  virtual void RegisterAgent(const dbus::ObjectPath& agent_path,
                             const std::string& capability,
                             const base::Closure& callback,
                             const ErrorCallback& error_callback) = 0;

  // Unregisters the agent with the D-Bus object path |agent_path| from the
  // remote agent manager.
  virtual void UnregisterAgent(const dbus::ObjectPath& agent_path,
                               const base::Closure& callback,
                               const ErrorCallback& error_callback) = 0;

  // Requests that the agent with the D-Bus object path |agent_path| be made
  // the default.
  virtual void RequestDefaultAgent(const dbus::ObjectPath& agent_path,
                                   const base::Closure& callback,
                                   const ErrorCallback& error_callback) = 0;

  // Creates the instance.
  static BluetoothAgentManagerClient* Create();

  // Constants used to indicate exceptional error conditions.
  static const char kNoResponseError[];

 protected:
  BluetoothAgentManagerClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothAgentManagerClient);
};

}  // namespace bluez

#endif  // DEVICE_BLUETOOTH_DBUS_BLUETOOTH_AGENT_MANAGER_CLIENT_H_
