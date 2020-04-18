// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <map>
#include <set>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_piece.h"
#include "base/values.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_shill_profile_client.h"
#include "chromeos/dbus/fake_shill_service_client.h"
#include "chromeos/network/network_configuration_handler.h"
#include "chromeos/network/network_configuration_observer.h"
#include "chromeos/network/network_profile_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "chromeos/network/shill_property_util.h"
#include "chromeos/network/tether_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

// Copies the result of GetProperties().
void CopyProperties(bool* called,
                    std::string* service_path_out,
                    base::Value* result_out,
                    const std::string& service_path,
                    const base::DictionaryValue& result) {
  *called = true;
  *service_path_out = service_path;
  *result_out = result.Clone();
}

// Copies service_path and guid returned for CreateShillConfiguration.
void CopyServiceResult(bool* called,
                       std::string* service_path_out,
                       std::string* guid_out,
                       const std::string& service_path,
                       const std::string& guid) {
  *called = true;
  *service_path_out = service_path;
  *guid_out = guid;
}

std::string PrettyJson(const base::DictionaryValue& value) {
  std::string pretty;
  base::JSONWriter::WriteWithOptions(
      value, base::JSONWriter::OPTIONS_PRETTY_PRINT, &pretty);
  return pretty;
}

void ErrorCallback(const std::string& error_name,
                   std::unique_ptr<base::DictionaryValue> error_data) {
  ADD_FAILURE() << "Unexpected error: " << error_name
                << " with associated data: \n"
                << PrettyJson(*error_data);
}

void RecordError(std::string* error_name_ptr,
                 const std::string& error_name,
                 std::unique_ptr<base::DictionaryValue> error_data) {
  *error_name_ptr = error_name;
}

class TestCallback {
 public:
  TestCallback() : run_count_(0) {}
  void Run() { ++run_count_; }
  int run_count() const { return run_count_; }

 private:
  int run_count_;
};

class TestNetworkConfigurationObserver : public NetworkConfigurationObserver {
 public:
  TestNetworkConfigurationObserver() = default;

  // NetworkConfigurationObserver
  void OnConfigurationCreated(
      const std::string& service_path,
      const std::string& profile_path,
      const base::DictionaryValue& properties,
      NetworkConfigurationObserver::Source source) override {
    ASSERT_EQ(0u, configurations_.count(service_path));
    configurations_[service_path] = properties.CreateDeepCopy();
    profiles_[profile_path].insert(service_path);
  }

  void OnConfigurationRemoved(
      const std::string& service_path,
      const std::string& guid,
      NetworkConfigurationObserver::Source source) override {
    ASSERT_EQ(1u, configurations_.count(service_path));
    configurations_.erase(service_path);
    for (auto& p : profiles_) {
      p.second.erase(service_path);
    }
  }

  void OnConfigurationProfileChanged(
      const std::string& service_path,
      const std::string& profile_path,
      NetworkConfigurationObserver::Source source) override {
    for (auto& p : profiles_) {
      p.second.erase(service_path);
    }
    profiles_[profile_path].insert(service_path);
  }

  void OnPropertiesSet(const std::string& service_path,
                       const std::string& guid,
                       const base::DictionaryValue& set_properties,
                       NetworkConfigurationObserver::Source source) override {
    configurations_[service_path]->MergeDictionary(&set_properties);
  }

  bool HasConfiguration(const std::string& service_path) {
    return configurations_.count(service_path) == 1;
  }

  std::string GetStringProperty(const std::string& service_path,
                                const std::string& property) {
    if (!HasConfiguration(service_path))
      return "";
    std::string result;
    configurations_[service_path]->GetStringWithoutPathExpansion(property,
                                                                 &result);
    return result;
  }

  bool HasConfigurationInProfile(const std::string& service_path,
                                 const std::string& profile_path) {
    if (profiles_.count(profile_path) == 0)
      return false;
    return profiles_[profile_path].count(service_path) == 1;
  }

