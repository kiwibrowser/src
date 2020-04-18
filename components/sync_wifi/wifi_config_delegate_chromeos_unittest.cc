// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_config_delegate_chromeos.h"

#include <stddef.h>

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "base/values.h"
#include "chromeos/network/managed_network_configuration_handler.h"
#include "chromeos/network/network_handler_callbacks.h"
#include "components/sync_wifi/wifi_credential.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_wifi {

namespace {
const char kSsid[] = "fake-ssid";
const char kSsidNonUtf8[] = "\xc0";
const char kUserHash[] = "fake-user-hash";
}

using chromeos::network_handler::DictionaryResultCallback;
using chromeos::network_handler::ErrorCallback;
using chromeos::network_handler::ServiceResultCallback;

class FakeManagedNetworkConfigurationHandler
    : public chromeos::ManagedNetworkConfigurationHandler {
 public:
  FakeManagedNetworkConfigurationHandler()
      : create_configuration_called_(false) {}

  // ManagedNetworkConfigurationHandler implementation.
  void AddObserver(chromeos::NetworkPolicyObserver* observer) override {
    NOTIMPLEMENTED();
  }
  void RemoveObserver(chromeos::NetworkPolicyObserver* observer) override {
    NOTIMPLEMENTED();
  }
  void GetProperties(const std::string& userhash,
                     const std::string& service_path,
                     const DictionaryResultCallback& callback,
                     const ErrorCallback& error_callback) override {
    NOTIMPLEMENTED();
  }
  void GetManagedProperties(const std::string& userhash,
                            const std::string& service_path,
                            const DictionaryResultCallback& callback,
                            const ErrorCallback& error_callback) override {
    NOTIMPLEMENTED();
  }
  void SetProperties(const std::string& service_path,
                     const base::DictionaryValue& user_settings,
                     const base::Closure& callback,
                     const ErrorCallback& error_callback) override {
    NOTIMPLEMENTED();
  }
  void CreateConfiguration(const std::string& userhash,
                           const base::DictionaryValue& properties,
                           const ServiceResultCallback& callback,
                           const ErrorCallback& error_callback) const override {
    EXPECT_FALSE(create_configuration_called_);
    create_configuration_called_ = true;
    create_configuration_success_callback_ = callback;
    create_configuration_error_callback_ = error_callback;
  }
  void RemoveConfiguration(const std::string& service_path,
                           const base::Closure& callback,
                           const ErrorCallback& error_callback) const override {
    NOTIMPLEMENTED();
  }
  void RemoveConfigurationFromCurrentProfile(
      const std::string& service_path,
      const base::Closure& callback,
      const ErrorCallback& error_callback) const override {
    NOTIMPLEMENTED();
  }
  void SetPolicy(::onc::ONCSource onc_source,
                 const std::string& userhash,
                 const base::ListValue& network_configs_onc,
                 const base::DictionaryValue& global_network_config) override {
    NOTIMPLEMENTED();
  }
  bool IsAnyPolicyApplicationRunning() const override {
    NOTIMPLEMENTED();
    return false;
  }
  const base::DictionaryValue* FindPolicyByGUID(
      const std::string userhash,
      const std::string& guid,
      ::onc::ONCSource* onc_source) const override {
    NOTIMPLEMENTED();
    return nullptr;
  }
  const GuidToPolicyMap* GetNetworkConfigsFromPolicy(
      const std::string& userhash) const override {
    NOTIMPLEMENTED();
    return nullptr;
  }
  const base::DictionaryValue* GetGlobalConfigFromPolicy(
      const std::string& userhash) const override {
    NOTIMPLEMENTED();
    return nullptr;
  }
  const base::DictionaryValue* FindPolicyByGuidAndProfile(
      const std::string& guid,
      const std::string& profile_path,
      ::onc::ONCSource* onc_source) const override {
    NOTIMPLEMENTED();
    return nullptr;
  }

  bool create_configuration_called() const {
    return create_configuration_called_;
  }
  const ServiceResultCallback& create_configuration_success_callback() const {
    return create_configuration_success_callback_;
  }
  const ErrorCallback& create_configuration_error_callback() const {
    return create_configuration_error_callback_;
  }

 private:
  // Whether or not CreateConfiguration has been called on this fake.
  mutable bool create_configuration_called_;
  // The last |callback| passed to CreateConfiguration.
  mutable ServiceResultCallback create_configuration_success_callback_;
  // The last |error_callback| passed to CreateConfiguration.
  mutable ErrorCallback create_configuration_error_callback_;
};

