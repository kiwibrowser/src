// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "chrome/browser/chromeos/policy/device_network_configuration_updater.h"
#include "chrome/browser/chromeos/policy/user_network_configuration_updater.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/scoped_cros_settings_test_helper.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/network/fake_network_device_handler.h"
#include "chromeos/network/mock_managed_network_configuration_handler.h"
#include "chromeos/network/onc/onc_certificate_importer.h"
#include "chromeos/network/onc/onc_parsed_certificates.h"
#include "chromeos/network/onc/onc_test_utils.h"
#include "chromeos/network/onc/onc_utils.h"
#include "chromeos/system/fake_statistics_provider.h"
#include "chromeos/system/statistics_provider.h"
#include "components/account_id/account_id.h"
#include "components/onc/onc_constants.h"
#include "components/policy/core/common/external_data_fetcher.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_service_impl.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_type.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_nss.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::AnyNumber;
using testing::AtLeast;
using testing::Mock;
using testing::Ne;
using testing::Return;
using testing::StrictMock;
using testing::_;

namespace policy {

namespace {

const char kFakeUserEmail[] = "fake email";
const char kFakeUsernameHash[] = "fake hash";
const char kFakeSerialNumber[] = "FakeSerial";
const char kFakeAssetId[] = "FakeAssetId";

class FakeUser : public user_manager::User {
 public:
  FakeUser() : User(AccountId::FromUserEmail(kFakeUserEmail)) {
    set_display_email(kFakeUserEmail);
    set_username_hash(kFakeUsernameHash);
  }
  ~FakeUser() override {}

  // User overrides
  user_manager::UserType GetType() const override {
    return user_manager::USER_TYPE_REGULAR;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeUser);
};

class FakeWebTrustedCertsObserver
    : public UserNetworkConfigurationUpdater::WebTrustedCertsObserver {
 public:
  FakeWebTrustedCertsObserver() {}

  void OnTrustAnchorsChanged(
      const net::CertificateList& trust_anchors) override {
    trust_anchors_ = trust_anchors;
  }
  net::CertificateList trust_anchors_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeWebTrustedCertsObserver);
};

class FakeNetworkDeviceHandler : public chromeos::FakeNetworkDeviceHandler {
 public:
  FakeNetworkDeviceHandler()
      : allow_roaming_(false), mac_addr_randomization_(false) {}

  void SetCellularAllowRoaming(bool allow_roaming) override {
    allow_roaming_ = allow_roaming;
  }

  void SetMACAddressRandomizationEnabled(bool enabled) override {
    mac_addr_randomization_ = enabled;
  }

  bool allow_roaming_;
  bool mac_addr_randomization_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeNetworkDeviceHandler);
};

class FakeCertificateImporter : public chromeos::onc::CertificateImporter {
 public:
  FakeCertificateImporter()
      : expected_onc_source_(::onc::ONC_SOURCE_UNKNOWN), call_count_(0) {}
  ~FakeCertificateImporter() override {}

  void SetTrustedCertificatesResult(
      net::ScopedCERTCertificateList onc_trusted_certificates) {
    onc_trusted_certificates_ = std::move(onc_trusted_certificates);
  }

  void SetExpectedONCCertificates(
      std::unique_ptr<chromeos::onc::OncParsedCertificates>
          expected_onc_certificates) {
    expected_onc_certificates_ = std::move(expected_onc_certificates);
  }

  void SetExpectedONCSource(::onc::ONCSource source) {
    expected_onc_source_ = source;
  }

  unsigned int GetAndResetImportCount() {
    unsigned int count = call_count_;
    call_count_ = 0;
    return count;
  }

  void ImportCertificates(
      std::unique_ptr<chromeos::onc::OncParsedCertificates> certificates,
      ::onc::ONCSource source,
      DoneCallback done_callback) override {
    if (expected_onc_source_ != ::onc::ONC_SOURCE_UNKNOWN)
      EXPECT_EQ(expected_onc_source_, source);
    if (expected_onc_certificates_) {
      EXPECT_EQ(expected_onc_certificates_->has_error(),
                certificates->has_error());
      EXPECT_EQ(expected_onc_certificates_->server_or_authority_certificates(),
                certificates->server_or_authority_certificates());
      EXPECT_EQ(expected_onc_certificates_->client_certificates(),
                certificates->client_certificates());
    }

    ++call_count_;
    std::move(done_callback).Run(true, std::move(onc_trusted_certificates_));
  }