 private:
  std::map<std::string, std::unique_ptr<base::DictionaryValue>> configurations_;
  std::map<std::string, std::set<std::string>> profiles_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkConfigurationObserver);
};

class TestNetworkStateHandlerObserver
    : public chromeos::NetworkStateHandlerObserver {
 public:
  TestNetworkStateHandlerObserver() = default;

  // Returns the number of NetworkListChanged() call.
  size_t network_list_changed_count() const {
    return network_list_changed_count_;
  }

  // Returns the number of NetworkPropertiesUpdated() call for the
  // given |service_path|.
  int PropertyUpdatesForService(const std::string& service_path) const {
    auto iter = property_updates_.find(service_path);
    return iter == property_updates_.end() ? 0 : iter->second;
  }

  // chromeos::NetworkStateHandlerObserver overrides:
  void NetworkListChanged() override { ++network_list_changed_count_; }
  void NetworkPropertiesUpdated(const NetworkState* network) override {
    property_updates_[network->path()]++;
  }

 private:
  size_t network_list_changed_count_ = 0;
  std::map<std::string, int> property_updates_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkStateHandlerObserver);
};

}  // namespace

class NetworkConfigurationHandlerTest : public testing::Test {
 public:
  NetworkConfigurationHandlerTest() {
    DBusThreadManager::Initialize();

    network_state_handler_ = NetworkStateHandler::InitializeForTest();
    // Note: NetworkConfigurationHandler's contructor is private, so
    // std::make_unique cannot be used.
    network_configuration_handler_.reset(new NetworkConfigurationHandler());
    network_configuration_handler_->Init(network_state_handler_.get(),
                                         nullptr /* network_device_handler */);
    base::RunLoop().RunUntilIdle();
    network_state_handler_observer_ =
        std::make_unique<TestNetworkStateHandlerObserver>();
    network_state_handler_->AddObserver(network_state_handler_observer_.get(),
                                        FROM_HERE);
  }

  ~NetworkConfigurationHandlerTest() override {
    network_state_handler_->Shutdown();
    network_state_handler_->RemoveObserver(
        network_state_handler_observer_.get(), FROM_HERE);
    network_state_handler_observer_.reset();
    network_configuration_handler_.reset();
    network_state_handler_.reset();

    DBusThreadManager::Shutdown();
  }

  void SuccessCallback(const std::string& callback_name) {
    success_callback_name_ = callback_name;
  }

  void GetPropertiesCallback(const std::string& service_path,
                             const base::DictionaryValue& dictionary) {
    get_properties_path_ = service_path;
    get_properties_ = dictionary.CreateDeepCopy();
  }

  void CreateConfigurationCallback(const std::string& service_path,
                                   const std::string& guid) {
    create_service_path_ = service_path;
  }

  void CreateTestConfiguration(const std::string& service_path,
                               const std::string& type) {
    base::DictionaryValue properties;
    shill_property_util::SetSSID(service_path, &properties);
    properties.SetKey(shill::kNameProperty, base::Value(service_path));
    properties.SetKey(shill::kGuidProperty, base::Value(service_path));
    properties.SetKey(shill::kTypeProperty, base::Value(type));
    properties.SetKey(shill::kStateProperty, base::Value(shill::kStateIdle));
    properties.SetKey(
        shill::kProfileProperty,
        base::Value(NetworkProfileHandler::GetSharedProfilePath()));

    network_configuration_handler_->CreateShillConfiguration(
        properties, NetworkConfigurationObserver::SOURCE_USER_ACTION,
        base::Bind(
            &NetworkConfigurationHandlerTest::CreateConfigurationCallback,
            base::Unretained(this)),
        base::Bind(&ErrorCallback));
    base::RunLoop().RunUntilIdle();
  }

