// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "chrome/browser/chromeos/arc/intent_helper/arc_settings_service.h"
#include "chrome/browser/chromeos/policy/configuration_policy_handler_chromeos.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_profile_client.h"
#include "chromeos/dbus/shill_service_client.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/proxy/proxy_config_handler.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/arc_util.h"
#include "components/arc/test/connection_holder_util.h"
#include "components/arc/test/fake_backup_settings_instance.h"
#include "components/arc/test/fake_intent_helper_instance.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "components/proxy_config/proxy_prefs.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using testing::_;
using testing::Return;

namespace arc {

namespace {
constexpr char kONCPolicy[] =
    "{ \"NetworkConfigurations\": ["
    "    { \"GUID\": \"stub_ethernet_guid\","
    "      \"Type\": \"Ethernet\","
    "      \"Name\": \"My Ethernet\","
    "      \"Ethernet\": {"
    "        \"Authentication\": \"None\" },"
    "      \"ProxySettings\": {"
    "        \"PAC\": \"http://domain.com/x\","
    "        \"Type\": \"PAC\" }"
    "    }"
    "  ],"
    "  \"Type\": \"UnencryptedConfiguration\""
    "}";

constexpr char kDeviceONCPolicy[] =
    "{"
    "   \"GlobalNetworkConfiguration\": {"
    "      \"AllowOnlyPolicyNetworksToAutoconnect\": false,"
    "      \"AllowOnlyPolicyNetworksToConnect\": false"
    "   },"
    "   \"NetworkConfigurations\": [ {"
    "      \"GUID\": \"{wifi1_guid}\","
    "      \"Name\": \"wifi1\","
    "      \"ProxySettings\": {"
    "         \"Manual\": {"
    "            \"FTPProxy\": {"
    "               \"Host\": \"proxy\","
    "               \"Port\": 5000"
    "            },"
    "            \"HTTPProxy\": {"
    "               \"Host\": \"proxy\","
    "               \"Port\": 5000"
    "            },"
    "            \"SOCKS\": {"
    "               \"Host\": \"proxy\","
    "               \"Port\": 5000"
    "            },"
    "            \"SecureHTTPProxy\": {"
    "               \"Host\": \"proxy\","
    "               \"Port\": 5000"
    "            }"
    "         },"
    "         \"Type\": \"Manual\""
    "      },"
    "      \"Type\": \"WiFi\","
    "      \"WiFi\": {"
    "         \"AutoConnect\": false,"
    "         \"HiddenSSID\": false,"
    "         \"SSID\": \"wifi1\","
    "         \"Security\": \"None\""
    "      }"
    "   } ]"
    "}";

constexpr char kUserONCPolicy[] =
    "{"
    "   \"NetworkConfigurations\": [ {"
    "      \"GUID\": \"{direct_guid}\","
    "      \"Name\": \"EAP-TTLS\","
    "      \"ProxySettings\": {"
    "         \"Type\": \"Direct\""
    "      },"
    "      \"Type\": \"WiFi\","
    "      \"WiFi\": {"
    "         \"AutoConnect\": false,"
    "         \"EAP\": {"
    "            \"Identity\": \"CrOS\","
    "            \"Inner\": \"Automatic\","
    "            \"Outer\": \"EAP-TTLS\","
    "            \"Password\": \"********\","
    "            \"Recommended\": ["
    "              \"AnonymousIdentity\","
    "              \"Identity\","
    "              \"Password\""
    "            ],"
    "            \"SaveCredentials\": true,"
    "            \"UseSystemCAs\": false"
    "         },"
    "         \"HiddenSSID\": false,"
    "        \"SSID\": \"direct_ssid\","
    "        \"Security\": \"WPA-EAP\""
    "     }"
    "  }, {"
    "      \"GUID\": \"{wifi0_guid}\","
    "      \"Name\": \"wifi0\","
    "      \"ProxySettings\": {"
    "         \"Manual\": {"
    "            \"FTPProxy\": {"
    "               \"Host\": \"proxy-n300\","
    "               \"Port\": 3000"
    "            },"
    "            \"HTTPProxy\": {"
    "               \"Host\": \"proxy-n300\","
    "               \"Port\": 3000"
    "            },"
    "            \"SOCKS\": {"
    "               \"Host\": \"proxy-n300\","
    "               \"Port\": 3000"
    "            },"
    "            \"SecureHTTPProxy\": {"
    "               \"Host\": \"proxy-n300\","
    "               \"Port\": 3000"
    "            }"
    "         },"
    "         \"Type\": \"Manual\""
    "      },"
    "      \"Type\": \"WiFi\","
    "      \"WiFi\": {"
    "         \"AutoConnect\": false,"
    "         \"HiddenSSID\": false,"
    "         \"SSID\": \"wifi0\","
    "         \"Security\": \"None\""
    "      }"
    "   } ]"
    "}";

constexpr char kUserProfilePath[] = "user_profile";
constexpr char kDefaultServicePath[] = "stub_ethernet";

constexpr char kWifi0ServicePath[] = "stub_wifi0";
constexpr char kWifi0Ssid[] = "wifi0";
constexpr char kWifi0Guid[] = "{wifi0_guid}";

constexpr char kWifi1ServicePath[] = "stub_wifi1";
constexpr char kWifi1Ssid[] = "wifi1";
constexpr char kWifi1Guid[] = "{wifi1_guid}";

constexpr char kONCPacUrl[] = "http://domain.com/x";

constexpr char kSetProxyBroadcastAction[] =
    "org.chromium.arc.intent_helper.SET_PROXY";

// Returns the number of |broadcasts| having the proxy action, and checks that
// all their extras match with |extras|.
int CountProxyBroadcasts(
    const std::vector<FakeIntentHelperInstance::Broadcast>& broadcasts,
    const base::DictionaryValue* extras) {
  int count = 0;
  for (const FakeIntentHelperInstance::Broadcast& broadcast : broadcasts) {
    if (broadcast.action == kSetProxyBroadcastAction) {
      EXPECT_TRUE(base::JSONReader::Read(broadcast.extras)->Equals(extras));
      count++;
    }
  }
  return count;
}

void RunUntilIdle() {
  DCHECK(base::MessageLoopCurrent::Get());
  base::RunLoop loop;
  loop.RunUntilIdle();
}

}  // namespace

class ArcSettingsServiceTest : public InProcessBrowserTest {
 public:
  ArcSettingsServiceTest() = default;

