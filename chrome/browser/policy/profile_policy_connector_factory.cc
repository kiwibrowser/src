// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/profile_policy_connector_factory.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/schema_registry_service.h"
#include "chrome/browser/policy/schema_registry_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_service_impl.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/active_directory_policy_manager.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/policy/user_policy_manager_factory_chromeos.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "components/user_manager/user.h"
#else  // Non-ChromeOS.
#include "chrome/browser/policy/cloud/user_cloud_policy_manager_factory.h"
#include "components/policy/core/common/cloud/user_cloud_policy_manager.h"
#endif

namespace policy {

// static
ProfilePolicyConnectorFactory* ProfilePolicyConnectorFactory::GetInstance() {
  return base::Singleton<ProfilePolicyConnectorFactory>::get();
}

// static
ProfilePolicyConnector* ProfilePolicyConnectorFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return GetInstance()->GetForBrowserContextInternal(context);
}

// static
std::unique_ptr<ProfilePolicyConnector>
ProfilePolicyConnectorFactory::CreateForBrowserContext(
    content::BrowserContext* context,
    bool force_immediate_load) {
  return GetInstance()->CreateForBrowserContextInternal(context,
                                                        force_immediate_load);
}

// static
bool ProfilePolicyConnectorFactory::IsProfileManaged(
    const content::BrowserContext* context) {
  // Note: GetForBrowserContextInternal CHECK-fails if there is no
  // ProfilePolicyConnector for |context| yet.
  return GetInstance()->GetForBrowserContextInternal(context)->IsManaged();
}

void ProfilePolicyConnectorFactory::SetServiceForTesting(
    content::BrowserContext* context,
    ProfilePolicyConnector* connector) {
  ProfilePolicyConnector*& map_entry = connectors_[context];
  CHECK(!map_entry);
  map_entry = connector;
}

void ProfilePolicyConnectorFactory::PushProviderForTesting(
    ConfigurationPolicyProvider* provider) {
  test_providers_.push_back(provider);
}

ProfilePolicyConnectorFactory::ProfilePolicyConnectorFactory()
    : BrowserContextKeyedBaseFactory(
        "ProfilePolicyConnector",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SchemaRegistryServiceFactory::GetInstance());
#if defined(OS_CHROMEOS)
  DependsOn(UserPolicyManagerFactoryChromeOS::GetInstance());
#else
  DependsOn(UserCloudPolicyManagerFactory::GetInstance());
#endif
}

ProfilePolicyConnectorFactory::~ProfilePolicyConnectorFactory() {
  DCHECK(connectors_.empty());
}

ProfilePolicyConnector*
ProfilePolicyConnectorFactory::GetForBrowserContextInternal(
    const content::BrowserContext* context) const {
  // Get the connector for the original Profile, so that the incognito Profile
  // gets managed settings from the same PolicyService.
  const content::BrowserContext* const original_context =
      chrome::GetBrowserContextRedirectedInIncognito(context);
  const ConnectorMap::const_iterator it = connectors_.find(original_context);
  CHECK(it != connectors_.end());
  return it->second;
}

std::unique_ptr<ProfilePolicyConnector>
ProfilePolicyConnectorFactory::CreateForBrowserContextInternal(
    content::BrowserContext* context,
    bool force_immediate_load) {
  DCHECK(connectors_.find(context) == connectors_.end());

  const user_manager::User* user = nullptr;
  SchemaRegistry* schema_registry =
      SchemaRegistryServiceFactory::GetForContext(context)->registry();

  ConfigurationPolicyProvider* policy_provider = nullptr;
  const CloudPolicyStore* policy_store = nullptr;

#if defined(OS_CHROMEOS)
  Profile* const profile = Profile::FromBrowserContext(context);
  if (!chromeos::ProfileHelper::IsSigninProfile(profile) &&
      !chromeos::ProfileHelper::IsLockScreenAppProfile(profile)) {
    user = chromeos::ProfileHelper::Get()->GetUserByProfile(profile);
    CHECK(user);
  }

  CloudPolicyManager* user_cloud_policy_manager =
      UserPolicyManagerFactoryChromeOS::GetCloudPolicyManagerForProfile(
          profile);
  ActiveDirectoryPolicyManager* active_directory_manager =
      UserPolicyManagerFactoryChromeOS::
          GetActiveDirectoryPolicyManagerForProfile(profile);
  if (user_cloud_policy_manager) {
    policy_provider = user_cloud_policy_manager;
    policy_store = user_cloud_policy_manager->core()->store();
  } else if (active_directory_manager) {
    policy_provider = active_directory_manager;
    policy_store = active_directory_manager->store();
  }
#else
  CloudPolicyManager* user_cloud_policy_manager =
      UserCloudPolicyManagerFactory::GetForBrowserContext(context);
  if (user_cloud_policy_manager) {
    policy_provider = user_cloud_policy_manager;
    policy_store = user_cloud_policy_manager->core()->store();
  }
#endif  // defined(OS_CHROMEOS)

  std::unique_ptr<ProfilePolicyConnector> connector(
      new ProfilePolicyConnector());

  if (test_providers_.empty()) {
    connector->Init(user, schema_registry, policy_provider, policy_store,
                    force_immediate_load);
  } else {
    PolicyServiceImpl::Providers providers;
    providers.push_back(test_providers_.front());
    test_providers_.pop_front();
    std::unique_ptr<PolicyServiceImpl> service =
        std::make_unique<PolicyServiceImpl>(std::move(providers));
    connector->InitForTesting(std::move(service));
  }

  connectors_[context] = connector.get();
  return connector;
}

void ProfilePolicyConnectorFactory::BrowserContextShutdown(
    content::BrowserContext* context) {
  if (Profile::FromBrowserContext(context)->IsOffTheRecord())
    return;
  const ConnectorMap::const_iterator it = connectors_.find(context);
  if (it != connectors_.end())
    it->second->Shutdown();
}

void ProfilePolicyConnectorFactory::BrowserContextDestroyed(
    content::BrowserContext* context) {
  const ConnectorMap::iterator it = connectors_.find(context);
  if (it != connectors_.end())
    connectors_.erase(it);
  BrowserContextKeyedBaseFactory::BrowserContextDestroyed(context);
}

void ProfilePolicyConnectorFactory::SetEmptyTestingFactory(
    content::BrowserContext* context) {}

bool ProfilePolicyConnectorFactory::HasTestingFactory(
    content::BrowserContext* context) {
  return false;
}

void ProfilePolicyConnectorFactory::CreateServiceNow(
    content::BrowserContext* context) {}

}  // namespace policy
