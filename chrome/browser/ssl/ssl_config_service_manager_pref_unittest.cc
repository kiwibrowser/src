// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "chrome/browser/ssl/ssl_config_service_manager.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/variations_params_manager.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/ssl/ssl_config.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/ssl_config.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ListValue;

class SSLConfigServiceManagerPrefTest : public testing::Test,
                                        public network::mojom::SSLConfigClient {
 public:
  SSLConfigServiceManagerPrefTest() : binding_(this) {}

  ~SSLConfigServiceManagerPrefTest() override {
    EXPECT_EQ(updates_waited_for_, observed_configs_.size());
  }

  std::unique_ptr<SSLConfigServiceManager> SetUpConfigServiceManager(
      TestingPrefServiceSimple* local_state) {
    std::unique_ptr<SSLConfigServiceManager> config_manager(
        SSLConfigServiceManager::CreateDefaultManager(local_state));
    if (!config_manager)
      return nullptr;

    // Create NetworkContextParams, pass it to the |config_manager|, and then
    // steal the only two params that the |config_manager| populates.
    network::mojom::NetworkContextParamsPtr network_context_params =
        network::mojom::NetworkContextParams::New();
    config_manager->AddToNetworkContextParams(network_context_params.get());
    EXPECT_TRUE(network_context_params->initial_ssl_config);
    initial_config_ = std::move(network_context_params->initial_ssl_config);
    EXPECT_TRUE(network_context_params->ssl_config_client_request);
    // It's safe to destroy the SSLConfigServiceManager before |binding_|.
    binding_.Bind(std::move(network_context_params->ssl_config_client_request));
    return config_manager;
  }

  // Waits for a single SSLConfigUpdate call. Expected to be called once for
  // every update, and does not support multple updates occuring between calls.
  void WaitForUpdate() {
    ASSERT_FALSE(run_loop_);

    ++updates_waited_for_;
    if (observed_configs_.size() == updates_waited_for_)
      return;

    // Fail if there was more than one update since the last call to
    // WaitForUpdate.
    ASSERT_EQ(updates_waited_for_, observed_configs_.size() + 1);

    // Not going to have much luck waiting for an update if this isn't bound to
    // anything.
    ASSERT_TRUE(binding_.is_bound());

    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
    run_loop_.reset();
    // Fail if there was more than one update while spinning the message loop.
    ASSERT_EQ(updates_waited_for_, observed_configs_.size());
  }

  // network::mojom::SSLConfigClient implementation:
  void OnSSLConfigUpdated(network::mojom::SSLConfigPtr ssl_config) override {
    observed_configs_.emplace_back(std::move(ssl_config));
    if (run_loop_)
      run_loop_->Quit();
  }

 protected:
  base::MessageLoop message_loop_;

  TestingPrefServiceSimple local_state_;

  mojo::Binding<network::mojom::SSLConfigClient> binding_;
  network::mojom::SSLConfigPtr initial_config_;
  std::vector<network::mojom::SSLConfigPtr> observed_configs_;
  size_t updates_waited_for_ = 0;
  std::unique_ptr<base::RunLoop> run_loop_;
};

// Test that cipher suites can be disabled. "Good" refers to the fact that
// every value is expected to be successfully parsed into a cipher suite.
TEST_F(SSLConfigServiceManagerPrefTest, GoodDisabledCipherSuites) {
  TestingPrefServiceSimple local_state;
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());
  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_TRUE(initial_config_->disabled_cipher_suites.empty());

  auto list_value = std::make_unique<base::ListValue>();
  list_value->AppendString("0x0004");
  list_value->AppendString("0x0005");
  local_state.SetUserPref(prefs::kCipherSuiteBlacklist, std::move(list_value));

  // Wait for the SSLConfigServiceManagerPref to be notified of the preferences
  // being changed, and for it to notify the test fixture of the change.
  ASSERT_NO_FATAL_FAILURE(WaitForUpdate());

  EXPECT_NE(initial_config_->disabled_cipher_suites,
            observed_configs_[0]->disabled_cipher_suites);
  ASSERT_EQ(2u, observed_configs_[0]->disabled_cipher_suites.size());
  EXPECT_EQ(0x0004, observed_configs_[0]->disabled_cipher_suites[0]);
  EXPECT_EQ(0x0005, observed_configs_[0]->disabled_cipher_suites[1]);
}