 private:
  ::onc::ONCSource expected_onc_source_;
  std::unique_ptr<chromeos::onc::OncParsedCertificates>
      expected_onc_certificates_;
  net::ScopedCERTCertificateList onc_trusted_certificates_;
  unsigned int call_count_;

  DISALLOW_COPY_AND_ASSIGN(FakeCertificateImporter);
};

// Note: HexSSID 737369642D6E6F6E65 maps to "ssid-none".
// HexSSID 7465737431323334 maps to "test1234"
const char kFakeONC[] = R"(
    { "NetworkConfigurations": [
        { "GUID": "{485d6076-dd44-6b6d-69787465725f5040}",
          "Type": "WiFi",
          "Name": "My WiFi Network",
          "WiFi": {
            "HexSSID": "737369642D6E6F6E65",
            "Security": "None" }
        },
        { "GUID": "{guid-for-wifi-with-device-exp}",
          "Type": "WiFi",
          "Name": "My WiFi with device placeholder expansions",
          "WiFi": {
            "EAP": {
              "Outer": "EAP-TLS",
              "Identity": "${DEVICE_SERIAL_NUMBER}-${DEVICE_ASSET_ID}"
            },
            "HexSSID": "7465737431323334",
            "Security": "WPA-EAP",
            "SSID": "test1234",
          }
        }
      ],
      "GlobalNetworkConfiguration": {
        "AllowOnlyPolicyNetworksToAutoconnect": true,
      },
      "Certificates": [
        { "GUID": "{f998f760-272b-6939-4c2beffe428697ac}",
          "PKCS12": "YWJj",
          "Type": "Client" }
      ],
      "Type": "UnencryptedConfiguration"
    })";

std::string ValueToString(const base::Value& value) {
  std::stringstream str;
  str << value;
  return str.str();
}

// Matcher to match base::Value.
MATCHER_P(IsEqualTo,
          value,
          std::string(negation ? "isn't" : "is") + " equal to " +
              ValueToString(*value)) {
  return value->Equals(&arg);
}

MATCHER(IsEmpty, std::string(negation ? "isn't" : "is") + " empty.") {
  return arg.empty();
}

ACTION_P(SetCertificateList, list) {
  if (arg2)
    *arg2 = list;
  return true;
}

}  // namespace

class NetworkConfigurationUpdaterTest : public testing::Test {
 protected:
  NetworkConfigurationUpdaterTest() : certificate_importer_(NULL) {}

  void SetUp() override {
    fake_statistics_provider_.SetMachineStatistic(
        chromeos::system::kSerialNumberKeyForTest, kFakeSerialNumber);

    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(false));
    provider_.Init();
    PolicyServiceImpl::Providers providers;
    providers.push_back(&provider_);
    policy_service_ = std::make_unique<PolicyServiceImpl>(std::move(providers));

    std::unique_ptr<base::DictionaryValue> fake_toplevel_onc =
        chromeos::onc::ReadDictionaryFromJson(kFakeONC);

    base::DictionaryValue* global_config = NULL;
    fake_toplevel_onc->GetDictionaryWithoutPathExpansion(
        onc::toplevel_config::kGlobalNetworkConfiguration, &global_config);
    fake_global_network_config_.MergeDictionary(global_config);

    base::ListValue* certs = NULL;
    fake_toplevel_onc->GetListWithoutPathExpansion(
        onc::toplevel_config::kCertificates, &certs);
    fake_certificates_ =
        std::make_unique<chromeos::onc::OncParsedCertificates>(*certs);

