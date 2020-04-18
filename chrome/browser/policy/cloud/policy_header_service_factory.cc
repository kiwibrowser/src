// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/cloud/policy_header_service_factory.h"

#include <memory>
#include <utility>

#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "components/policy/core/common/cloud/policy_header_service.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/active_directory_policy_manager.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/policy/user_policy_manager_factory_chromeos.h"
#else
#include "chrome/browser/policy/cloud/user_cloud_policy_manager_factory.h"
#include "components/policy/core/common/cloud/user_cloud_policy_manager.h"
#endif

namespace policy {

namespace {

class PolicyHeaderServiceWrapper : public KeyedService {
 public:
  explicit PolicyHeaderServiceWrapper(
      std::unique_ptr<PolicyHeaderService> service)
      : policy_header_service_(std::move(service)) {}

  PolicyHeaderService* policy_header_service() const {
    return policy_header_service_.get();
  }

  void Shutdown() override {
    // Shutdown our core object so it can unregister any observers before the
    // services we depend on are shutdown.
    policy_header_service_.reset();
  }

 private:
  std::unique_ptr<PolicyHeaderService> policy_header_service_;
};

}  // namespace

PolicyHeaderServiceFactory::PolicyHeaderServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "PolicyHeaderServiceFactory",
        BrowserContextDependencyManager::GetInstance()) {
#if defined(OS_CHROMEOS)
  DependsOn(UserPolicyManagerFactoryChromeOS::GetInstance());
#else
  DependsOn(UserCloudPolicyManagerFactory::GetInstance());
#endif
}

PolicyHeaderServiceFactory::~PolicyHeaderServiceFactory() {
}

// static
PolicyHeaderService* PolicyHeaderServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  PolicyHeaderServiceWrapper* wrapper =
      static_cast<PolicyHeaderServiceWrapper*>(
          GetInstance()->GetServiceForBrowserContext(context, true));
  if (wrapper)
    return wrapper->policy_header_service();
  else
    return NULL;
}

KeyedService* PolicyHeaderServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
#if defined(OS_CHROMEOS)
  BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
#else
  BrowserPolicyConnector* connector =
      g_browser_process->browser_policy_connector();
#endif

  CloudPolicyStore* user_store;
#if defined(OS_CHROMEOS)
  Profile* profile = Profile::FromBrowserContext(context);
  CloudPolicyManager* cloud_manager =
      UserPolicyManagerFactoryChromeOS::GetCloudPolicyManagerForProfile(
          profile);
  if (cloud_manager) {
    user_store = cloud_manager->core()->store();
  } else {
    ActiveDirectoryPolicyManager* active_directory_manager =
        UserPolicyManagerFactoryChromeOS::
            GetActiveDirectoryPolicyManagerForProfile(profile);
    if (!active_directory_manager)
      return nullptr;
    user_store = active_directory_manager->store();
  }
#else
  CloudPolicyManager* manager =
      UserCloudPolicyManagerFactory::GetForBrowserContext(context);
  if (!manager)
    return nullptr;
  user_store = manager->core()->store();
#endif

  std::unique_ptr<PolicyHeaderService> service =
      std::make_unique<PolicyHeaderService>(
          connector->device_management_service()->GetServerUrl(),
          kPolicyVerificationKeyHash, user_store);
  return new PolicyHeaderServiceWrapper(std::move(service));
}

// static
PolicyHeaderServiceFactory* PolicyHeaderServiceFactory::GetInstance() {
  return base::Singleton<PolicyHeaderServiceFactory>::get();
}

}  // namespace policy
