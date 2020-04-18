// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SHILL_THIRD_PARTY_VPN_DRIVER_CLIENT_H_
#define CHROMEOS_DBUS_SHILL_THIRD_PARTY_VPN_DRIVER_CLIENT_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/shill_client_helper.h"

namespace chromeos {

class ShillThirdPartyVpnObserver;

// ShillThirdPartyVpnDriverClient is used to communicate with the Shill
// ThirdPartyVpnDriver service.
// All methods should be called from the origin thread which initializes the
// DBusThreadManager instance.
class CHROMEOS_EXPORT ShillThirdPartyVpnDriverClient : public DBusClient {
 public:
  class TestInterface {
   public:
    virtual void OnPacketReceived(const std::string& object_path_value,
                                  const std::vector<char>& packet) = 0;
    virtual void OnPlatformMessage(const std::string& object_path_value,
                                   uint32_t message) = 0;

   protected:
    virtual ~TestInterface() {}
  };

  ~ShillThirdPartyVpnDriverClient() override;

  // Factory function, creates a new instance which is owned by the caller.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static ShillThirdPartyVpnDriverClient* Create();

  // Adds an |observer| for the third party vpn driver at |object_path_value|.
  virtual void AddShillThirdPartyVpnObserver(
      const std::string& object_path_value,
      ShillThirdPartyVpnObserver* observer) = 0;

  // Removes an |observer| for the third party vpn driver at
  // |object_path_value|.
  virtual void RemoveShillThirdPartyVpnObserver(
      const std::string& object_path_value) = 0;

  // Calls SetParameters method.
  // |callback| is called after the method call succeeds.
  virtual void SetParameters(
      const std::string& object_path_value,
      const base::DictionaryValue& parameters,
      const ShillClientHelper::StringCallback& callback,
      const ShillClientHelper::ErrorCallback& error_callback) = 0;

  // Calls UpdateConnectionState method.
  // |callback| is called after the method call succeeds.
  virtual void UpdateConnectionState(
      const std::string& object_path_value,
      const uint32_t connection_state,
      const base::Closure& callback,
      const ShillClientHelper::ErrorCallback& error_callback) = 0;

  // Calls SendPacket method.
  // |callback| is called after the method call succeeds.
  virtual void SendPacket(
      const std::string& object_path_value,
      const std::vector<char>& ip_packet,
      const base::Closure& callback,
      const ShillClientHelper::ErrorCallback& error_callback) = 0;

  // Returns an interface for testing (stub only), or returns nullptr.
  virtual ShillThirdPartyVpnDriverClient::TestInterface* GetTestInterface() = 0;

 protected:
  friend class ShillThirdPartyVpnDriverClientTest;

  // Create() should be used instead.
  ShillThirdPartyVpnDriverClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(ShillThirdPartyVpnDriverClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SHILL_THIRD_PARTY_VPN_DRIVER_CLIENT_H_
