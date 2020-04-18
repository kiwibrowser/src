// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/wifi_credentials_helper_chromeos.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_profile_client.h"
#include "chromeos/network/managed_network_configuration_handler.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state_handler.h"
#include "components/onc/onc_constants.h"
#include "components/sync_wifi/network_state_helper_chromeos.h"
#include "components/sync_wifi/wifi_credential_syncable_service_factory.h"
#include "content/public/browser/browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"

using sync_wifi::WifiCredential;

using WifiCredentialSet = sync_wifi::WifiCredential::CredentialSet;

namespace wifi_credentials_helper {

namespace {

const char kProfilePrefix[] = "/profile/";

void LogCreateConfigurationFailure(
    const std::string& debug_hint,
    const std::string& /* network_config_error_message */,
    std::unique_ptr<base::DictionaryValue> /* network_config_error_data */) {
  LOG(FATAL) << debug_hint;
}

std::string ChromeOsUserHashForBrowserContext(
    const content::BrowserContext& context) {
  return context.GetPath().BaseName().value();
}

// Return value is distinct per |context|, but otherwise arbitrary.
std::string ShillProfilePathForBrowserContext(
    const content::BrowserContext& context) {
  return kProfilePrefix + ChromeOsUserHashForBrowserContext(context);
}

::chromeos::ShillProfileClient::TestInterface*
GetShillProfileClientTestInterface() {
  EXPECT_TRUE(::chromeos::DBusThreadManager::Get()->GetShillProfileClient());
  EXPECT_TRUE(::chromeos::DBusThreadManager::Get()
                  ->GetShillProfileClient()
                  ->GetTestInterface());
  return ::chromeos::DBusThreadManager::Get()->GetShillProfileClient()
      ->GetTestInterface();
}

::chromeos::ManagedNetworkConfigurationHandler*
GetManagedNetworkConfigurationHandler() {
  EXPECT_TRUE(::chromeos::NetworkHandler::Get()
                  ->managed_network_configuration_handler());
  return ::chromeos::NetworkHandler::Get()
      ->managed_network_configuration_handler();
}

::chromeos::NetworkStateHandler* GetNetworkStateHandler() {
  EXPECT_TRUE(::chromeos::NetworkHandler::Get()->network_state_handler());
  return ::chromeos::NetworkHandler::Get()->network_state_handler();
}

}  // namespace

namespace chromeos {

void SetUpChromeOs() {
  sync_wifi::WifiCredentialSyncableServiceFactory::GetInstance()
      ->set_ignore_login_state_for_test(true);
}

void SetupClientForProfileChromeOs(
    const content::BrowserContext* browser_context) {
  ASSERT_TRUE(browser_context);
  GetShillProfileClientTestInterface()
      ->AddProfile(ShillProfilePathForBrowserContext(*browser_context),
                   ChromeOsUserHashForBrowserContext(*browser_context));

  const base::ListValue policy_network_configs;
  const base::DictionaryValue policy_global_config;
  GetManagedNetworkConfigurationHandler()
      ->SetPolicy(onc::ONC_SOURCE_UNKNOWN,
                  ChromeOsUserHashForBrowserContext(*browser_context),
                  policy_network_configs,
                  policy_global_config);
}

void AddWifiCredentialToProfileChromeOs(
    const content::BrowserContext* browser_context,
    const WifiCredential& credential) {
  ASSERT_TRUE(browser_context);
  std::unique_ptr<base::DictionaryValue> onc_properties =
      credential.ToOncProperties();
  ASSERT_TRUE(onc_properties)
      << "Failed to generate ONC properties for " << credential.ToString();
  GetManagedNetworkConfigurationHandler()->CreateConfiguration(
      ChromeOsUserHashForBrowserContext(*browser_context), *onc_properties,
      ::chromeos::network_handler::ServiceResultCallback(),
      base::Bind(LogCreateConfigurationFailure,
                 base::StringPrintf("Failed to add credential %s",
                                    credential.ToString().c_str())));
  base::RunLoop().RunUntilIdle();
}

WifiCredentialSet GetWifiCredentialsForProfileChromeOs(
    const content::BrowserContext* browser_context) {
  EXPECT_TRUE(browser_context);
  return sync_wifi::GetWifiCredentialsForShillProfile(
      GetNetworkStateHandler(),
      ShillProfilePathForBrowserContext(*browser_context));
}

}  // namespace chromeos

}  // namespace wifi_credentials_helper
