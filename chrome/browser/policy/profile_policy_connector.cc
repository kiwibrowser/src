// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/profile_policy_connector.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/cloud/cloud_policy_core.h"
#include "components/policy/core/common/cloud/cloud_policy_manager.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service_impl.h"
#include "components/policy/core/common/schema_registry_tracking_policy_provider.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/chromeos/policy/active_directory_policy_manager.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/device_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/chromeos/policy/device_local_account_policy_provider.h"
#include "chrome/browser/chromeos/policy/login_profile_policy_provider.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#endif

namespace policy {

ProfilePolicyConnector::ProfilePolicyConnector() {}

ProfilePolicyConnector::~ProfilePolicyConnector() {}

void ProfilePolicyConnector::Init(
    const user_manager::User* user,
    SchemaRegistry* schema_registry,
    ConfigurationPolicyProvider* configuration_policy_provider,
    const CloudPolicyStore* policy_store,
    bool force_immediate_load) {
  configuration_policy_provider_ = configuration_policy_provider;
  policy_store_ = policy_store;

#if defined(OS_CHROMEOS)
  BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
#else
  DCHECK_EQ(nullptr, user);
  ChromeBrowserPolicyConnector* connector =
      g_browser_process->browser_policy_connector();
#endif

  if (connector->GetPlatformProvider()) {
    wrapped_platform_policy_provider_.reset(
        new SchemaRegistryTrackingPolicyProvider(
            connector->GetPlatformProvider()));
    wrapped_platform_policy_provider_->Init(schema_registry);
    policy_providers_.push_back(wrapped_platform_policy_provider_.get());
  }

#if defined(OS_CHROMEOS)
  if (connector->GetDeviceCloudPolicyManager()) {
    policy_providers_.push_back(connector->GetDeviceCloudPolicyManager());
  }
  if (connector->GetDeviceActiveDirectoryPolicyManager()) {
    policy_providers_.push_back(
        connector->GetDeviceActiveDirectoryPolicyManager());
  }
#else
  for (auto* provider : connector->GetPolicyProviders()) {
    // Skip the platform provider since it was already handled above.  The
    // platform provider should be first in the list so that it always takes
    // precedence.
    if (provider == connector->GetPlatformProvider()) {
      continue;
    } else {
      // TODO(zmin): In the future, we may want to have special handling for
      // the other providers too.
      policy_providers_.push_back(provider);
    }
  }
#endif

  if (configuration_policy_provider)
    policy_providers_.push_back(configuration_policy_provider);

#if defined(OS_CHROMEOS)
  if (!user) {
    DCHECK(schema_registry);
    // This case occurs for the signin and the lock screen app profiles.
    special_user_policy_provider_.reset(
        new LoginProfilePolicyProvider(connector->GetPolicyService()));
  } else {
    // |user| should never be nullptr except for the signin and the lock screen
    // app profile.
    is_primary_user_ =
        user == user_manager::UserManager::Get()->GetPrimaryUser();
    // Note that |DeviceLocalAccountPolicyProvider::Create| returns nullptr when
    // the user supplied is not a device-local account user.
    special_user_policy_provider_ = DeviceLocalAccountPolicyProvider::Create(
        user->GetAccountId().GetUserEmail(),
        connector->GetDeviceLocalAccountPolicyService(), force_immediate_load);
  }
  if (special_user_policy_provider_) {
    special_user_policy_provider_->Init(schema_registry);
    policy_providers_.push_back(special_user_policy_provider_.get());
  }
#endif

  policy_service_ = std::make_unique<PolicyServiceImpl>(policy_providers_);

#if defined(OS_CHROMEOS)
  if (is_primary_user_) {
    if (configuration_policy_provider)
      connector->SetUserPolicyDelegate(configuration_policy_provider);
    else if (special_user_policy_provider_)
      connector->SetUserPolicyDelegate(special_user_policy_provider_.get());
  }
#endif
}

void ProfilePolicyConnector::InitForTesting(
    std::unique_ptr<PolicyService> service) {
  policy_service_ = std::move(service);
}

void ProfilePolicyConnector::OverrideIsManagedForTesting(bool is_managed) {
  is_managed_override_.reset(new bool(is_managed));
}

void ProfilePolicyConnector::Shutdown() {
#if defined(OS_CHROMEOS)
  if (is_primary_user_) {
    BrowserPolicyConnectorChromeOS* connector =
        g_browser_process->platform_part()->browser_policy_connector_chromeos();
    connector->SetUserPolicyDelegate(nullptr);
  }
  if (special_user_policy_provider_)
    special_user_policy_provider_->Shutdown();
#endif
  if (wrapped_platform_policy_provider_)
    wrapped_platform_policy_provider_->Shutdown();
}

bool ProfilePolicyConnector::IsManaged() const {
  if (is_managed_override_)
    return *is_managed_override_;
  const CloudPolicyStore* actual_policy_store = GetActualPolicyStore();
  if (actual_policy_store)
    return actual_policy_store->is_managed();
  return false;
}

bool ProfilePolicyConnector::IsProfilePolicy(const char* policy_key) const {
  const ConfigurationPolicyProvider* const provider =
      DeterminePolicyProviderForPolicy(policy_key);
  return provider == configuration_policy_provider_;
}

const CloudPolicyStore* ProfilePolicyConnector::GetActualPolicyStore() const {
  if (policy_store_)
    return policy_store_;
#if defined(OS_CHROMEOS)
  if (special_user_policy_provider_) {
    // |special_user_policy_provider_| is non-null for device-local accounts,
    // for the login profile, and the lock screen app profile.
    const DeviceCloudPolicyManagerChromeOS* const device_cloud_policy_manager =
        g_browser_process->platform_part()
            ->browser_policy_connector_chromeos()
            ->GetDeviceCloudPolicyManager();
    // The device_cloud_policy_manager can be a nullptr in unit tests.
    if (device_cloud_policy_manager)
      return device_cloud_policy_manager->core()->store();
  }
#endif
  return nullptr;
}

const ConfigurationPolicyProvider*
ProfilePolicyConnector::DeterminePolicyProviderForPolicy(
    const char* policy_key) const {
  const PolicyNamespace chrome_ns(POLICY_DOMAIN_CHROME, "");
  for (const ConfigurationPolicyProvider* provider : policy_providers_) {
    if (provider->policies().Get(chrome_ns).Get(policy_key))
      return provider;
  }
  return nullptr;
}

}  // namespace policy