    certificate_importer_ = new FakeCertificateImporter;
    certificate_importer_owned_.reset(certificate_importer_);
  }

  base::Value* GetExpectedFakeNetworkConfigs(::onc::ONCSource source) {
    std::unique_ptr<base::DictionaryValue> fake_toplevel_onc =
        chromeos::onc::ReadDictionaryFromJson(kFakeONC);
    fake_network_configs_ =
        fake_toplevel_onc->FindKey(onc::toplevel_config::kNetworkConfigurations)
            ->Clone();
    if (source == ::onc::ONC_SOURCE_DEVICE_POLICY) {
      std::string expected_identity =
          std::string(kFakeSerialNumber) + "-" + std::string(kFakeAssetId);
      SetExpectedValueInNetworkConfig(
          &fake_network_configs_, "{guid-for-wifi-with-device-exp}",
          {"WiFi", "EAP", "Identity"}, base::Value(expected_identity));
    }
    return &fake_network_configs_;
  }

  base::Value* GetExpectedFakeGlobalNetworkConfig() {
    return &fake_global_network_config_;
  }

  void TearDown() override {
    network_configuration_updater_.reset();
    provider_.Shutdown();
    base::RunLoop().RunUntilIdle();
  }

  void MarkPolicyProviderInitialized() {
    Mock::VerifyAndClearExpectations(&provider_);
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    provider_.SetAutoRefresh();
    provider_.RefreshPolicies();
    base::RunLoop().RunUntilIdle();
  }

  void UpdateProviderPolicy(const PolicyMap& policy) {
    provider_.UpdateChromePolicy(policy);
    base::RunLoop().RunUntilIdle();
  }

  UserNetworkConfigurationUpdater*
  CreateNetworkConfigurationUpdaterForUserPolicy(
      bool allow_trusted_certs_from_policy,
      bool set_cert_importer) {
    UserNetworkConfigurationUpdater* updater =
        UserNetworkConfigurationUpdater::CreateForUserPolicy(
            &profile_,
            allow_trusted_certs_from_policy,
            fake_user_,
            policy_service_.get(),
            &network_config_handler_).release();
    if (set_cert_importer) {
      EXPECT_TRUE(certificate_importer_owned_);
      updater->SetCertificateImporterForTest(
          std::move(certificate_importer_owned_));
    }
    network_configuration_updater_.reset(updater);
    return updater;
  }

  void CreateNetworkConfigurationUpdaterForDevicePolicy() {
    auto testing_device_asset_id_getter =
        base::BindRepeating([] { return std::string(kFakeAssetId); });
    network_configuration_updater_ =
        DeviceNetworkConfigurationUpdater::CreateForDevicePolicy(
            policy_service_.get(), &network_config_handler_,
            &network_device_handler_, chromeos::CrosSettings::Get(),
            testing_device_asset_id_getter);
  }

  content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<chromeos::onc::OncParsedCertificates> fake_certificates_;
  StrictMock<chromeos::MockManagedNetworkConfigurationHandler>
      network_config_handler_;
  FakeNetworkDeviceHandler network_device_handler_;
  chromeos::ScopedCrosSettingsTestHelper settings_helper_;
  chromeos::system::ScopedFakeStatisticsProvider fake_statistics_provider_;

  // Ownership of certificate_importer_owned_ is passed to the
  // NetworkConfigurationUpdater. When that happens, |certificate_importer_|
  // continues to point to that instance but |certificate_importer_owned_| is
  // released.
  FakeCertificateImporter* certificate_importer_;
  std::unique_ptr<chromeos::onc::CertificateImporter>
      certificate_importer_owned_;

  StrictMock<MockConfigurationPolicyProvider> provider_;
  std::unique_ptr<PolicyServiceImpl> policy_service_;
  FakeUser fake_user_;

  TestingProfile profile_;

  std::unique_ptr<NetworkConfigurationUpdater> network_configuration_updater_;

 private:
  void SetExpectedValueInNetworkConfig(
      base::Value* network_configs,
      base::StringPiece guid,
      std::initializer_list<base::StringPiece> path,
      base::Value value) {
    for (base::Value& network_config : network_configs->GetList()) {
      const base::Value* guid_value =
          network_config.FindKey(::onc::network_config::kGUID);
      if (!guid_value || guid_value->GetString() != guid)
        continue;
      network_config.SetPath(path, std::move(value));
      break;
    }
  }

  base::Value fake_network_configs_;
  base::DictionaryValue fake_global_network_config_;
};

