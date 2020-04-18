// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_STATE_TEST_H_
#define CHROMEOS_NETWORK_NETWORK_STATE_TEST_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/shill_manager_client.h"
#include "chromeos/network/network_state_handler.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

// Class for tests that need NetworkStateHandler. Handles initialization,
// shutdown, and adds default profiles and a wifi device.
class NetworkStateTest : public testing::Test {
 public:
  NetworkStateTest();
  ~NetworkStateTest() override;

  // DBusThreadManager::Initialize() must be called before SetUp().
  void SetUp() override;
  void TearDown() override;

  // Call this before TearDown() to shut down NetworkStateHandler.
  void ShutdownNetworkState();

  // Configures a new service using Shill properties from |shill_json_string|
  // which must include a GUID and Type. Returns the service path, or "" if the
  // service could not be configured.
  std::string ConfigureService(const std::string& shill_json_string);

  // Returns a string value for property |key| associated with |service_path|.
  // The result will be empty if the service or property do not exist.
  std::string GetServiceStringProperty(const std::string& service_path,
                                       const std::string& key);

  void SetServiceProperty(const std::string& service_path,
                          const std::string& key,
                          const base::Value& value);

  ShillManagerClient::TestInterface* test_manager_client() {
    return test_manager_client_;
  }
  NetworkStateHandler* network_state_handler() {
    return network_state_handler_.get();
  }

  static const char kUserHash[];

 private:
  void ConfigureCallback(const dbus::ObjectPath& result);

  std::string last_created_service_path_;

  ShillManagerClient::TestInterface* test_manager_client_;
  std::unique_ptr<NetworkStateHandler> network_state_handler_;

  base::WeakPtrFactory<NetworkStateTest> weak_ptr_factory_;
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_STATE_TEST_H_