class WifiConfigDelegateChromeOsTest : public testing::Test {
 protected:
  WifiConfigDelegateChromeOsTest()
      : fake_managed_network_configuration_handler_(
            new FakeManagedNetworkConfigurationHandler()) {
    config_delegate_.reset(new WifiConfigDelegateChromeOs(
        kUserHash, fake_managed_network_configuration_handler_.get()));
  }

  // Wrapper for WifiConfigDelegateChromeOs::AddToLocalNetworks.
  void AddToLocalNetworks(const WifiCredential& network_credential) {
    config_delegate_->AddToLocalNetworks(network_credential);
  }

  // Returns a new WifiCredential constructed from the given parameters.
  WifiCredential MakeCredential(const std::string& ssid,
                                WifiSecurityClass security_class,
                                const std::string& passphrase) {
    std::unique_ptr<WifiCredential> credential = WifiCredential::Create(
        WifiCredential::MakeSsidBytesForTest(ssid), security_class, passphrase);
    CHECK(credential);
    return *credential;
  }

  // Runs the last |callback| passed to CreateConfiguration, unless
  // that |callback| is null.
  void RunCreateConfigurationSuccessCallback() {
    const char new_service_path[] = "/service/0";
    const ServiceResultCallback callback =
        fake_managed_network_configuration_handler_
            ->create_configuration_success_callback();
    if (!callback.is_null())
      callback.Run(new_service_path, nullptr);
  }

  // Returns whether or not CreateConfiguration has been called
  // on |fake_managed_network_configuration_handler_|.
  size_t create_configuration_called() const {
    return fake_managed_network_configuration_handler_
        ->create_configuration_called();
  }

  // Returns the last |error_callback| passed to the CreateConfiguration
  // method of |fake_managed_network_configuration_handler_|.
  const ErrorCallback& create_configuration_error_callback() const {
    return fake_managed_network_configuration_handler_
        ->create_configuration_error_callback();
  }

 private:
  std::unique_ptr<WifiConfigDelegateChromeOs> config_delegate_;
  std::unique_ptr<FakeManagedNetworkConfigurationHandler>
      fake_managed_network_configuration_handler_;

  DISALLOW_COPY_AND_ASSIGN(WifiConfigDelegateChromeOsTest);
};

TEST_F(WifiConfigDelegateChromeOsTest, AddToLocalNetworksOpen) {
  AddToLocalNetworks(MakeCredential(kSsid, SECURITY_CLASS_NONE, ""));
  ASSERT_TRUE(create_configuration_called());
  RunCreateConfigurationSuccessCallback();
}

TEST_F(WifiConfigDelegateChromeOsTest, AddToLocalNetworksWep) {
  AddToLocalNetworks(MakeCredential(kSsid, SECURITY_CLASS_WEP, "abcde"));
  ASSERT_TRUE(create_configuration_called());
  RunCreateConfigurationSuccessCallback();
}

TEST_F(WifiConfigDelegateChromeOsTest, AddToLocalNetworksPsk) {
  AddToLocalNetworks(
      MakeCredential(kSsid, SECURITY_CLASS_PSK, "fake-psk-passphrase"));
  ASSERT_TRUE(create_configuration_called());
  RunCreateConfigurationSuccessCallback();
}

TEST_F(WifiConfigDelegateChromeOsTest, AddToLocalNetworksNonUtf8) {
  AddToLocalNetworks(MakeCredential(kSsidNonUtf8, SECURITY_CLASS_PSK, ""));
  // TODO(quiche): Change to EXPECT_TRUE, once we support non-UTF-8 SSIDs.
  EXPECT_FALSE(create_configuration_called());
}

TEST_F(WifiConfigDelegateChromeOsTest,
       AddToLocalNetworksCreateConfigurationFailure) {
  AddToLocalNetworks(MakeCredential(kSsid, SECURITY_CLASS_NONE, ""));
  EXPECT_TRUE(create_configuration_called());
  if (!create_configuration_error_callback().is_null()) {
    create_configuration_error_callback().Run(
        "Config.CreateConfiguration Failed",
        std::make_unique<base::DictionaryValue>());
  }
}

}  // namespace sync_wifi
