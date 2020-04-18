// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_state_test.h"

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/run_loop.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_device_client.h"
#include "chromeos/dbus/shill_profile_client.h"
#include "chromeos/dbus/shill_service_client.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/onc/onc_utils.h"
#include "dbus/object_path.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

void FailErrorCallback(const std::string& error_name,
                       const std::string& error_message) {}

const char NetworkStateTest::kUserHash[] = "user_hash";

NetworkStateTest::NetworkStateTest()
    : test_manager_client_(nullptr), weak_ptr_factory_(this) {}

NetworkStateTest::~NetworkStateTest() = default;

void NetworkStateTest::SetUp() {
  DBusThreadManager* dbus_manager = DBusThreadManager::Get();
  CHECK(dbus_manager)
      << "NetworkStateTest requires DBusThreadManager::Initialize()";

  test_manager_client_ =
      dbus_manager->GetShillManagerClient()->GetTestInterface();

  dbus_manager->GetShillProfileClient()->GetTestInterface()->AddProfile(
      "shared_profile_path", std::string() /* shared profile */);
  dbus_manager->GetShillProfileClient()->GetTestInterface()->AddProfile(
      "user_profile_path", kUserHash);
  test_manager_client_->AddTechnology(shill::kTypeWifi, true /* enabled */);
  dbus_manager->GetShillDeviceClient()->GetTestInterface()->AddDevice(
      "/device/wifi1", shill::kTypeWifi, "wifi_device1");

  base::RunLoop().RunUntilIdle();

  network_state_handler_ = NetworkStateHandler::InitializeForTest();
}

void NetworkStateTest::TearDown() {
  network_state_handler_.reset();
}

void NetworkStateTest::ShutdownNetworkState() {
  network_state_handler_->Shutdown();
}

std::string NetworkStateTest::ConfigureService(
    const std::string& shill_json_string) {
  last_created_service_path_ = "";

  std::unique_ptr<base::DictionaryValue> shill_json_dict =
      onc::ReadDictionaryFromJson(shill_json_string);
  if (!shill_json_dict) {
    LOG(ERROR) << "Error parsing json: " << shill_json_string;
    return last_created_service_path_;
  }

  // As a result of the ConfigureService() and RunUntilIdle() calls below,
  // ConfigureCallback() will be invoked before the end of this function, so
  // |last_created_service_path| will be set before it is returned. In
  // error cases, ConfigureCallback() will not run, resulting in "" being
  // returned from this function.
  DBusThreadManager::Get()->GetShillManagerClient()->ConfigureService(
      *shill_json_dict,
      base::Bind(&NetworkStateTest::ConfigureCallback,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&FailErrorCallback));
  base::RunLoop().RunUntilIdle();

  return last_created_service_path_;
}

void NetworkStateTest::ConfigureCallback(const dbus::ObjectPath& result) {
  last_created_service_path_ = result.value();
}

std::string NetworkStateTest::GetServiceStringProperty(
    const std::string& service_path,
    const std::string& key) {
  const base::DictionaryValue* properties =
      DBusThreadManager::Get()
          ->GetShillServiceClient()
          ->GetTestInterface()
          ->GetServiceProperties(service_path);
  std::string result;
  if (properties)
    properties->GetStringWithoutPathExpansion(key, &result);
  return result;
}

void NetworkStateTest::SetServiceProperty(const std::string& service_path,
                                          const std::string& key,
                                          const base::Value& value) {
  DBusThreadManager::Get()
      ->GetShillServiceClient()
      ->GetTestInterface()
      ->SetServiceProperty(service_path, key, value);
  base::RunLoop().RunUntilIdle();
}

}  // namespace chromeos