  // Creates two profiles "profile1" and "profile2", and two services
  // "/service/1" and "/service/2", and ties for four combinations.
  // "/service/2"'s current profile is "profile2".
  void SetUpRemovableConfiguration() {
    // Create two profiles.
    GetShillProfileClient()->AddProfile("profile1", "abcde");
    GetShillProfileClient()->AddProfile("profile2", "vwxyz");

    // Create two services.
    GetShillServiceClient()->AddService(
        "/service/1", std::string() /* guid */, std::string() /* name */,
        "wifi", std::string() /* state */, true /* visible */);
    GetShillServiceClient()->AddService(
        "/service/2", std::string() /* guid */, std::string() /* name */,
        "wifi", std::string() /* state */, true /* visible */);

    // Register "/service/2" to "profile2".
    GetShillProfileClient()->AddService("profile2", "/service/2");

    // Tie profiles and services.
    const base::DictionaryValue* service_properties_1 =
        GetShillServiceClient()->GetServiceProperties("/service/1");
    const base::DictionaryValue* service_properties_2 =
        GetShillServiceClient()->GetServiceProperties("/service/2");
    ASSERT_TRUE(service_properties_1);
    ASSERT_TRUE(service_properties_2);
    GetShillProfileClient()->AddEntry("profile1", "/service/1",
                                      *service_properties_1);
    GetShillProfileClient()->AddEntry("profile1", "/service/2",
                                      *service_properties_2);
    GetShillProfileClient()->AddEntry("profile2", "/service/1",
                                      *service_properties_1);
    GetShillProfileClient()->AddEntry("profile2", "/service/2",
                                      *service_properties_2);
    base::RunLoop().RunUntilIdle();
  }

 protected:
  bool GetServiceStringProperty(const std::string& service_path,
                                const std::string& key,
                                std::string* result) {
    ShillServiceClient::TestInterface* service_test =
        DBusThreadManager::Get()->GetShillServiceClient()->GetTestInterface();
    const base::DictionaryValue* properties =
        service_test->GetServiceProperties(service_path);
    if (!properties)
      return false;
    const base::Value* value =
        properties->FindKeyOfType(key, base::Value::Type::STRING);
    if (!value)
      return false;
    *result = value->GetString();
    return true;
  }

  bool GetReceivedStringProperty(const std::string& service_path,
                                 const std::string& key,
                                 std::string* result) {
    if (get_properties_path_ != service_path || !get_properties_)
      return false;
    const base::Value* value =
        get_properties_->FindKeyOfType(key, base::Value::Type::STRING);
    if (!value)
      return false;
    *result = value->GetString();
    return true;
  }

  FakeShillServiceClient* GetShillServiceClient() {
    return static_cast<FakeShillServiceClient*>(
        DBusThreadManager::Get()->GetShillServiceClient());
  }

  FakeShillProfileClient* GetShillProfileClient() {
    return static_cast<FakeShillProfileClient*>(
        DBusThreadManager::Get()->GetShillProfileClient());
  }

  std::unique_ptr<NetworkStateHandler> network_state_handler_;
  std::unique_ptr<NetworkConfigurationHandler> network_configuration_handler_;
  std::unique_ptr<TestNetworkStateHandlerObserver>
      network_state_handler_observer_;
  base::MessageLoopForUI message_loop_;
  std::string success_callback_name_;
  std::string get_properties_path_;
  std::unique_ptr<base::DictionaryValue> get_properties_;
  std::string create_service_path_;
};

