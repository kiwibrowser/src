// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_DBUS_BLUETOOTH_MEDIA_CLIENT_H_
#define DEVICE_BLUETOOTH_DBUS_BLUETOOTH_MEDIA_CLIENT_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/values.h"
#include "dbus/object_path.h"
#include "dbus/property.h"
#include "device/bluetooth/bluetooth_export.h"
#include "device/bluetooth/dbus/bluez_dbus_client.h"

namespace bluez {

// BluetoothMediaClient is used to communicate with the Media interface of a
// local Bluetooth adapter.
class DEVICE_BLUETOOTH_EXPORT BluetoothMediaClient : public BluezDBusClient {
 public:
  // Properties used to register a Media Endpoint.
  struct DEVICE_BLUETOOTH_EXPORT EndpointProperties {
    EndpointProperties();
    ~EndpointProperties();

    // UUID of the profile implemented by the endpoint.
    std::string uuid;

    // Assigned codec value supported by the endpoint. The byte should match the
    // codec specification indicated by the UUID.
    // Since SBC codec is mandatory for A2DP, the default value of codec should
    // be 0x00.
    uint8_t codec;

    // Capabilities of the endpoints. The order of bytes should match the bit
    // arrangement in the specification indicated by the UUID.
    std::vector<uint8_t> capabilities;
  };

  class Observer {
   public:
    virtual ~Observer() {}

    // Called when the Media object with object path |object_path| is added to
    // the system.
    virtual void MediaAdded(const dbus::ObjectPath& object_path) {}

    // Called when the Media object with object path |object_path| is removed
    // from the system.
    virtual void MediaRemoved(const dbus::ObjectPath& object_path) {}
  };

  // Constants used to indicate exceptional error conditions.
  static const char kNoResponseError[];

  // The string representation for the 128-bit UUID for A2DP Sink.
  static const char kBluetoothAudioSinkUUID[];

  ~BluetoothMediaClient() override;

  // The ErrorCallback is used by media API methods to indicate failure.
  // It receives two arguments: the name of the error in |error_name| and
  // an optional message in |error_message|.
  typedef base::Callback<void(const std::string& error_name,
                              const std::string& error_message)> ErrorCallback;

  // Adds and removes observers for events on all Media objects. Check the
  // |object_path| parameter of observer methods to determine which Media object
  // is issuing the event.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Registers a media endpoint to sender at the D-Bus object path
  // |endpoint_path|. |properties| specifies profile UUID which the endpoint is
  // for, Codec implemented by the endpoint and the Capabilities of the
  // endpoint.
  virtual void RegisterEndpoint(const dbus::ObjectPath& object_path,
                                const dbus::ObjectPath& endpoint_path,
                                const EndpointProperties& properties,
                                const base::Closure& callback,
                                const ErrorCallback& error_callback) = 0;

  // Unregisters the media endpoint with the D-Bus object path |endpoint_path|.
  virtual void UnregisterEndpoint(const dbus::ObjectPath& object_path,
                                  const dbus::ObjectPath& endpoint_path,
                                  const base::Closure& callback,
                                  const ErrorCallback& error_callback) = 0;

  // TODO(mcchou): The RegisterPlayer and UnregisterPlayer methods are not
  // included, since they are not used. These two methods may be added later.

  static BluetoothMediaClient* Create();

 protected:
  BluetoothMediaClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothMediaClient);
};

}  // namespace bluez

#endif  // DEVICE_BLUETOOTH_DBUS_BLUETOOTH_MEDIA_CLIENT_H_
