// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/ssl_config_service_mojo.h"

#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"
#include "net/url_request/url_request_context.h"
#include "services/network/network_context.h"
#include "services/network/network_service.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/ssl_config.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {
namespace {

class TestSSLConfigServiceObserver : public net::SSLConfigService::Observer {
 public:
  explicit TestSSLConfigServiceObserver(
      net::SSLConfigService* ssl_config_service)
      : ssl_config_service_(ssl_config_service) {
    ssl_config_service_->AddObserver(this);
  }

  ~TestSSLConfigServiceObserver() override {
    EXPECT_EQ(observed_changes_, changes_to_wait_for_);
    ssl_config_service_->RemoveObserver(this);
  }

  // net::SSLConfigService::Observer implementation:
  void OnSSLConfigChanged() override {
    ++observed_changes_;
    ssl_config_service_->GetSSLConfig(&ssl_config_during_change_);
    if (run_loop_)
      run_loop_->Quit();
  }

  // Waits for a SSLConfig change. The first time it's called, waits for the
  // first change, if one hasn't been observed already, the second time, waits
  // for the second, etc. Must be called once for each change that happens, and
  // fails it more than once change happens between calls, or during a call.
  void WaitForChange() {
    EXPECT_FALSE(run_loop_);
    ++changes_to_wait_for_;
    if (changes_to_wait_for_ == observed_changes_)
      return;
    EXPECT_LT(observed_changes_, changes_to_wait_for_);

    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
    run_loop_.reset();
    EXPECT_EQ(observed_changes_, changes_to_wait_for_);
  }

  const net::SSLConfig& ssl_config_during_change() const {
    return ssl_config_during_change_;
  }

  int observed_changes() const { return observed_changes_; }

 private:
  net::SSLConfigService* const ssl_config_service_;
  int observed_changes_ = 0;
  int changes_to_wait_for_ = 0;
  net::SSLConfig ssl_config_during_change_;
  std::unique_ptr<base::RunLoop> run_loop_;
};

class NetworkServiceSSLConfigServiceTest : public testing::Test {
 public:
  NetworkServiceSSLConfigServiceTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        network_service_(NetworkService::CreateForTesting()) {}

  // Creates a NetworkContext using the specified NetworkContextParams, and
  // stores it in |network_context_|.
  void SetUpNetworkContext(
      mojom::NetworkContextParamsPtr network_context_params) {
    network_context_params->ssl_config_client_request =
        mojo::MakeRequest(&ssl_config_client_);
    network_context_ = std::make_unique<NetworkContext>(
        network_service_.get(), mojo::MakeRequest(&network_context_ptr_),
        std::move(network_context_params));
  }

  // Returns the current SSLConfig for |network_context_|.
  net::SSLConfig GetSSLConfig() {
    net::SSLConfig ssl_config;
    network_context_->url_request_context()->ssl_config_service()->GetSSLConfig(
        &ssl_config);
    return ssl_config;
  }

  // Runs two conversion tests for |mojo_config|.  Uses it as a initial
  // SSLConfig for a NetworkContext, making sure it matches
  // |expected_net_config|. Then switches to the default configuration and then
  // back to |mojo_config|, to make sure it works as a new configuration. The
  // expected configuration must not be the default configuration.
  void RunConversionTests(const mojom::SSLConfig& mojo_config,
                          const net::SSLConfig& expected_net_config) {
    // The expected configuration must not be the default configuration, or the
    // change test won't send an event.
    EXPECT_FALSE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
        net::SSLConfig(), expected_net_config));