// Test that cipher suites can be disabled. "Bad" refers to the fact that
// there are one or more non-cipher suite strings in the preference. They
// should be ignored.
TEST_F(SSLConfigServiceManagerPrefTest, BadDisabledCipherSuites) {
  TestingPrefServiceSimple local_state;
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());
  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_TRUE(initial_config_->disabled_cipher_suites.empty());

  auto list_value = std::make_unique<base::ListValue>();
  list_value->AppendString("0x0004");
  list_value->AppendString("TLS_NOT_WITH_A_CIPHER_SUITE");
  list_value->AppendString("0x0005");
  list_value->AppendString("0xBEEFY");
  local_state.SetUserPref(prefs::kCipherSuiteBlacklist, std::move(list_value));

  // Wait for the SSLConfigServiceManagerPref to be notified of the preferences
  // being changed, and for it to notify the test fixture of the change.
  ASSERT_NO_FATAL_FAILURE(WaitForUpdate());

  EXPECT_NE(initial_config_->disabled_cipher_suites,
            observed_configs_[0]->disabled_cipher_suites);
  ASSERT_EQ(2u, observed_configs_[0]->disabled_cipher_suites.size());
  EXPECT_EQ(0x0004, observed_configs_[0]->disabled_cipher_suites[0]);
  EXPECT_EQ(0x0005, observed_configs_[0]->disabled_cipher_suites[1]);
}

// Test that without command-line settings for minimum and maximum SSL versions,
// TLS versions from 1.0 up to 1.1 or 1.2 are enabled.
TEST_F(SSLConfigServiceManagerPrefTest, NoCommandLinePrefs) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());
  TestingPrefServiceSimple local_state;
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());
  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  // The settings should not be added to the local_state.
  EXPECT_FALSE(local_state.HasPrefPath(prefs::kSSLVersionMin));
  EXPECT_FALSE(local_state.HasPrefPath(prefs::kSSLVersionMax));
  EXPECT_FALSE(local_state.HasPrefPath(prefs::kTLS13Variant));

  // Explicitly double-check the settings are not in the preference store.
  std::string version_min_str;
  std::string version_max_str;
  std::string tls13_variant_str;
  EXPECT_FALSE(
      local_state_store->GetString(prefs::kSSLVersionMin, &version_min_str));
  EXPECT_FALSE(
      local_state_store->GetString(prefs::kSSLVersionMax, &version_max_str));
  EXPECT_FALSE(
      local_state_store->GetString(prefs::kTLS13Variant, &tls13_variant_str));
}

// Tests that "ssl3" is not treated as a valid minimum version.
TEST_F(SSLConfigServiceManagerPrefTest, NoSSL3) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  TestingPrefServiceSimple local_state;
  local_state.SetUserPref(prefs::kSSLVersionMin,
                          std::make_unique<base::Value>("ssl3"));
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  // The command-line option must not have been honored.
  // TODO(mmenke):  SSL3 no longer even has an enum value. Does this test
  // matter?
  EXPECT_LE(network::mojom::SSLVersion::kTLS1, initial_config_->version_min);
}

// Tests that SSLVersionMin correctly sets the minimum version.
TEST_F(SSLConfigServiceManagerPrefTest, SSLVersionMin) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  TestingPrefServiceSimple local_state;
  local_state.SetUserPref(prefs::kSSLVersionMin,
                          std::make_unique<base::Value>("tls1.1"));
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_EQ(network::mojom::SSLVersion::kTLS11, initial_config_->version_min);
}

// Tests that SSL max version correctly sets the maximum version.
TEST_F(SSLConfigServiceManagerPrefTest, SSLVersionMax) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  TestingPrefServiceSimple local_state;
  local_state.SetUserPref(prefs::kSSLVersionMax,
                          std::make_unique<base::Value>("tls1.3"));
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_EQ(network::mojom::SSLVersion::kTLS13, initial_config_->version_max);
}

