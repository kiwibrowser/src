// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/attestation/attestation_ca_client.h"
#include "chrome/browser/chromeos/policy/active_directory_policy_manager.h"
#include "chrome/browser/chromeos/policy/affiliated_cloud_policy_invalidator.h"
#include "chrome/browser/chromeos/policy/affiliated_invalidation_service_provider.h"
#include "chrome/browser/chromeos/policy/affiliated_invalidation_service_provider_impl.h"
#include "chrome/browser/chromeos/policy/bluetooth_policy_handler.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_initializer.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_store_chromeos.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/chromeos/policy/device_local_account_policy_service.h"
#include "chrome/browser/chromeos/policy/device_network_configuration_updater.h"
#include "chrome/browser/chromeos/policy/enrollment_config.h"
#include "chrome/browser/chromeos/policy/hostname_handler.h"
#include "chrome/browser/chromeos/policy/minimum_version_policy_handler.h"
#include "chrome/browser/chromeos/policy/remote_commands/affiliated_remote_commands_invalidator.h"
#include "chrome/browser/chromeos/policy/server_backed_state_keys_broker.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chrome/browser/chromeos/settings/install_attributes.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chrome/browser/policy/device_management_service_configuration.h"
#include "chrome/common/pref_names.h"
#include "chromeos/attestation/attestation_flow.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/cryptohome/async_method_caller.h"
#include "chromeos/cryptohome/system_salt_getter.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/upstart_client.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/onc/onc_certificate_importer_impl.h"
#include "chromeos/settings/cros_settings_names.h"
#include "chromeos/settings/cros_settings_provider.h"
#include "chromeos/settings/timezone_settings.h"
#include "chromeos/system/statistics_provider.h"
#include "components/policy/core/common/cloud/cloud_policy_client.h"
#include "components/policy/core/common/cloud/cloud_policy_refresh_scheduler.h"
#include "components/policy/core/common/proxy_policy_provider.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "net/url_request/url_request_context_getter.h"

using content::BrowserThread;