TEST_F(NetworkConfigurationUpdaterTest, CellularAllowRoaming) {
  // Ignore network config updates.
  EXPECT_CALL(network_config_handler_, SetPolicy(_, _, _, _)).Times(AtLeast(1));

  settings_helper_.ReplaceProvider(chromeos::kSignedDataRoamingEnabled);
  settings_helper_.SetBoolean(chromeos::kSignedDataRoamingEnabled, false);
  EXPECT_FALSE(network_device_handler_.allow_roaming_);

  CreateNetworkConfigurationUpdaterForDevicePolicy();
  MarkPolicyProviderInitialized();
  settings_helper_.SetBoolean(chromeos::kSignedDataRoamingEnabled, true);
  EXPECT_TRUE(network_device_handler_.allow_roaming_);

  settings_helper_.SetBoolean(chromeos::kSignedDataRoamingEnabled, false);
  EXPECT_FALSE(network_device_handler_.allow_roaming_);
}

TEST_F(NetworkConfigurationUpdaterTest, PolicyIsValidatedAndRepaired) {
  std::unique_ptr<base::DictionaryValue> onc_repaired =
      chromeos::onc::test_utils::ReadTestDictionary(
          "repaired_toplevel_partially_invalid.onc");

  base::ListValue* network_configs_repaired = NULL;
  onc_repaired->GetListWithoutPathExpansion(
      onc::toplevel_config::kNetworkConfigurations, &network_configs_repaired);
  ASSERT_TRUE(network_configs_repaired);

  base::DictionaryValue* global_config_repaired = NULL;
  onc_repaired->GetDictionaryWithoutPathExpansion(
      onc::toplevel_config::kGlobalNetworkConfiguration,
      &global_config_repaired);
  ASSERT_TRUE(global_config_repaired);

  std::string onc_policy =
      chromeos::onc::test_utils::ReadTestData("toplevel_partially_invalid.onc");
  PolicyMap policy;
  policy.Set(key::kOpenNetworkConfiguration, POLICY_LEVEL_MANDATORY,
             POLICY_SCOPE_USER, POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(onc_policy), nullptr);
  UpdateProviderPolicy(policy);

  EXPECT_CALL(network_config_handler_,
              SetPolicy(onc::ONC_SOURCE_USER_POLICY,
                        _,
                        IsEqualTo(network_configs_repaired),
                        IsEqualTo(global_config_repaired)));
  certificate_importer_->SetExpectedONCSource(onc::ONC_SOURCE_USER_POLICY);

  CreateNetworkConfigurationUpdaterForUserPolicy(
      false /* do not allow trusted certs from policy */,
      true /* set certificate importer */);
  MarkPolicyProviderInitialized();
  EXPECT_EQ(1u, certificate_importer_->GetAndResetImportCount());
}

TEST_F(NetworkConfigurationUpdaterTest,
       DoNotAllowTrustedCertificatesFromPolicy) {
  net::ScopedCERTCertificateList cert_list =
      net::CreateCERTCertificateListFromFile(net::GetTestCertsDirectory(),
                                             "ok_cert.pem",
                                             net::X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1u, cert_list.size());

  EXPECT_CALL(network_config_handler_,
              SetPolicy(onc::ONC_SOURCE_USER_POLICY, _, _, _));
  certificate_importer_->SetTrustedCertificatesResult(std::move(cert_list));

  UserNetworkConfigurationUpdater* updater =
      CreateNetworkConfigurationUpdaterForUserPolicy(
          false /* do not allow trusted certs from policy */,
          true /* set certificate importer */);
  MarkPolicyProviderInitialized();

  // Certificates with the "Web" trust flag set should not be forwarded to
  // observers.
  FakeWebTrustedCertsObserver observer;
  updater->AddTrustedCertsObserver(&observer);

  base::RunLoop().RunUntilIdle();

  net::CertificateList trust_anchors;
  updater->GetWebTrustedCertificates(&trust_anchors);
  EXPECT_TRUE(trust_anchors.empty());

  EXPECT_TRUE(observer.trust_anchors_.empty());
  updater->RemoveTrustedCertsObserver(&observer);
}

