// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_DBUS_FAKE_BLUETOOTH_MEDIA_TRANSPORT_CLIENT_H_
#define DEVICE_BLUETOOTH_DBUS_FAKE_BLUETOOTH_MEDIA_TRANSPORT_CLIENT_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "dbus/object_path.h"
#include "device/bluetooth/bluetooth_export.h"
#include "device/bluetooth/dbus/bluetooth_media_transport_client.h"

namespace bluez {

class FakeBluetoothMediaEndpointServiceProvider;

class DEVICE_BLUETOOTH_EXPORT FakeBluetoothMediaTransportClient
    : public BluetoothMediaTransportClient {
 public:
  struct Properties : public BluetoothMediaTransportClient::Properties {
    explicit Properties(const PropertyChangedCallback& callback);
    ~Properties() override;

    void Get(dbus::PropertyBase* property,
             dbus::PropertySet::GetCallback callback) override;
    void GetAll() override;
    void Set(dbus::PropertyBase* property,
             dbus::PropertySet::SetCallback callback) override;
  };

  // The default path of the transport object.
  static const char kTransportPath[];

  // The default properties including device, codec, configuration, state, delay
  // and volume, owned by a fake media transport object we emulate.
  static const char kTransportDevicePath[];
  static const uint8_t kTransportCodec;
  static const uint8_t kTransportConfiguration[];
  static const uint8_t kTransportConfigurationLength;
  static const uint16_t kTransportDelay;
  static const uint16_t kTransportVolume;

  // The default MTUs for read and write.
  static const uint16_t kDefaultReadMtu;
  static const uint16_t kDefaultWriteMtu;

  FakeBluetoothMediaTransportClient();
  ~FakeBluetoothMediaTransportClient() override;

  // DBusClient override.
  void Init(dbus::Bus* bus, const std::string& bluetooth_service_name) override;

  // BluetoothMediaTransportClient override.
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  Properties* GetProperties(const dbus::ObjectPath& object_path) override;
  void Acquire(const dbus::ObjectPath& object_path,
               const AcquireCallback& callback,
               const ErrorCallback& error_callback) override;
  void TryAcquire(const dbus::ObjectPath& object_path,
                  const AcquireCallback& callback,
                  const ErrorCallback& error_callback) override;
  void Release(const dbus::ObjectPath& object_path,
               const base::Closure& callback,
               const ErrorCallback& error_callback) override;

  // Makes the transport valid/invalid for a given media endpoint. The transport
  // object is assigned to the given endpoint if valid is true, false
  // otherwise.
  void SetValid(FakeBluetoothMediaEndpointServiceProvider* endpoint,
                bool valid);

  // Set state/volume property to a certain value.
  void SetState(const dbus::ObjectPath& endpoint_path,
                const std::string& state);
  void SetVolume(const dbus::ObjectPath& endpoint_path, const uint16_t& volume);

  // Writes bytes to the input file descriptor, |input_fd|, associated with a
  // transport object which is bound to |endpoint_path|.
  void WriteData(const dbus::ObjectPath& endpoint_path,
                 const std::vector<char>& bytes);

  // Retrieves the transport object path bound to |endpoint_path|.
  dbus::ObjectPath GetTransportPath(const dbus::ObjectPath& endpoint_path);

 private:
  // This class is used for simulating the scenario where each media endpoint
  // has a corresponding transport path and properties. Once an endpoint is
  // assigned with a transport path, an object of Transport is created.
  struct Transport {
    Transport(const dbus::ObjectPath& transport_path,
              std::unique_ptr<Properties> transport_properties);
    ~Transport();

    // An unique transport path.
    dbus::ObjectPath path;

    // The property set bound with |path|.
    std::unique_ptr<Properties> properties;

    // This is the internal end of socketpair created for simulation purposes.
    // |input_fd| will be initialized when Acquire/TryAcquire is called.
    std::unique_ptr<base::File> input_fd;
  };

  // Property callback passed while a Properties structure is created.
  void OnPropertyChanged(const std::string& property_name);

  // Gets the endpoint path associated with the given transport_path.
  dbus::ObjectPath GetEndpointPath(const dbus::ObjectPath& transport_path);

  // Retrieves the transport structure bound to |endpoint_path|.
  Transport* GetTransport(const dbus::ObjectPath& endpoint_path);

  // Retrieves the transport structure with |transport_path|.
  Transport* GetTransportByPath(const dbus::ObjectPath& transport_path);

  // Helper function used by Acquire and TryAcquire to set up the sockpair and
  // invoke callback/error_callback.
  void AcquireInternal(bool try_flag,
                       const dbus::ObjectPath& object_path,
                       const AcquireCallback& callback,
                       const ErrorCallback& error_callback);

  // Map of endpoints with valid transport. Each pair is composed of an endpoint
  // path and a Transport structure containing a transport path and its
  // properties.
  std::map<dbus::ObjectPath, std::unique_ptr<Transport>>
      endpoint_to_transport_map_;

  // Map of valid transports. Each pair is composed of a transport path as the
  // key and an endpoint path as the value. This map is used to get the
  // corresponding endpoint path when GetProperties() is called.
  std::map<dbus::ObjectPath, dbus::ObjectPath> transport_to_endpoint_map_;

  base::ObserverList<BluetoothMediaTransportClient::Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(FakeBluetoothMediaTransportClient);
};

}  // namespace bluez

#endif  // DEVICE_BLUETOOTH_DBUS_FAKE_BLUETOOTH_MEDIA_TRANSPORT_CLIENT_H_