// Tests that SSL max version can not be set below TLS 1.2.
TEST_F(SSLConfigServiceManagerPrefTest, NoTLS11Max) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  TestingPrefServiceSimple local_state;
  local_state.SetUserPref(prefs::kSSLVersionMax,
                          std::make_unique<base::Value>("tls1.1"));
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  // The command-line option must not have been honored.
  EXPECT_LE(network::mojom::SSLVersion::kTLS12, initial_config_->version_max);
}

// Tests that TLS 1.3 can be disabled via field trials.
TEST_F(SSLConfigServiceManagerPrefTest, TLS13VariantFeatureDisabled) {
  // Toggle the field trial.
  variations::testing::VariationParamsManager variation_params(
      "TLS13Variant", {{"variant", "disabled"}});

  TestingPrefServiceSimple local_state;
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_EQ(network::mojom::SSLVersion::kTLS12, initial_config_->version_max);
}

// Tests that Draft23 TLS 1.3 can be enabled via field trials.
TEST_F(SSLConfigServiceManagerPrefTest, TLS13VariantFeatureDraft23) {
  // Toggle the field trial.
  variations::testing::VariationParamsManager variation_params(
      "TLS13Variant", {{"variant", "draft23"}});

  TestingPrefServiceSimple local_state;
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_EQ(network::mojom::SSLVersion::kTLS13, initial_config_->version_max);
  EXPECT_EQ(network::mojom::TLS13Variant::kDraft23,
            initial_config_->tls13_variant);
}

// Tests that Draft28 TLS 1.3 can be enabled via field trials.
TEST_F(SSLConfigServiceManagerPrefTest, TLS13VariantFeatureDraft28) {
  // Toggle the field trial.
  variations::testing::VariationParamsManager variation_params(
      "TLS13Variant", {{"variant", "draft28"}});

  TestingPrefServiceSimple local_state;
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_EQ(network::mojom::SSLVersion::kTLS13, initial_config_->version_max);
  EXPECT_EQ(network::mojom::TLS13Variant::kDraft28,
            initial_config_->tls13_variant);
}

// Tests that the SSLVersionMax preference overwites the TLS 1.3 variant
// field trial.
TEST_F(SSLConfigServiceManagerPrefTest, TLS13SSLVersionMax) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  // Toggle the field trial.
  variations::testing::VariationParamsManager variation_params(
      "TLS13Variant", {{"variant", "draft23"}});

  TestingPrefServiceSimple local_state;
  local_state.SetUserPref(prefs::kSSLVersionMax,
                          std::make_unique<base::Value>("tls1.2"));
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_EQ(network::mojom::SSLVersion::kTLS12, initial_config_->version_max);
}

// Tests that disabling TLS 1.3 by preference overwrites the TLS 1.3 field
// trial.
TEST_F(SSLConfigServiceManagerPrefTest, TLS13VariantOverrideDisable) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  // Toggle the field trial.
  variations::testing::VariationParamsManager variation_params(
      "TLS13Variant", {{"variant", "draft23"}});

  TestingPrefServiceSimple local_state;
  local_state.SetUserPref(prefs::kTLS13Variant,
                          std::make_unique<base::Value>("disabled"));
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_EQ(network::mojom::SSLVersion::kTLS12, initial_config_->version_max);
}

// Tests that enabling TLS 1.3 by preference overwrites the TLS 1.3 field trial.
TEST_F(SSLConfigServiceManagerPrefTest, TLS13VariantOverrideEnable) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  // Toggle the field trial.
  variations::testing::VariationParamsManager variation_params(
      "TLS13Variant", {{"variant", "disabled"}});

  TestingPrefServiceSimple local_state;
  local_state.SetUserPref(prefs::kSSLVersionMax,
                          std::make_unique<base::Value>("tls1.3"));
  local_state.SetUserPref(prefs::kTLS13Variant,
                          std::make_unique<base::Value>("draft23"));
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  EXPECT_EQ(network::mojom::SSLVersion::kTLS13, initial_config_->version_max);
  EXPECT_EQ(network::mojom::TLS13Variant::kDraft23,
            initial_config_->tls13_variant);
}