TEST_F(NetworkConfigurationUpdaterTest,
       AllowTrustedCertificatesFromPolicyInitially) {
  // Ignore network configuration changes.
  EXPECT_CALL(network_config_handler_, SetPolicy(_, _, _, _))
      .Times(AnyNumber());

  net::ScopedCERTCertificateList cert_list =
      net::CreateCERTCertificateListFromFile(net::GetTestCertsDirectory(),
                                             "ok_cert.pem",
                                             net::X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1u, cert_list.size());

  certificate_importer_->SetExpectedONCSource(onc::ONC_SOURCE_USER_POLICY);
  certificate_importer_->SetTrustedCertificatesResult(std::move(cert_list));

  UserNetworkConfigurationUpdater* updater =
      CreateNetworkConfigurationUpdaterForUserPolicy(
          true /* allow trusted certs from policy */,
          true /* set certificate importer */);
  MarkPolicyProviderInitialized();

  base::RunLoop().RunUntilIdle();

  // Certificates with the "Web" trust flag set will be returned.
  net::CertificateList trust_anchors;
  updater->GetWebTrustedCertificates(&trust_anchors);
  EXPECT_EQ(1u, trust_anchors.size());
}

TEST_F(NetworkConfigurationUpdaterTest,
       AllowTrustedCertificatesFromPolicyOnUpdate) {
  // Ignore network configuration changes.
  EXPECT_CALL(network_config_handler_, SetPolicy(_, _, _, _))
      .Times(AnyNumber());

  // Start with an empty certificate list.
  UserNetworkConfigurationUpdater* updater =
      CreateNetworkConfigurationUpdaterForUserPolicy(
          true /* allow trusted certs from policy */,
          true /* set certificate importer */);
  MarkPolicyProviderInitialized();

  FakeWebTrustedCertsObserver observer;
  updater->AddTrustedCertsObserver(&observer);

  base::RunLoop().RunUntilIdle();

  // Verify that the returned certificate list is empty.
  {
    net::CertificateList trust_anchors;
    updater->GetWebTrustedCertificates(&trust_anchors);
    EXPECT_TRUE(trust_anchors.empty());
  }
  EXPECT_TRUE(observer.trust_anchors_.empty());

  // Now use a non-empty certificate list to test the observer notification.
  net::ScopedCERTCertificateList cert_list =
      net::CreateCERTCertificateListFromFile(net::GetTestCertsDirectory(),
                                             "ok_cert.pem",
                                             net::X509Certificate::FORMAT_AUTO);
  ASSERT_EQ(1u, cert_list.size());
  certificate_importer_->SetTrustedCertificatesResult(std::move(cert_list));

  // Change to any non-empty policy, so that updates are triggered. The actual
  // content of the policy is irrelevant.
  PolicyMap policy;
  policy.Set(key::kOpenNetworkConfiguration, POLICY_LEVEL_MANDATORY,
             POLICY_SCOPE_USER, POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(kFakeONC), nullptr);
  UpdateProviderPolicy(policy);
  base::RunLoop().RunUntilIdle();

  // Certificates with the "Web" trust flag set will be returned and forwarded
  // to observers.
  {
    net::CertificateList trust_anchors;
    updater->GetWebTrustedCertificates(&trust_anchors);
    EXPECT_EQ(1u, trust_anchors.size());
  }
  EXPECT_EQ(1u, observer.trust_anchors_.size());

  updater->RemoveTrustedCertsObserver(&observer);
}

TEST_F(NetworkConfigurationUpdaterTest,
       DontImportCertificateBeforeCertificateImporterSet) {
  PolicyMap policy;
  policy.Set(key::kOpenNetworkConfiguration, POLICY_LEVEL_MANDATORY,
             POLICY_SCOPE_USER, POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(kFakeONC), nullptr);
  UpdateProviderPolicy(policy);

  ::onc::ONCSource source = onc::ONC_SOURCE_USER_POLICY;
  EXPECT_CALL(network_config_handler_,
              SetPolicy(source, kFakeUsernameHash,
                        IsEqualTo(GetExpectedFakeNetworkConfigs(source)),
                        IsEqualTo(GetExpectedFakeGlobalNetworkConfig())));

  UserNetworkConfigurationUpdater* updater =
      CreateNetworkConfigurationUpdaterForUserPolicy(
          true /* allow trusted certs from policy */,
          false /* do not set certificate importer */);
  MarkPolicyProviderInitialized();

  Mock::VerifyAndClearExpectations(&network_config_handler_);
  EXPECT_EQ(0u, certificate_importer_->GetAndResetImportCount());

  certificate_importer_->SetExpectedONCCertificates(
      std::move(fake_certificates_));
  certificate_importer_->SetExpectedONCSource(onc::ONC_SOURCE_USER_POLICY);

  ASSERT_TRUE(certificate_importer_owned_);
  updater->SetCertificateImporterForTest(
      std::move(certificate_importer_owned_));
  EXPECT_EQ(1u, certificate_importer_->GetAndResetImportCount());
}

