// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_DBUS_FAKE_BLUETOOTH_MEDIA_CLIENT_H_
#define DEVICE_BLUETOOTH_DBUS_FAKE_BLUETOOTH_MEDIA_CLIENT_H_

#include <stdint.h>

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "dbus/object_path.h"
#include "device/bluetooth/bluetooth_export.h"
#include "device/bluetooth/dbus/bluetooth_media_client.h"

namespace bluez {

class FakeBluetoothMediaEndpointServiceProvider;

class DEVICE_BLUETOOTH_EXPORT FakeBluetoothMediaClient
    : public BluetoothMediaClient {
 public:
  // The default codec is SBC(0x00).
  static const uint8_t kDefaultCodec;

  FakeBluetoothMediaClient();
  ~FakeBluetoothMediaClient() override;

  // DBusClient override.
  void Init(dbus::Bus* bus, const std::string& bluetooth_service_name) override;

  // BluetoothMediaClient overrides.
  void AddObserver(BluetoothMediaClient::Observer* observer) override;
  void RemoveObserver(BluetoothMediaClient::Observer* observer) override;
  void RegisterEndpoint(const dbus::ObjectPath& object_path,
                        const dbus::ObjectPath& endpoint_path,
                        const EndpointProperties& properties,
                        const base::Closure& callback,
                        const ErrorCallback& error_callback) override;
  void UnregisterEndpoint(const dbus::ObjectPath& object_path,
                          const dbus::ObjectPath& endpoint_path,
                          const base::Closure& callback,
                          const ErrorCallback& error_callback) override;

  // Makes the media object visible/invisible to emulate the addition/removal
  // events.
  void SetVisible(bool visible);

  // Sets the registration state for a given media endpoint.
  void SetEndpointRegistered(
      FakeBluetoothMediaEndpointServiceProvider* endpoint,
      bool registered);

  // Indicates whether the given endpoint path is registered or not.
  bool IsRegistered(const dbus::ObjectPath& endpoint_path);

 private:
  // Indicates whether the media object is visible or not.
  bool visible_;

  // The path of the media object.
  dbus::ObjectPath object_path_;

  // Map of registered endpoints. Each pair is composed of an endpoint path as
  // key and a pointer to the endpoint as value.
  std::map<dbus::ObjectPath, FakeBluetoothMediaEndpointServiceProvider*>
      endpoints_;

  // List of observers interested in event notifications from us.
  base::ObserverList<BluetoothMediaClient::Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(FakeBluetoothMediaClient);
};

}  // namespace bluez

#endif  // DEVICE_BLUETOOTH_DBUS_FAKE_BLUETOOTH_MEDIA_CLIENT_H_