namespace policy {

namespace {

// Install attributes for tests.
chromeos::InstallAttributes* g_testing_install_attributes = nullptr;

// Helper that returns a new BACKGROUND SequencedTaskRunner. Each
// SequencedTaskRunner returned is independent from the others.
scoped_refptr<base::SequencedTaskRunner> GetBackgroundTaskRunner() {
  return base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

}  // namespace

BrowserPolicyConnectorChromeOS::BrowserPolicyConnectorChromeOS()
    : weak_ptr_factory_(this) {
  if (g_testing_install_attributes) {
    install_attributes_.reset(g_testing_install_attributes);
    g_testing_install_attributes = nullptr;
  }

  // SystemSaltGetter, DBusThreadManager or DeviceSettingsService may be
  // uninitialized on unit tests.

  // TODO(satorux): Remove SystemSaltGetter::IsInitialized() when it's ready
  // (removing it now breaks tests). crbug.com/141016.
  if (chromeos::SystemSaltGetter::IsInitialized() &&
      chromeos::DBusThreadManager::IsInitialized() &&
      chromeos::DeviceSettingsService::IsInitialized()) {
    // Don't initialize install attributes if g_testing_install_attributes have
    // been injected.
    if (!install_attributes_) {
      install_attributes_ = std::make_unique<chromeos::InstallAttributes>(
          chromeos::DBusThreadManager::Get()->GetCryptohomeClient());
      base::FilePath install_attrs_file;
      CHECK(base::PathService::Get(chromeos::FILE_INSTALL_ATTRIBUTES,
                                   &install_attrs_file));
      install_attributes_->Init(install_attrs_file);
    }

    std::unique_ptr<DeviceCloudPolicyStoreChromeOS> device_cloud_policy_store =
        std::make_unique<DeviceCloudPolicyStoreChromeOS>(
            chromeos::DeviceSettingsService::Get(), install_attributes_.get(),
            GetBackgroundTaskRunner());

    if (install_attributes_->IsActiveDirectoryManaged()) {
      chromeos::DBusThreadManager::Get()
          ->GetUpstartClient()
          ->StartAuthPolicyService();

      device_active_directory_policy_manager_ =
          new DeviceActiveDirectoryPolicyManager(
              std::move(device_cloud_policy_store));
      providers_for_init_.push_back(
          base::WrapUnique<ConfigurationPolicyProvider>(
              device_active_directory_policy_manager_));
    } else {
      state_keys_broker_ = std::make_unique<ServerBackedStateKeysBroker>(
          chromeos::DBusThreadManager::Get()->GetSessionManagerClient());

      device_cloud_policy_manager_ = new DeviceCloudPolicyManagerChromeOS(
          std::move(device_cloud_policy_store),
          base::ThreadTaskRunnerHandle::Get(), state_keys_broker_.get());
      providers_for_init_.push_back(
          base::WrapUnique<ConfigurationPolicyProvider>(
              device_cloud_policy_manager_));
    }
  }

  global_user_cloud_policy_provider_ = new ProxyPolicyProvider();
  providers_for_init_.push_back(std::unique_ptr<ConfigurationPolicyProvider>(
      global_user_cloud_policy_provider_));
}

BrowserPolicyConnectorChromeOS::~BrowserPolicyConnectorChromeOS() {}

void BrowserPolicyConnectorChromeOS::Init(
    PrefService* local_state,
    scoped_refptr<net::URLRequestContextGetter> request_context) {
  local_state_ = local_state;
  ChromeBrowserPolicyConnector::Init(local_state, request_context);

  affiliated_invalidation_service_provider_ =
      std::make_unique<AffiliatedInvalidationServiceProviderImpl>();

  if (device_cloud_policy_manager_) {
    // Note: for now the |device_cloud_policy_manager_| is using the global
    // schema registry. Eventually it will have its own registry, once device
    // cloud policy for extensions is introduced. That means it'd have to be
    // initialized from here instead of BrowserPolicyConnector::Init().

    device_cloud_policy_manager_->Initialize(local_state);
    device_cloud_policy_manager_->AddDeviceCloudPolicyManagerObserver(this);
    RestartDeviceCloudPolicyInitializer();
  }

  DCHECK(install_attributes_);
  if (!install_attributes_->IsActiveDirectoryManaged()) {
    device_local_account_policy_service_ =
        std::make_unique<DeviceLocalAccountPolicyService>(
            chromeos::DBusThreadManager::Get()->GetSessionManagerClient(),
            chromeos::DeviceSettingsService::Get(),
            chromeos::CrosSettings::Get(),
            affiliated_invalidation_service_provider_.get(),
            GetBackgroundTaskRunner(), GetBackgroundTaskRunner(),
            GetBackgroundTaskRunner(),
            content::BrowserThread::GetTaskRunnerForThread(
                content::BrowserThread::IO),
            request_context);
    device_local_account_policy_service_->Connect(device_management_service());
  }

  if (device_cloud_policy_manager_) {
    device_cloud_policy_invalidator_ =
        std::make_unique<AffiliatedCloudPolicyInvalidator>(
            enterprise_management::DeviceRegisterRequest::DEVICE,
            device_cloud_policy_manager_->core(),
            affiliated_invalidation_service_provider_.get());
    device_remote_commands_invalidator_ =
        std::make_unique<AffiliatedRemoteCommandsInvalidator>(
            device_cloud_policy_manager_->core(),
            affiliated_invalidation_service_provider_.get());
  }

  SetTimezoneIfPolicyAvailable();

  device_network_configuration_updater_ =
      DeviceNetworkConfigurationUpdater::CreateForDevicePolicy(
          GetPolicyService(),
          chromeos::NetworkHandler::Get()
              ->managed_network_configuration_handler(),
          chromeos::NetworkHandler::Get()->network_device_handler(),
          chromeos::CrosSettings::Get(),
          DeviceNetworkConfigurationUpdater::DeviceAssetIDFetcher());

  bluetooth_policy_handler_ =
      std::make_unique<BluetoothPolicyHandler>(chromeos::CrosSettings::Get());

  hostname_handler_ =
      std::make_unique<HostnameHandler>(chromeos::CrosSettings::Get());

  minimum_version_policy_handler_ =
      std::make_unique<MinimumVersionPolicyHandler>(
          chromeos::CrosSettings::Get());
}

void BrowserPolicyConnectorChromeOS::PreShutdown() {
  // Let the |affiliated_invalidation_service_provider_| unregister itself as an
  // observer of per-Profile InvalidationServices and the device-global
  // invalidation::TiclInvalidationService it may have created as an observer of
  // the DeviceOAuth2TokenService that is destroyed before Shutdown() is called.
  if (affiliated_invalidation_service_provider_)
    affiliated_invalidation_service_provider_->Shutdown();
}

void BrowserPolicyConnectorChromeOS::Shutdown() {
  device_network_configuration_updater_.reset();

  if (device_local_account_policy_service_)
    device_local_account_policy_service_->Shutdown();

  if (device_cloud_policy_initializer_)
    device_cloud_policy_initializer_->Shutdown();

  if (device_cloud_policy_manager_)
    device_cloud_policy_manager_->RemoveDeviceCloudPolicyManagerObserver(this);

  if (hostname_handler_)
    hostname_handler_->Shutdown();

  ChromeBrowserPolicyConnector::Shutdown();
}

bool BrowserPolicyConnectorChromeOS::IsEnterpriseManaged() const {
  return install_attributes_ && install_attributes_->IsEnterpriseManaged();
}

bool BrowserPolicyConnectorChromeOS::IsCloudManaged() const {
  return install_attributes_ && install_attributes_->IsCloudManaged();
}

bool BrowserPolicyConnectorChromeOS::IsActiveDirectoryManaged() const {
  return install_attributes_ && install_attributes_->IsActiveDirectoryManaged();
}

std::string BrowserPolicyConnectorChromeOS::GetEnterpriseEnrollmentDomain()
    const {
  return install_attributes_ ? install_attributes_->GetDomain() : std::string();
}

std::string BrowserPolicyConnectorChromeOS::GetEnterpriseDisplayDomain() const {
  if (device_cloud_policy_manager_) {
    const enterprise_management::PolicyData* policy =
        device_cloud_policy_manager_->device_store()->policy();
    if (policy && policy->has_display_domain())
      return policy->display_domain();
  }
  return GetEnterpriseEnrollmentDomain();
}

std::string BrowserPolicyConnectorChromeOS::GetRealm() const {
  return install_attributes_ ? install_attributes_->GetRealm() : std::string();
}

std::string BrowserPolicyConnectorChromeOS::GetDeviceAssetID() const {
  if (device_cloud_policy_manager_) {
    const enterprise_management::PolicyData* policy =
        device_cloud_policy_manager_->device_store()->policy();
    if (policy && policy->has_annotated_asset_id())
      return policy->annotated_asset_id();
  }
  return std::string();
}

std::string BrowserPolicyConnectorChromeOS::GetDeviceAnnotatedLocation() const {
  if (device_cloud_policy_manager_) {
    const enterprise_management::PolicyData* policy =
        device_cloud_policy_manager_->device_store()->policy();
    if (policy && policy->has_annotated_location())
      return policy->annotated_location();
  }
  return std::string();
}

std::string BrowserPolicyConnectorChromeOS::GetDirectoryApiID() const {
  if (device_cloud_policy_manager_) {
    const enterprise_management::PolicyData* policy =
        device_cloud_policy_manager_->device_store()->policy();
    if (policy && policy->has_directory_api_id())
      return policy->directory_api_id();
  }
  return std::string();
}

DeviceMode BrowserPolicyConnectorChromeOS::GetDeviceMode() const {
  return install_attributes_ ? install_attributes_->GetMode()
                             : DEVICE_MODE_NOT_SET;
}

EnrollmentConfig BrowserPolicyConnectorChromeOS::GetPrescribedEnrollmentConfig()
    const {
  if (device_cloud_policy_initializer_)
    return device_cloud_policy_initializer_->GetPrescribedEnrollmentConfig();

  return EnrollmentConfig();
}

void BrowserPolicyConnectorChromeOS::SetUserPolicyDelegate(
    ConfigurationPolicyProvider* user_policy_provider) {
  global_user_cloud_policy_provider_->SetDelegate(user_policy_provider);
}

void BrowserPolicyConnectorChromeOS::SetDeviceCloudPolicyInitializerForTesting(
    std::unique_ptr<DeviceCloudPolicyInitializer> initializer) {
  device_cloud_policy_initializer_ = std::move(initializer);
}

// static
void BrowserPolicyConnectorChromeOS::SetInstallAttributesForTesting(
    chromeos::InstallAttributes* attributes) {
  DCHECK(!g_testing_install_attributes);
  g_testing_install_attributes = attributes;
}

// static
void BrowserPolicyConnectorChromeOS::RemoveInstallAttributesForTesting() {
  if (g_testing_install_attributes) {
    delete g_testing_install_attributes;
    g_testing_install_attributes = nullptr;
  }
}

// static
void BrowserPolicyConnectorChromeOS::RegisterPrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(
      prefs::kDevicePolicyRefreshRate,
      CloudPolicyRefreshScheduler::kDefaultRefreshDelayMs);
}

void BrowserPolicyConnectorChromeOS::OnDeviceCloudPolicyManagerConnected() {
  CHECK(device_cloud_policy_initializer_);

  // DeviceCloudPolicyInitializer might still be on the call stack, so we
  // should release the initializer after this function returns.
  device_cloud_policy_initializer_->Shutdown();
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(
      FROM_HERE, device_cloud_policy_initializer_.release());
}

void BrowserPolicyConnectorChromeOS::OnDeviceCloudPolicyManagerDisconnected() {
  DCHECK(!device_cloud_policy_initializer_);

  RestartDeviceCloudPolicyInitializer();
}

std::vector<std::unique_ptr<policy::ConfigurationPolicyProvider>>
BrowserPolicyConnectorChromeOS::CreatePolicyProviders() {
  auto providers = ChromeBrowserPolicyConnector::CreatePolicyProviders();
  for (auto& provider_ptr : providers_for_init_)
    providers.push_back(std::move(provider_ptr));
  providers_for_init_.clear();
  return providers;
}

void BrowserPolicyConnectorChromeOS::SetTimezoneIfPolicyAvailable() {
  typedef chromeos::CrosSettingsProvider Provider;
  Provider::TrustedStatus result =
      chromeos::CrosSettings::Get()->PrepareTrustedValues(base::Bind(
          &BrowserPolicyConnectorChromeOS::SetTimezoneIfPolicyAvailable,
          weak_ptr_factory_.GetWeakPtr()));

  if (result != Provider::TRUSTED)
    return;

  std::string timezone;
  if (chromeos::CrosSettings::Get()->GetString(chromeos::kSystemTimezonePolicy,
                                               &timezone) &&
      !timezone.empty()) {
    chromeos::system::SetSystemAndSigninScreenTimezone(timezone);
  }
}

void BrowserPolicyConnectorChromeOS::RestartDeviceCloudPolicyInitializer() {
  device_cloud_policy_initializer_ =
      std::make_unique<DeviceCloudPolicyInitializer>(
          local_state_, device_management_service(), GetBackgroundTaskRunner(),
          install_attributes_.get(), state_keys_broker_.get(),
          device_cloud_policy_manager_->device_store(),
          device_cloud_policy_manager_,
          cryptohome::AsyncMethodCaller::GetInstance(), CreateAttestationFlow(),
          chromeos::system::StatisticsProvider::GetInstance());
  device_cloud_policy_initializer_->Init();
}

std::unique_ptr<chromeos::attestation::AttestationFlow>
BrowserPolicyConnectorChromeOS::CreateAttestationFlow() {
  return std::make_unique<chromeos::attestation::AttestationFlow>(
      cryptohome::AsyncMethodCaller::GetInstance(),
      chromeos::DBusThreadManager::Get()->GetCryptohomeClient(),
      std::make_unique<chromeos::attestation::AttestationCAClient>());
}

chromeos::AffiliationIDSet
BrowserPolicyConnectorChromeOS::GetDeviceAffiliationIDs() const {
  chromeos::AffiliationIDSet affiliation_ids;
  if (device_cloud_policy_manager_) {
    const enterprise_management::PolicyData* const policy_data =
        device_cloud_policy_manager_->device_store()->policy();
    if (policy_data) {
      affiliation_ids.insert(policy_data->device_affiliation_ids().begin(),
                             policy_data->device_affiliation_ids().end());
    }
  }
  return affiliation_ids;
}

}  // namespace policy