TEST_F(NetworkConfigurationUpdaterTest, ReplaceDeviceOncPlaceholders) {
  PolicyMap policy;
  policy.Set(key::kDeviceOpenNetworkConfiguration, POLICY_LEVEL_MANDATORY,
             POLICY_SCOPE_MACHINE, POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(kFakeONC), nullptr);
  UpdateProviderPolicy(policy);

  ::onc::ONCSource source = onc::ONC_SOURCE_DEVICE_POLICY;
  EXPECT_CALL(network_config_handler_,
              SetPolicy(source, std::string(),
                        IsEqualTo(GetExpectedFakeNetworkConfigs(source)),
                        IsEqualTo(GetExpectedFakeGlobalNetworkConfig())));

  CreateNetworkConfigurationUpdaterForDevicePolicy();
  MarkPolicyProviderInitialized();
}

class NetworkConfigurationUpdaterTestWithParam
    : public NetworkConfigurationUpdaterTest,
      public testing::WithParamInterface<const char*> {
 protected:
  // Returns the currently tested ONC source.
  onc::ONCSource CurrentONCSource() {
    if (GetParam() == key::kOpenNetworkConfiguration)
      return onc::ONC_SOURCE_USER_POLICY;
    DCHECK(GetParam() == key::kDeviceOpenNetworkConfiguration);
    return onc::ONC_SOURCE_DEVICE_POLICY;
  }

  // Returns the expected username hash to push policies to
  // ManagedNetworkConfigurationHandler.
  std::string ExpectedUsernameHash() {
    if (GetParam() == key::kOpenNetworkConfiguration)
      return kFakeUsernameHash;
    return std::string();
  }

  size_t ExpectedImportCertificatesCallCount() {
    if (GetParam() == key::kOpenNetworkConfiguration)
      return 1u;
    return 0u;
  }

  void CreateNetworkConfigurationUpdater() {
    if (GetParam() == key::kOpenNetworkConfiguration) {
      CreateNetworkConfigurationUpdaterForUserPolicy(
          false /* do not allow trusted certs from policy */,
          true /* set certificate importer */);
    } else {
      CreateNetworkConfigurationUpdaterForDevicePolicy();
    }
  }
};

TEST_P(NetworkConfigurationUpdaterTestWithParam, InitialUpdates) {
  PolicyMap policy;
  policy.Set(GetParam(), POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(kFakeONC),
             nullptr);
  UpdateProviderPolicy(policy);

  EXPECT_CALL(
      network_config_handler_,
      SetPolicy(CurrentONCSource(), ExpectedUsernameHash(),
                IsEqualTo(GetExpectedFakeNetworkConfigs(CurrentONCSource())),
                IsEqualTo(GetExpectedFakeGlobalNetworkConfig())));
  certificate_importer_->SetExpectedONCCertificates(
      std::move(fake_certificates_));
  certificate_importer_->SetExpectedONCSource(CurrentONCSource());

  CreateNetworkConfigurationUpdater();
  MarkPolicyProviderInitialized();
  EXPECT_EQ(ExpectedImportCertificatesCallCount(),
            certificate_importer_->GetAndResetImportCount());
}