  // InProcessBrowserTest:
  ~ArcSettingsServiceTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    arc::SetArcAvailableCommandLineForTesting(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

  void SetUpOnMainThread() override {
    SetupNetworkEnvironment();
    RunUntilIdle();

    fake_intent_helper_instance_ = std::make_unique<FakeIntentHelperInstance>();
    ArcServiceManager::Get()
        ->arc_bridge_service()
        ->intent_helper()
        ->SetInstance(fake_intent_helper_instance_.get());
    WaitForInstanceReady(
        ArcServiceManager::Get()->arc_bridge_service()->intent_helper());

    fake_backup_settings_instance_ =
        std::make_unique<FakeBackupSettingsInstance>();
    ArcServiceManager::Get()
        ->arc_bridge_service()
        ->backup_settings()
        ->SetInstance(fake_backup_settings_instance_.get());
    WaitForInstanceReady(
        ArcServiceManager::Get()->arc_bridge_service()->backup_settings());
  }

  void TearDownOnMainThread() override {
    ArcServiceManager::Get()
        ->arc_bridge_service()
        ->backup_settings()
        ->CloseInstance(fake_backup_settings_instance_.get());
    fake_backup_settings_instance_.reset();

    ArcServiceManager::Get()
        ->arc_bridge_service()
        ->intent_helper()
        ->CloseInstance(fake_intent_helper_instance_.get());
    fake_intent_helper_instance_.reset();
  }

  void UpdatePolicy(const policy::PolicyMap& policy) {
    provider_.UpdateChromePolicy(policy);
    RunUntilIdle();
  }

 protected:
  void DisconnectNetworkService(const std::string& service_path) {
    chromeos::ShillServiceClient::TestInterface* service_test =
        chromeos::DBusThreadManager::Get()
            ->GetShillServiceClient()
            ->GetTestInterface();
    base::Value value(shill::kStateIdle);
    service_test->SetServiceProperty(service_path, shill::kStateProperty,
                                     value);
    RunUntilIdle();
  }

  void ConnectWifiNetworkService(const std::string& service_path,
                                 const std::string& guid,
                                 const std::string& ssid) {
    chromeos::ShillServiceClient::TestInterface* service_test =
        chromeos::DBusThreadManager::Get()
            ->GetShillServiceClient()
            ->GetTestInterface();

    service_test->AddService(service_path, guid, ssid, shill::kTypeWifi,
                             shill::kStateOnline, true /* add_to_visible */);

    service_test->SetServiceProperty(service_path, shill::kProfileProperty,
                                     base::Value(kUserProfilePath));
    RunUntilIdle();
  }

  void SetProxyConfigForNetworkService(
      const std::string& service_path,
      const base::DictionaryValue* proxy_config) {
    ProxyConfigDictionary proxy_config_dict(proxy_config->CreateDeepCopy());
    const chromeos::NetworkState* network = chromeos::NetworkHandler::Get()
                                                ->network_state_handler()
                                                ->GetNetworkState(service_path);
    ASSERT_TRUE(network);
    chromeos::proxy_config::SetProxyConfigForNetwork(proxy_config_dict,
                                                     *network);
  }

  std::unique_ptr<FakeIntentHelperInstance> fake_intent_helper_instance_;
  std::unique_ptr<FakeBackupSettingsInstance> fake_backup_settings_instance_;

 private:
  void SetupNetworkEnvironment() {
    chromeos::ShillProfileClient::TestInterface* profile_test =
        chromeos::DBusThreadManager::Get()
            ->GetShillProfileClient()
            ->GetTestInterface();
    chromeos::ShillServiceClient::TestInterface* service_test =
        chromeos::DBusThreadManager::Get()
            ->GetShillServiceClient()
            ->GetTestInterface();

    profile_test->AddProfile(kUserProfilePath, "user");

    service_test->ClearServices();

    service_test->AddService(kDefaultServicePath, "stub_ethernet_guid", "eth0",
                             shill::kTypeEthernet, shill::kStateOnline,
                             true /* add_to_visible */);
    service_test->SetServiceProperty(kDefaultServicePath,
                                     shill::kProfileProperty,
                                     base::Value(kUserProfilePath));
  }

  policy::MockConfigurationPolicyProvider provider_;

  DISALLOW_COPY_AND_ASSIGN(ArcSettingsServiceTest);
};

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, BackupRestorePolicyTest) {
  // The policy is initially set to user control.
  policy::PolicyMap policy;
  policy.Set(policy::key::kArcBackupRestoreServiceEnabled,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(static_cast<int>(
                 policy::ArcServicePolicyValue::kUnderUserControl)),
             nullptr);
  UpdatePolicy(policy);

  PrefService* const prefs = browser()->profile()->GetPrefs();

  // Set the user pref as initially enabled.
  prefs->SetBoolean(prefs::kArcBackupRestoreEnabled, true);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kArcBackupRestoreEnabled));

  fake_backup_settings_instance_->ClearCallHistory();

  // The policy is set to disabled.
  policy.Set(policy::key::kArcBackupRestoreServiceEnabled,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(
                 static_cast<int>(policy::ArcServicePolicyValue::kDisabled)),
             nullptr);
  UpdatePolicy(policy);

  // The pref is disabled and managed, but the corresponding sync method does
  // not reflect the pref as it is not dynamically applied.
  EXPECT_FALSE(prefs->GetBoolean(prefs::kArcBackupRestoreEnabled));
  EXPECT_TRUE(prefs->IsManagedPreference(prefs::kArcBackupRestoreEnabled));
  EXPECT_EQ(0, fake_backup_settings_instance_->set_backup_enabled_count());
  EXPECT_FALSE(fake_backup_settings_instance_->enabled());
  EXPECT_FALSE(fake_backup_settings_instance_->managed());

  fake_backup_settings_instance_->ClearCallHistory();

  // The policy is set to user control.
  policy.Set(policy::key::kArcBackupRestoreServiceEnabled,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(static_cast<int>(
                 policy::ArcServicePolicyValue::kUnderUserControl)),
             nullptr);
  UpdatePolicy(policy);

  // The pref is unmanaged, but the corresponding sync method does not reflect
  // the pref as it is not dynamically applied.
  EXPECT_TRUE(prefs->GetBoolean(prefs::kArcBackupRestoreEnabled));
  EXPECT_FALSE(prefs->IsManagedPreference(prefs::kArcBackupRestoreEnabled));
  EXPECT_EQ(0, fake_backup_settings_instance_->set_backup_enabled_count());
  EXPECT_FALSE(fake_backup_settings_instance_->enabled());
  EXPECT_FALSE(fake_backup_settings_instance_->managed());
}

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, LocationServicePolicyTest) {
  // The policy is initially set to user control.
  policy::PolicyMap policy;
  policy.Set(policy::key::kArcGoogleLocationServicesEnabled,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(static_cast<int>(
                 policy::ArcServicePolicyValue::kUnderUserControl)),
             nullptr);
  UpdatePolicy(policy);

  PrefService* const prefs = browser()->profile()->GetPrefs();

  // Set the user pref as initially enabled.
  prefs->SetBoolean(prefs::kArcLocationServiceEnabled, true);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kArcLocationServiceEnabled));

  fake_intent_helper_instance_->clear_broadcasts();

  // The policy is set to disabled.
  policy.Set(policy::key::kArcGoogleLocationServicesEnabled,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(
                 static_cast<int>(policy::ArcServicePolicyValue::kDisabled)),
             nullptr);
  UpdatePolicy(policy);

  // The pref is disabled and managed, but no broadcast is sent as the setting
  // is not dynamically applied.
  EXPECT_FALSE(prefs->GetBoolean(prefs::kArcLocationServiceEnabled));
  EXPECT_TRUE(prefs->IsManagedPreference(prefs::kArcLocationServiceEnabled));
  EXPECT_EQ(0UL, fake_intent_helper_instance_->broadcasts().size());

  fake_intent_helper_instance_->clear_broadcasts();

  // The policy is set to user control.
  policy.Set(policy::key::kArcGoogleLocationServicesEnabled,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(static_cast<int>(
                 policy::ArcServicePolicyValue::kUnderUserControl)),
             nullptr);
  UpdatePolicy(policy);

  // The pref is unmanaged, but no broadcast is sent as the setting is not
  // dynamically applied.
  EXPECT_FALSE(prefs->IsManagedPreference(prefs::kArcLocationServiceEnabled));
  EXPECT_EQ(0UL, fake_intent_helper_instance_->broadcasts().size());
}

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, ProxyModePolicyTest) {
  fake_intent_helper_instance_->clear_broadcasts();

  policy::PolicyMap policy;
  policy.Set(
      policy::key::kProxyMode, policy::POLICY_LEVEL_MANDATORY,
      policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
      std::make_unique<base::Value>(ProxyPrefs::kAutoDetectProxyModeName),
      nullptr);
  UpdatePolicy(policy);

  std::unique_ptr<base::DictionaryValue> expected_proxy_config(
      std::make_unique<base::DictionaryValue>());
  expected_proxy_config->SetString("mode",
                                   ProxyPrefs::kAutoDetectProxyModeName);
  expected_proxy_config->SetString("pacUrl", "http://wpad/wpad.dat");
  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_proxy_config.get()),
            1);
}

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, ONCProxyPolicyTest) {
  fake_intent_helper_instance_->clear_broadcasts();

  policy::PolicyMap policy;
  policy.Set(policy::key::kOpenNetworkConfiguration,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(kONCPolicy), nullptr);
  UpdatePolicy(policy);

  std::unique_ptr<base::DictionaryValue> expected_proxy_config(
      std::make_unique<base::DictionaryValue>());
  expected_proxy_config->SetString("mode", ProxyPrefs::kPacScriptProxyModeName);
  expected_proxy_config->SetString("pacUrl", kONCPacUrl);

  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_proxy_config.get()),
            1);
}