TEST_F(NetworkConfigurationHandlerTest, GetProperties) {
  constexpr char kServicePath[] = "/service/1";
  constexpr char kNetworkName[] = "MyName";
  GetShillServiceClient()->AddService(
      kServicePath, std::string() /* guid */, kNetworkName, shill::kTypeWifi,
      std::string() /* state */, true /* visible */);

  bool success = false;
  std::string service_path;
  base::DictionaryValue result;
  network_configuration_handler_->GetShillProperties(
      kServicePath,
      base::Bind(&CopyProperties, &success, &service_path, &result),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(success);
  EXPECT_EQ(kServicePath, service_path);
  const base::Value* ssid =
      result.FindKeyOfType(shill::kSSIDProperty, base::Value::Type::STRING);
  ASSERT_TRUE(ssid);
  EXPECT_EQ(kNetworkName, ssid->GetString());
}

TEST_F(NetworkConfigurationHandlerTest, GetProperties_TetherNetwork) {
  constexpr char kTetherGuid[] = "TetherGuid";
  constexpr char kTetherNetworkName[] = "TetherNetworkName";
  constexpr char kTetherNetworkCarrier[] = "TetherNetworkCarrier";
  constexpr int kBatteryPercentage = 100;
  constexpr int kSignalStrength = 100;
  constexpr bool kHasConnectedToHost = true;

  network_state_handler_->SetTetherTechnologyState(
      NetworkStateHandler::TechnologyState::TECHNOLOGY_ENABLED);
  network_state_handler_->AddTetherNetworkState(
      kTetherGuid, kTetherNetworkName, kTetherNetworkCarrier,
      kBatteryPercentage, kSignalStrength, kHasConnectedToHost);

  bool success = false;
  std::string service_path;
  base::DictionaryValue result;
  network_configuration_handler_->GetShillProperties(
      // Tether networks use service path and GUID interchangeably.
      kTetherGuid,
      base::Bind(&CopyProperties, &success, &service_path, &result),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(success);
  const base::Value* guid =
      result.FindKeyOfType(shill::kGuidProperty, base::Value::Type::STRING);
  ASSERT_TRUE(guid);
  EXPECT_EQ(kTetherGuid, guid->GetString());
  const base::Value* name =
      result.FindKeyOfType(shill::kNameProperty, base::Value::Type::STRING);
  ASSERT_TRUE(name);
  EXPECT_EQ(kTetherNetworkName, name->GetString());
  const base::Value* battery_percentage = result.FindKeyOfType(
      kTetherBatteryPercentage, base::Value::Type::INTEGER);
  ASSERT_TRUE(battery_percentage);
  EXPECT_EQ(kBatteryPercentage, battery_percentage->GetInt());
  const base::Value* carrier =
      result.FindKeyOfType(kTetherCarrier, base::Value::Type::STRING);
  ASSERT_TRUE(carrier);
  EXPECT_EQ(kTetherNetworkCarrier, carrier->GetString());
  const base::Value* has_connected_to_host = result.FindKeyOfType(
      kTetherHasConnectedToHost, base::Value::Type::BOOLEAN);
  ASSERT_TRUE(has_connected_to_host);
  EXPECT_TRUE(has_connected_to_host->GetBool());
  const base::Value* signal_strength =
      result.FindKeyOfType(kTetherSignalStrength, base::Value::Type::INTEGER);
  ASSERT_TRUE(signal_strength);
  EXPECT_EQ(kSignalStrength, signal_strength->GetInt());
}

TEST_F(NetworkConfigurationHandlerTest, SetProperties) {
  constexpr char kServicePath[] = "/service/1";
  constexpr char kNetworkName[] = "MyNetwork";

  GetShillServiceClient()->AddService(
      kServicePath, std::string() /* guid */, std::string() /* name */,
      shill::kTypeWifi, std::string() /* state */, true /* visible */);

  base::DictionaryValue value;
  value.SetString(shill::kSSIDProperty, kNetworkName);
  network_configuration_handler_->SetShillProperties(
      kServicePath, value, NetworkConfigurationObserver::SOURCE_USER_ACTION,
      base::DoNothing(), base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  const base::DictionaryValue* properties =
      GetShillServiceClient()->GetServiceProperties(kServicePath);
  ASSERT_TRUE(properties);
  const base::Value* ssid = properties->FindKeyOfType(
      shill::kSSIDProperty, base::Value::Type::STRING);
  ASSERT_TRUE(ssid);
  EXPECT_EQ(kNetworkName, ssid->GetString());
}

TEST_F(NetworkConfigurationHandlerTest, ClearProperties) {
  constexpr char kServicePath[] = "/service/1";
  constexpr char kNetworkName[] = "MyNetwork";

  // Set up a value to be cleared.
  GetShillServiceClient()->AddService(
      kServicePath, std::string() /* guid */, kNetworkName, shill::kTypeWifi,
      std::string() /* state */, true /* visible */);

  // Now clear it.
  std::vector<std::string> names = {shill::kSSIDProperty};
  network_configuration_handler_->ClearShillProperties(
      kServicePath, names, base::DoNothing(), base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  const base::DictionaryValue* properties =
      GetShillServiceClient()->GetServiceProperties(kServicePath);
  ASSERT_TRUE(properties);
  const base::Value* ssid = properties->FindKeyOfType(
      shill::kSSIDProperty, base::Value::Type::STRING);
  EXPECT_FALSE(ssid);
}

TEST_F(NetworkConfigurationHandlerTest, ClearProperties_Error) {
  constexpr char kServicePath[] = "/service/1";
  constexpr char kNetworkName[] = "MyNetwork";

  GetShillServiceClient()->AddService(
      kServicePath, std::string() /* guid */, kNetworkName, shill::kTypeWifi,
      std::string() /* state */, true /* visible */);

  // Now clear it. Even for unknown property removal (i.e. fail to clear it),
  // the whole ClearShillProperties() should succeed.
  std::vector<std::string> names = {"Unknown name"};
  network_configuration_handler_->ClearShillProperties(
      kServicePath, names, base::DoNothing(), base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkConfigurationHandlerTest, CreateConfiguration) {
  constexpr char kGuid[] = "/service/2";
  constexpr char kNetworkName[] = "MyNetwork";

  base::DictionaryValue value;
  shill_property_util::SetSSID(kNetworkName, &value);
  value.SetString(shill::kTypeProperty, "wifi");
  value.SetString(shill::kProfileProperty, "profile path");
  value.SetString(shill::kGuidProperty, kGuid);

  bool success = false;
  std::string service_path;
  std::string guid;
  network_configuration_handler_->CreateShillConfiguration(
      value, NetworkConfigurationObserver::SOURCE_USER_ACTION,
      base::Bind(&CopyServiceResult, &success, &service_path, &guid),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(success);
  // In FakeShillManagerClient, instead of re-implementing shill's behavior,
  // guid is used for service_path.
  EXPECT_EQ(service_path, kGuid);
  EXPECT_EQ(guid, kGuid);
}

TEST_F(NetworkConfigurationHandlerTest, RemoveConfiguration) {
  ASSERT_NO_FATAL_FAILURE(SetUpRemovableConfiguration());

  TestCallback test_callback;
  network_configuration_handler_->RemoveConfiguration(
      "/service/2", NetworkConfigurationObserver::SOURCE_USER_ACTION,
      base::Bind(&TestCallback::Run, base::Unretained(&test_callback)),
      base::Bind(&ErrorCallback));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, test_callback.run_count());

  std::vector<std::string> profiles;
  // "/service/1" should not be affected.
  GetShillProfileClient()->GetProfilePathsContainingService("/service/1",
                                                            &profiles);
  EXPECT_EQ(2u, profiles.size());
  profiles.clear();
  // "/service/2" should be removed from both profiles.
  GetShillProfileClient()->GetProfilePathsContainingService("/service/2",
                                                            &profiles);
  EXPECT_TRUE(profiles.empty());
}

TEST_F(NetworkConfigurationHandlerTest, RemoveConfigurationFromCurrentProfile) {
  ASSERT_NO_FATAL_FAILURE(SetUpRemovableConfiguration());

  TestCallback test_callback;
  network_configuration_handler_->RemoveConfigurationFromCurrentProfile(
      "/service/2", NetworkConfigurationObserver::SOURCE_USER_ACTION,
      base::Bind(&TestCallback::Run, base::Unretained(&test_callback)),
      base::Bind(&ErrorCallback));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, test_callback.run_count());

  std::vector<std::string> profiles;
  // "/service/1" should not be affected.
  GetShillProfileClient()->GetProfilePathsContainingService("/service/1",
                                                            &profiles);
  EXPECT_EQ(2u, profiles.size());
  profiles.clear();
  // "/service/2" should be removed only from the current profile.
  GetShillProfileClient()->GetProfilePathsContainingService("/service/2",
                                                            &profiles);
  EXPECT_EQ(1u, profiles.size());
}

TEST_F(NetworkConfigurationHandlerTest,
       RemoveNonExistentConfigurationFromCurrentProfile) {
  ASSERT_NO_FATAL_FAILURE(SetUpRemovableConfiguration());

  TestCallback test_callback;
  std::string error;
  network_configuration_handler_->RemoveConfigurationFromCurrentProfile(
      "/service/3", NetworkConfigurationObserver::SOURCE_USER_ACTION,
      base::Bind(&TestCallback::Run, base::Unretained(&test_callback)),
      base::Bind(&RecordError, base::Unretained(&error)));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, test_callback.run_count());
  // Should report an error for unknown configuration.
  EXPECT_EQ("NetworkNotConfigured", error);
}

TEST_F(NetworkConfigurationHandlerTest, StubSetAndClearProperties) {
  // TODO(stevenjb): Remove dependency on default Stub service.
  const std::string service_path("/service/wifi1");
  const std::string test_identity("test_identity");
  const std::string test_passphrase("test_passphrase");

  // Set Properties
  base::DictionaryValue properties_to_set;
  properties_to_set.SetKey(shill::kIdentityProperty,
                           base::Value(test_identity));
  properties_to_set.SetKey(shill::kPassphraseProperty,
                           base::Value(test_passphrase));
  network_configuration_handler_->SetShillProperties(
      service_path, properties_to_set,
      NetworkConfigurationObserver::SOURCE_USER_ACTION,
      base::Bind(&NetworkConfigurationHandlerTest::SuccessCallback,
                 base::Unretained(this), "SetProperties"),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ("SetProperties", success_callback_name_);
  std::string identity, passphrase;
  EXPECT_TRUE(GetServiceStringProperty(service_path, shill::kIdentityProperty,
                                       &identity));
  EXPECT_TRUE(GetServiceStringProperty(service_path, shill::kPassphraseProperty,
                                       &passphrase));
  EXPECT_EQ(test_identity, identity);
  EXPECT_EQ(test_passphrase, passphrase);
  EXPECT_EQ(1, network_state_handler_observer_->PropertyUpdatesForService(
                   service_path));

  // Clear Properties
  std::vector<std::string> properties_to_clear;
  properties_to_clear.push_back(shill::kIdentityProperty);
  properties_to_clear.push_back(shill::kPassphraseProperty);
  network_configuration_handler_->ClearShillProperties(
      service_path, properties_to_clear,
      base::Bind(&NetworkConfigurationHandlerTest::SuccessCallback,
                 base::Unretained(this), "ClearProperties"),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ("ClearProperties", success_callback_name_);
  EXPECT_FALSE(GetServiceStringProperty(service_path, shill::kIdentityProperty,
                                        &identity));
  EXPECT_FALSE(GetServiceStringProperty(service_path, shill::kIdentityProperty,
                                        &passphrase));
  EXPECT_EQ(2, network_state_handler_observer_->PropertyUpdatesForService(
                   service_path));
}

TEST_F(NetworkConfigurationHandlerTest, StubGetNameFromWifiHex) {
  // TODO(stevenjb): Remove dependency on default Stub service.
  const std::string service_path("/service/wifi1");
  std::string wifi_hex = "5468697320697320484558205353494421";
  std::string expected_name = "This is HEX SSID!";

  // Set Properties
  base::DictionaryValue properties_to_set;
  properties_to_set.SetKey(shill::kWifiHexSsid, base::Value(wifi_hex));
  network_configuration_handler_->SetShillProperties(
      service_path, properties_to_set,
      NetworkConfigurationObserver::SOURCE_USER_ACTION, base::DoNothing(),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();
  std::string wifi_hex_result;
  EXPECT_TRUE(GetServiceStringProperty(service_path, shill::kWifiHexSsid,
                                       &wifi_hex_result));
  EXPECT_EQ(wifi_hex, wifi_hex_result);

  // Get Properties
  network_configuration_handler_->GetShillProperties(
      service_path,
      base::Bind(&NetworkConfigurationHandlerTest::GetPropertiesCallback,
                 base::Unretained(this)),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(service_path, get_properties_path_);
  std::string name_result;
  EXPECT_TRUE(GetReceivedStringProperty(service_path, shill::kNameProperty,
                                        &name_result));
  EXPECT_EQ(expected_name, name_result);
}

TEST_F(NetworkConfigurationHandlerTest, StubCreateConfiguration) {
  const std::string service_path("/service/test_wifi");
  CreateTestConfiguration(service_path, shill::kTypeWifi);

  EXPECT_FALSE(create_service_path_.empty());

  std::string guid;
  EXPECT_TRUE(GetServiceStringProperty(create_service_path_,
                                       shill::kGuidProperty, &guid));
  EXPECT_EQ(service_path, guid);

  std::string actual_profile;
  EXPECT_TRUE(GetServiceStringProperty(
      create_service_path_, shill::kProfileProperty, &actual_profile));
  EXPECT_EQ(NetworkProfileHandler::GetSharedProfilePath(), actual_profile);
}

TEST_F(NetworkConfigurationHandlerTest, NetworkConfigurationObserver) {
  const std::string service_path("/service/test_wifi");
  const std::string test_passphrase("test_passphrase");

  auto network_configuration_observer =
      std::make_unique<TestNetworkConfigurationObserver>();
  network_configuration_handler_->AddObserver(
      network_configuration_observer.get());
  CreateTestConfiguration(service_path, shill::kTypeWifi);

  EXPECT_TRUE(network_configuration_observer->HasConfiguration(service_path));
  EXPECT_TRUE(network_configuration_observer->HasConfigurationInProfile(
      service_path, NetworkProfileHandler::GetSharedProfilePath()));
  EXPECT_EQ(shill::kTypeWifi, network_configuration_observer->GetStringProperty(
                                  service_path, shill::kTypeProperty));

  base::DictionaryValue properties_to_set;
  properties_to_set.SetKey(shill::kPassphraseProperty,
                           base::Value(test_passphrase));
  network_configuration_handler_->SetShillProperties(
      service_path, properties_to_set,
      NetworkConfigurationObserver::SOURCE_USER_ACTION, base::DoNothing(),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(test_passphrase, network_configuration_observer->GetStringProperty(
                                 service_path, shill::kPassphraseProperty));

  std::string user_profile = "/profiles/user1";
  std::string userhash = "user1";
  DBusThreadManager::Get()
      ->GetShillProfileClient()
      ->GetTestInterface()
      ->AddProfile(user_profile, userhash);

  network_configuration_handler_->SetNetworkProfile(
      service_path, user_profile,
      NetworkConfigurationObserver::SOURCE_USER_ACTION, base::DoNothing(),
      base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(network_configuration_observer->HasConfiguration(service_path));
  EXPECT_FALSE(network_configuration_observer->HasConfigurationInProfile(
      service_path, NetworkProfileHandler::GetSharedProfilePath()));
  EXPECT_TRUE(network_configuration_observer->HasConfigurationInProfile(
      service_path, user_profile));

  network_configuration_handler_->RemoveConfiguration(
      service_path, NetworkConfigurationObserver::SOURCE_USER_ACTION,
      base::DoNothing(), base::Bind(&ErrorCallback));
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(network_configuration_observer->HasConfiguration(service_path));
  EXPECT_FALSE(network_configuration_observer->HasConfigurationInProfile(
      service_path, NetworkProfileHandler::GetSharedProfilePath()));
  EXPECT_FALSE(network_configuration_observer->HasConfigurationInProfile(
      service_path, user_profile));

  network_configuration_handler_->RemoveObserver(
      network_configuration_observer.get());
}

}  // namespace chromeos