TEST_P(NetworkConfigurationUpdaterTestWithParam,
       PolicyNotSetBeforePolicyProviderInitialized) {
  PolicyMap policy;
  policy.Set(GetParam(), POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(kFakeONC),
             nullptr);
  UpdateProviderPolicy(policy);

  CreateNetworkConfigurationUpdater();

  Mock::VerifyAndClearExpectations(&network_config_handler_);
  EXPECT_EQ(0u, certificate_importer_->GetAndResetImportCount());

  EXPECT_CALL(
      network_config_handler_,
      SetPolicy(CurrentONCSource(), ExpectedUsernameHash(),
                IsEqualTo(GetExpectedFakeNetworkConfigs(CurrentONCSource())),
                IsEqualTo(GetExpectedFakeGlobalNetworkConfig())));
  certificate_importer_->SetExpectedONCSource(CurrentONCSource());
  certificate_importer_->SetExpectedONCCertificates(
      std::move(fake_certificates_));

  MarkPolicyProviderInitialized();
  EXPECT_EQ(ExpectedImportCertificatesCallCount(),
            certificate_importer_->GetAndResetImportCount());
}

TEST_P(NetworkConfigurationUpdaterTestWithParam,
       PolicyAppliedImmediatelyIfProvidersInitialized) {
  MarkPolicyProviderInitialized();

  PolicyMap policy;
  policy.Set(GetParam(), POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(kFakeONC),
             nullptr);
  UpdateProviderPolicy(policy);

  EXPECT_CALL(
      network_config_handler_,
      SetPolicy(CurrentONCSource(), ExpectedUsernameHash(),
                IsEqualTo(GetExpectedFakeNetworkConfigs(CurrentONCSource())),
                IsEqualTo(GetExpectedFakeGlobalNetworkConfig())));
  certificate_importer_->SetExpectedONCSource(CurrentONCSource());
  certificate_importer_->SetExpectedONCCertificates(
      std::move(fake_certificates_));

  CreateNetworkConfigurationUpdater();

  EXPECT_EQ(ExpectedImportCertificatesCallCount(),
            certificate_importer_->GetAndResetImportCount());
}

TEST_P(NetworkConfigurationUpdaterTestWithParam, PolicyChange) {
  // Ignore the initial updates.
  EXPECT_CALL(network_config_handler_, SetPolicy(_, _, _, _)).Times(AtLeast(1));

  CreateNetworkConfigurationUpdater();
  MarkPolicyProviderInitialized();

  Mock::VerifyAndClearExpectations(&network_config_handler_);
  EXPECT_LE(ExpectedImportCertificatesCallCount(),
            certificate_importer_->GetAndResetImportCount());

  // The Updater should update if policy changes.
  EXPECT_CALL(
      network_config_handler_,
      SetPolicy(CurrentONCSource(), _,
                IsEqualTo(GetExpectedFakeNetworkConfigs(CurrentONCSource())),
                IsEqualTo(GetExpectedFakeGlobalNetworkConfig())));
  certificate_importer_->SetExpectedONCSource(CurrentONCSource());
  certificate_importer_->SetExpectedONCCertificates(
      std::move(fake_certificates_));

  PolicyMap policy;
  policy.Set(GetParam(), POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
             POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(kFakeONC),
             nullptr);
  UpdateProviderPolicy(policy);
  Mock::VerifyAndClearExpectations(&network_config_handler_);
  EXPECT_EQ(ExpectedImportCertificatesCallCount(),
            certificate_importer_->GetAndResetImportCount());

  // Another update is expected if the policy goes away.
  EXPECT_CALL(network_config_handler_,
              SetPolicy(CurrentONCSource(), _, IsEmpty(), IsEmpty()));
  certificate_importer_->SetExpectedONCCertificates(
      std::make_unique<chromeos::onc::OncParsedCertificates>(
          base::ListValue()));

  policy.Erase(GetParam());
  UpdateProviderPolicy(policy);
  EXPECT_EQ(ExpectedImportCertificatesCallCount(),
            certificate_importer_->GetAndResetImportCount());
}

INSTANTIATE_TEST_CASE_P(NetworkConfigurationUpdaterTestWithParamInstance,
                        NetworkConfigurationUpdaterTestWithParam,
                        testing::Values(key::kDeviceOpenNetworkConfiguration,
                                        key::kOpenNetworkConfiguration));

}  // namespace policy
