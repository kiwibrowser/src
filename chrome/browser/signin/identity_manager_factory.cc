// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/identity_manager_factory.h"

#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "services/identity/public/cpp/identity_manager.h"

// Class that wraps IdentityManager in a KeyedService (as IdentityManager is a
// client-side library intended for use by any process, it would be a layering
// violation for IdentityManager itself to have direct knowledge of
// KeyedService).
class IdentityManagerHolder : public KeyedService {
 public:
  explicit IdentityManagerHolder(Profile* profile)
      : identity_manager_(
            SigninManagerFactory::GetForProfile(profile),
            ProfileOAuth2TokenServiceFactory::GetForProfile(profile)) {}

  identity::IdentityManager* identity_manager() { return &identity_manager_; }

 private:
  identity::IdentityManager identity_manager_;
};

IdentityManagerFactory::IdentityManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "IdentityManager",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
  DependsOn(SigninManagerFactory::GetInstance());
}

IdentityManagerFactory::~IdentityManagerFactory() {}

// static
identity::IdentityManager* IdentityManagerFactory::GetForProfile(
    Profile* profile) {
  IdentityManagerHolder* holder = static_cast<IdentityManagerHolder*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));

  return holder->identity_manager();
}

// static
IdentityManagerFactory* IdentityManagerFactory::GetInstance() {
  return base::Singleton<IdentityManagerFactory>::get();
}

KeyedService* IdentityManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new IdentityManagerHolder(Profile::FromBrowserContext(context));
}