// Tests that SHA-1 signatures for local trust anchors can be enabled.
TEST_F(SSLConfigServiceManagerPrefTest, SHA1ForLocalAnchors) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  TestingPrefServiceSimple local_state;
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  // By default, SHA-1 local trust anchors should not be enabled when not
  // not using any pref service.
  EXPECT_FALSE(net::SSLConfig().sha1_local_anchors_enabled);
  EXPECT_FALSE(network::mojom::SSLConfig::New()->sha1_local_anchors_enabled);

  // Using a pref service without any preference set should result in
  // SHA-1 local trust anchors being disabled.
  EXPECT_FALSE(initial_config_->sha1_local_anchors_enabled);

  // Enabling the local preference should result in SHA-1 local trust anchors
  // being enabled.
  local_state.SetUserPref(prefs::kCertEnableSha1LocalAnchors,
                          std::make_unique<base::Value>(true));
  // Wait for the SSLConfigServiceManagerPref to be notified of the preferences
  // being changed, and for it to notify the test fixture of the change.
  ASSERT_NO_FATAL_FAILURE(WaitForUpdate());
  EXPECT_TRUE(observed_configs_[0]->sha1_local_anchors_enabled);

  // Disabling the local preference should result in SHA-1 local trust
  // anchors being disabled.
  local_state.SetUserPref(prefs::kCertEnableSha1LocalAnchors,
                          std::make_unique<base::Value>(false));
  // Wait for the SSLConfigServiceManagerPref to be notified of the preferences
  // being changed, and for it to notify the test fixture of the change.
  ASSERT_NO_FATAL_FAILURE(WaitForUpdate());
  EXPECT_FALSE(observed_configs_[1]->sha1_local_anchors_enabled);
}

// Tests that Symantec's legacy infrastructure can be enabled.
TEST_F(SSLConfigServiceManagerPrefTest, SymantecLegacyInfrastructure) {
  scoped_refptr<TestingPrefStore> local_state_store(new TestingPrefStore());

  TestingPrefServiceSimple local_state;
  SSLConfigServiceManager::RegisterPrefs(local_state.registry());

  std::unique_ptr<SSLConfigServiceManager> config_manager =
      SetUpConfigServiceManager(&local_state);

  // By default, Symantec's legacy infrastructure should be disabled when
  // not using any pref service.
  EXPECT_FALSE(net::SSLConfig().symantec_enforcement_disabled);
  EXPECT_FALSE(network::mojom::SSLConfig::New()->symantec_enforcement_disabled);

  // Using a pref service without any preference set should result in
  // Symantec's legacy infrastructure being disabled.
  EXPECT_FALSE(initial_config_->symantec_enforcement_disabled);

  // Enabling the local preference should result in Symantec's legacy
  // infrastructure being enabled.
  local_state.SetUserPref(prefs::kCertEnableSymantecLegacyInfrastructure,
                          std::make_unique<base::Value>(true));
  // Wait for the SSLConfigServiceManagerPref to be notified of the preferences
  // being changed, and for it to notify the test fixture of the change.
  ASSERT_NO_FATAL_FAILURE(WaitForUpdate());
  EXPECT_TRUE(observed_configs_[0]->symantec_enforcement_disabled);

  // Disabling the local preference should result in Symantec's legacy
  // infrastructure being disabled.
  local_state.SetUserPref(prefs::kCertEnableSymantecLegacyInfrastructure,
                          std::make_unique<base::Value>(false));
  // Wait for the SSLConfigServiceManagerPref to be notified of the preferences
  // being changed, and for it to notify the test fixture of the change.
  ASSERT_NO_FATAL_FAILURE(WaitForUpdate());
  EXPECT_FALSE(observed_configs_[1]->symantec_enforcement_disabled);
}
