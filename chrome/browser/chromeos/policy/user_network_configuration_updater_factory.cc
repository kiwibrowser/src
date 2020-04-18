// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/user_network_configuration_updater_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/chromeos/policy/user_network_configuration_updater.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/network/network_handler.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"

namespace policy {

// static
UserNetworkConfigurationUpdater*
UserNetworkConfigurationUpdaterFactory::GetForProfile(Profile* profile) {
  return static_cast<UserNetworkConfigurationUpdater*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
UserNetworkConfigurationUpdaterFactory*
UserNetworkConfigurationUpdaterFactory::GetInstance() {
  return base::Singleton<UserNetworkConfigurationUpdaterFactory>::get();
}

UserNetworkConfigurationUpdaterFactory::UserNetworkConfigurationUpdaterFactory()
    : BrowserContextKeyedServiceFactory(
          "UserNetworkConfigurationUpdater",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ProfilePolicyConnectorFactory::GetInstance());
}

UserNetworkConfigurationUpdaterFactory::
    ~UserNetworkConfigurationUpdaterFactory() {}

content::BrowserContext*
UserNetworkConfigurationUpdaterFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool
UserNetworkConfigurationUpdaterFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool UserNetworkConfigurationUpdaterFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

KeyedService* UserNetworkConfigurationUpdaterFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // On the login/lock screen only device network policies apply.
  Profile* profile = Profile::FromBrowserContext(context);
  if (chromeos::ProfileHelper::IsSigninProfile(profile) ||
      chromeos::ProfileHelper::IsLockScreenAppProfile(profile)) {
    return nullptr;
  }

  const user_manager::User* user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(profile);
  DCHECK(user);
  // Currently, only the network policy of the primary user is supported. See
  // also http://crbug.com/310685 .
  if (user != user_manager::UserManager::Get()->GetPrimaryUser())
    return nullptr;

  // Guest sessions don't get user policy, but a UserNetworkConfigurationUpdater
  // can be created for them anyway.
  const bool allow_trusted_certs_from_policy =
      user->GetType() != user_manager::USER_TYPE_GUEST;

  ProfilePolicyConnector* profile_connector =
      ProfilePolicyConnectorFactory::GetForBrowserContext(context);

  return UserNetworkConfigurationUpdater::CreateForUserPolicy(
             profile, allow_trusted_certs_from_policy, *user,
             profile_connector->policy_service(),
             chromeos::NetworkHandler::Get()
                 ->managed_network_configuration_handler())
      .release();
}

}  // namespace policy