    // Set up |mojo_config| as the initial configuration of a NetworkContext.
    mojom::NetworkContextParamsPtr network_context_params =
        mojom::NetworkContextParams::New();
    network_context_params->initial_ssl_config = mojo_config.Clone();
    SetUpNetworkContext(std::move(network_context_params));
    EXPECT_TRUE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
        GetSSLConfig(), expected_net_config));
    // Sanity check.
    EXPECT_FALSE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
        GetSSLConfig(), net::SSLConfig()));

    // Reset the configuration to the default ones, and check the results.
    TestSSLConfigServiceObserver observer(
        network_context_->url_request_context()->ssl_config_service());
    ssl_config_client_->OnSSLConfigUpdated(mojom::SSLConfig::New());
    observer.WaitForChange();
    EXPECT_TRUE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
        GetSSLConfig(), net::SSLConfig()));
    EXPECT_TRUE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
        observer.ssl_config_during_change(), net::SSLConfig()));
    // Sanity check.
    EXPECT_FALSE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
        GetSSLConfig(), expected_net_config));

    // Set the configuration to |mojo_config| again, and check the results.
    ssl_config_client_->OnSSLConfigUpdated(mojo_config.Clone());
    observer.WaitForChange();
    EXPECT_TRUE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
        GetSSLConfig(), expected_net_config));
    EXPECT_TRUE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
        observer.ssl_config_during_change(), expected_net_config));
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<NetworkService> network_service_;
  mojom::SSLConfigClientPtr ssl_config_client_;
  mojom::NetworkContextPtr network_context_ptr_;
  std::unique_ptr<NetworkContext> network_context_;
};

// Check that passing in a no mojom::SSLConfig matches the default
// net::SSLConfig.
TEST_F(NetworkServiceSSLConfigServiceTest, NoSSLConfig) {
  SetUpNetworkContext(mojom::NetworkContextParams::New());
  EXPECT_TRUE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
      GetSSLConfig(), net::SSLConfig()));

  // Make sure the default TLS version range is as expected.
  EXPECT_EQ(net::kDefaultSSLVersionMin, GetSSLConfig().version_min);
  EXPECT_EQ(net::kDefaultSSLVersionMax, GetSSLConfig().version_max);
  EXPECT_EQ(net::kDefaultTLS13Variant, GetSSLConfig().tls13_variant);
}

// Check that passing in the default mojom::SSLConfig matches the default
// net::SSLConfig.
TEST_F(NetworkServiceSSLConfigServiceTest, Default) {
  mojom::NetworkContextParamsPtr network_context_params =
      mojom::NetworkContextParams::New();
  network_context_params->initial_ssl_config = mojom::SSLConfig::New();
  SetUpNetworkContext(std::move(network_context_params));
  EXPECT_TRUE(net::SSLConfigService::SSLConfigsAreEqualForTesting(
      GetSSLConfig(), net::SSLConfig()));

  // Make sure the default TLS version range is as expected.
  EXPECT_EQ(net::kDefaultSSLVersionMin, GetSSLConfig().version_min);
  EXPECT_EQ(net::kDefaultSSLVersionMax, GetSSLConfig().version_max);
  EXPECT_EQ(net::kDefaultTLS13Variant, GetSSLConfig().tls13_variant);
}

TEST_F(NetworkServiceSSLConfigServiceTest, RevCheckingEnabled) {
  net::SSLConfig expected_net_config;
  // Use the opposite of the default value.
  expected_net_config.rev_checking_enabled =
      !expected_net_config.rev_checking_enabled;

  mojom::SSLConfigPtr mojo_config = mojom::SSLConfig::New();
  mojo_config->rev_checking_enabled = expected_net_config.rev_checking_enabled;

  RunConversionTests(*mojo_config, expected_net_config);
}

TEST_F(NetworkServiceSSLConfigServiceTest,
       RevCheckingRequiredLocalTrustAnchors) {
  net::SSLConfig expected_net_config;
  // Use the opposite of the default value.
  expected_net_config.rev_checking_required_local_anchors =
      !expected_net_config.rev_checking_required_local_anchors;

  mojom::SSLConfigPtr mojo_config = mojom::SSLConfig::New();
  mojo_config->rev_checking_required_local_anchors =
      expected_net_config.rev_checking_required_local_anchors;

  RunConversionTests(*mojo_config, expected_net_config);
}