// Proxy policy has a higher priority than proxy default settings.
IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, TwoSourcesTest) {
  fake_intent_helper_instance_->clear_broadcasts();

  policy::PolicyMap policy;
  // Proxy policy.
  policy.Set(
      policy::key::kProxyMode, policy::POLICY_LEVEL_MANDATORY,
      policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
      std::make_unique<base::Value>(ProxyPrefs::kFixedServersProxyModeName),
      nullptr);
  policy.Set(policy::key::kProxyServer, policy::POLICY_LEVEL_MANDATORY,
             policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>("proxy:8888"), nullptr);
  UpdatePolicy(policy);

  std::unique_ptr<base::DictionaryValue> proxy_config(
      std::make_unique<base::DictionaryValue>());
  proxy_config->SetString("mode", ProxyPrefs::kAutoDetectProxyModeName);
  ProxyConfigDictionary proxy_config_dict(std::move(proxy_config));
  const chromeos::NetworkState* network = chromeos::NetworkHandler::Get()
                                              ->network_state_handler()
                                              ->DefaultNetwork();
  ASSERT_TRUE(network);
  chromeos::proxy_config::SetProxyConfigForNetwork(proxy_config_dict, *network);
  RunUntilIdle();

  std::unique_ptr<base::DictionaryValue> expected_proxy_config(
      std::make_unique<base::DictionaryValue>());
  expected_proxy_config->SetString("mode",
                                   ProxyPrefs::kFixedServersProxyModeName);
  expected_proxy_config->SetString("host", "proxy");
  expected_proxy_config->SetInteger("port", 8888);
  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_proxy_config.get()),
            1);
}

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, ProxyPrefTest) {
  fake_intent_helper_instance_->clear_broadcasts();

  std::unique_ptr<base::DictionaryValue> proxy_config(
      std::make_unique<base::DictionaryValue>());
  proxy_config->SetString("mode", ProxyPrefs::kPacScriptProxyModeName);
  proxy_config->SetString("pac_url", "http://proxy");
  browser()->profile()->GetPrefs()->Set(proxy_config::prefs::kProxy,
                                        *proxy_config.get());
  RunUntilIdle();

  std::unique_ptr<base::DictionaryValue> expected_proxy_config(
      std::make_unique<base::DictionaryValue>());
  expected_proxy_config->SetString("mode", ProxyPrefs::kPacScriptProxyModeName);
  expected_proxy_config->SetString("pacUrl", "http://proxy");
  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_proxy_config.get()),
            1);
}

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, DefaultNetworkProxyConfigTest) {
  fake_intent_helper_instance_->clear_broadcasts();

  std::unique_ptr<base::DictionaryValue> proxy_config(
      std::make_unique<base::DictionaryValue>());
  proxy_config->SetString("mode", ProxyPrefs::kFixedServersProxyModeName);
  proxy_config->SetString("server", "proxy:8080");
  SetProxyConfigForNetworkService(kDefaultServicePath, proxy_config.get());
  RunUntilIdle();

  std::unique_ptr<base::DictionaryValue> expected_proxy_config(
      std::make_unique<base::DictionaryValue>());
  expected_proxy_config->SetString("mode",
                                   ProxyPrefs::kFixedServersProxyModeName);
  expected_proxy_config->SetString("host", "proxy");
  expected_proxy_config->SetInteger("port", 8080);
  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_proxy_config.get()),
            1);
}

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, DefaultNetworkDisconnectedTest) {
  ConnectWifiNetworkService(kWifi0ServicePath, kWifi0Guid, kWifi0Ssid);
  fake_intent_helper_instance_->clear_broadcasts();
  // Set proxy confog for default network.
  std::unique_ptr<base::DictionaryValue> default_proxy_config(
      std::make_unique<base::DictionaryValue>());
  default_proxy_config->SetString("mode",
                                  ProxyPrefs::kFixedServersProxyModeName);
  default_proxy_config->SetString("server", "default/proxy:8080");
  SetProxyConfigForNetworkService(kDefaultServicePath,
                                  default_proxy_config.get());
  RunUntilIdle();

  // Set proxy confog for WI-FI network.
  std::unique_ptr<base::DictionaryValue> wifi_proxy_config(
      std::make_unique<base::DictionaryValue>());
  wifi_proxy_config->SetString("mode", ProxyPrefs::kFixedServersProxyModeName);
  wifi_proxy_config->SetString("server", "wifi/proxy:8080");
  SetProxyConfigForNetworkService(kWifi0ServicePath, wifi_proxy_config.get());
  RunUntilIdle();

  // Observe default network proxy config broadcast.
  std::unique_ptr<base::DictionaryValue> expected_default_proxy_config(
      std::make_unique<base::DictionaryValue>());
  expected_default_proxy_config->SetString(
      "mode", ProxyPrefs::kFixedServersProxyModeName);
  expected_default_proxy_config->SetString("host", "default/proxy");
  expected_default_proxy_config->SetInteger("port", 8080);
  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_default_proxy_config.get()),
            1);

  // Disconnect default network.
  fake_intent_helper_instance_->clear_broadcasts();
  DisconnectNetworkService(kDefaultServicePath);

  // Observe WI-FI network proxy config broadcast.
  std::unique_ptr<base::DictionaryValue> expected_wifi_proxy_config(
      std::make_unique<base::DictionaryValue>());
  expected_wifi_proxy_config->SetString("mode",
                                        ProxyPrefs::kFixedServersProxyModeName);
  expected_wifi_proxy_config->SetString("host", "wifi/proxy");
  expected_wifi_proxy_config->SetInteger("port", 8080);

  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_wifi_proxy_config.get()),
            1);
}

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, NoNetworkConnectedTest) {
  // Disconnect all networks.
  fake_intent_helper_instance_->clear_broadcasts();
  DisconnectNetworkService(kDefaultServicePath);

  EXPECT_EQ(
      CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(), nullptr),
      0);
}

