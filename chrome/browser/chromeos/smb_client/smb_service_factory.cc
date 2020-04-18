// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_service_factory.h"

#include "chrome/browser/chromeos/authpolicy/auth_policy_credentials_manager.h"
#include "chrome/browser/chromeos/file_system_provider/service_factory.h"
#include "chrome/browser/chromeos/smb_client/smb_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace chromeos {
namespace smb_client {

SmbService* SmbServiceFactory::Get(content::BrowserContext* context) {
  return static_cast<SmbService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

SmbService* SmbServiceFactory::FindExisting(content::BrowserContext* context) {
  return static_cast<SmbService*>(
      GetInstance()->GetServiceForBrowserContext(context, false));
}

SmbServiceFactory* SmbServiceFactory::GetInstance() {
  return base::Singleton<SmbServiceFactory>::get();
}

SmbServiceFactory::SmbServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SmbService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(file_system_provider::ServiceFactory::GetInstance());
  DependsOn(AuthPolicyCredentialsManagerFactory::GetInstance());
}

SmbServiceFactory::~SmbServiceFactory() {}

bool SmbServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

KeyedService* SmbServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new SmbService(Profile::FromBrowserContext(profile));
}

content::BrowserContext* SmbServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace smb_client
}  // namespace chromeos
