// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/chrome_ssl_host_state_delegate_factory.h"

#include <memory>

#include "base/macros.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/chrome_ssl_host_state_delegate.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace {

class Service : public KeyedService {
 public:
  explicit Service(Profile* profile)
      : decisions_(new ChromeSSLHostStateDelegate(profile)) {}

  ChromeSSLHostStateDelegate* decisions() { return decisions_.get(); }

  void Shutdown() override {}

 private:
  std::unique_ptr<ChromeSSLHostStateDelegate> decisions_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace

// static
ChromeSSLHostStateDelegate* ChromeSSLHostStateDelegateFactory::GetForProfile(
    Profile* profile) {
  return static_cast<Service*>(GetInstance()->GetServiceForBrowserContext(
                                   profile, true))->decisions();
}

// static
ChromeSSLHostStateDelegateFactory*
ChromeSSLHostStateDelegateFactory::GetInstance() {
  return base::Singleton<ChromeSSLHostStateDelegateFactory>::get();
}

ChromeSSLHostStateDelegateFactory::ChromeSSLHostStateDelegateFactory()
    : BrowserContextKeyedServiceFactory(
          "ChromeSSLHostStateDelegate",
          BrowserContextDependencyManager::GetInstance()) {
}

ChromeSSLHostStateDelegateFactory::~ChromeSSLHostStateDelegateFactory() {
}

KeyedService* ChromeSSLHostStateDelegateFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new Service(static_cast<Profile*>(profile));
}

void ChromeSSLHostStateDelegateFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
}

content::BrowserContext*
ChromeSSLHostStateDelegateFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