IN_PROC_BROWSER_TEST_F(ArcSettingsServiceTest, TwoONCProxyPolicyTest) {
  // Connect to wifi1 with appliead device ONC policy.
  ConnectWifiNetworkService(kWifi1ServicePath, kWifi1Guid, kWifi1Ssid);

  // Disconnect default network.
  DisconnectNetworkService(kDefaultServicePath);

  fake_intent_helper_instance_->clear_broadcasts();

  policy::PolicyMap policy;
  policy.Set(policy::key::kOpenNetworkConfiguration,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(kUserONCPolicy), nullptr);
  policy.Set(policy::key::kDeviceOpenNetworkConfiguration,
             policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_MACHINE,
             policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(kDeviceONCPolicy), nullptr);
  UpdatePolicy(policy);

  std::unique_ptr<base::DictionaryValue> expected_proxy_config(
      std::make_unique<base::DictionaryValue>());
  expected_proxy_config->SetString("mode",
                                   ProxyPrefs::kFixedServersProxyModeName);
  expected_proxy_config->SetString("host", "proxy");
  expected_proxy_config->SetInteger("port", 5000);

  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_proxy_config.get()),
            1);

  DisconnectNetworkService(kWifi1ServicePath);
  fake_intent_helper_instance_->clear_broadcasts();

  // Connect to wifi0 with appliead user ONC policy.
  ConnectWifiNetworkService(kWifi0ServicePath, kWifi0Guid, kWifi0Ssid);

  expected_proxy_config->SetString("mode",
                                   ProxyPrefs::kFixedServersProxyModeName);
  expected_proxy_config->SetString("host", "proxy-n300");
  expected_proxy_config->SetInteger("port", 3000);

  EXPECT_EQ(CountProxyBroadcasts(fake_intent_helper_instance_->broadcasts(),
                                 expected_proxy_config.get()),
            1);
}

}  // namespace arc