TEST_F(NetworkServiceSSLConfigServiceTest, Sha1LocalAnchorsEnabled) {
  net::SSLConfig expected_net_config;
  // Use the opposite of the default value.
  expected_net_config.sha1_local_anchors_enabled =
      !expected_net_config.sha1_local_anchors_enabled;

  mojom::SSLConfigPtr mojo_config = mojom::SSLConfig::New();
  mojo_config->sha1_local_anchors_enabled =
      expected_net_config.sha1_local_anchors_enabled;

  RunConversionTests(*mojo_config, expected_net_config);
}

TEST_F(NetworkServiceSSLConfigServiceTest, SymantecEnforcementDisabled) {
  net::SSLConfig expected_net_config;
  // Use the opposite of the default value.
  expected_net_config.symantec_enforcement_disabled =
      !expected_net_config.symantec_enforcement_disabled;

  mojom::SSLConfigPtr mojo_config = mojom::SSLConfig::New();
  mojo_config->symantec_enforcement_disabled =
      expected_net_config.symantec_enforcement_disabled;

  RunConversionTests(*mojo_config, expected_net_config);
}

TEST_F(NetworkServiceSSLConfigServiceTest, SSLVersion) {
  const struct {
    mojom::SSLVersion mojo_ssl_version;
    int net_ssl_version;
  } kVersionTable[] = {
      {mojom::SSLVersion::kTLS1, net::SSL_PROTOCOL_VERSION_TLS1},
      {mojom::SSLVersion::kTLS11, net::SSL_PROTOCOL_VERSION_TLS1_1},
      {mojom::SSLVersion::kTLS12, net::SSL_PROTOCOL_VERSION_TLS1_2},
      {mojom::SSLVersion::kTLS13, net::SSL_PROTOCOL_VERSION_TLS1_3},
  };

  for (size_t min_index = 0; min_index < base::size(kVersionTable);
       ++min_index) {
    for (size_t max_index = min_index; max_index < base::size(kVersionTable);
         ++max_index) {
      // If the versions match the default values, skip this value in the table.
      // The defaults will get plenty of testing anyways, when switching back to
      // the default values in RunConversionTests().
      if (kVersionTable[min_index].net_ssl_version ==
              net::SSLConfig().version_min &&
          kVersionTable[max_index].net_ssl_version ==
              net::SSLConfig().version_max) {
        continue;
      }
      net::SSLConfig expected_net_config;
      expected_net_config.version_min =
          kVersionTable[min_index].net_ssl_version;
      expected_net_config.version_max =
          kVersionTable[max_index].net_ssl_version;

      mojom::SSLConfigPtr mojo_config = mojom::SSLConfig::New();
      mojo_config->version_min = kVersionTable[min_index].mojo_ssl_version;
      mojo_config->version_max = kVersionTable[max_index].mojo_ssl_version;

      RunConversionTests(*mojo_config, expected_net_config);
    }
  }
}

TEST_F(NetworkServiceSSLConfigServiceTest, InitialConfigDisableCipherSuite) {
  net::SSLConfig expected_net_config;
  expected_net_config.disabled_cipher_suites.push_back(0x0004);

  mojom::SSLConfigPtr mojo_config = mojom::SSLConfig::New();
  mojo_config->disabled_cipher_suites =
      expected_net_config.disabled_cipher_suites;

  RunConversionTests(*mojo_config, expected_net_config);
}

TEST_F(NetworkServiceSSLConfigServiceTest,
       InitialConfigDisableTwoCipherSuites) {
  net::SSLConfig expected_net_config;
  expected_net_config.disabled_cipher_suites.push_back(0x0004);
  expected_net_config.disabled_cipher_suites.push_back(0x0005);

  mojom::SSLConfigPtr mojo_config = mojom::SSLConfig::New();
  mojo_config->disabled_cipher_suites =
      expected_net_config.disabled_cipher_suites;

  RunConversionTests(*mojo_config, expected_net_config);
}

}  // namespace
}  // namespace network
