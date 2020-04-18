// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/saml/saml_offline_signin_limiter_factory.h"

#include "base/time/clock.h"
#include "chrome/browser/chromeos/login/saml/saml_offline_signin_limiter.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace chromeos {

base::Clock* SAMLOfflineSigninLimiterFactory::clock_for_testing_ = NULL;

// static
SAMLOfflineSigninLimiterFactory*
SAMLOfflineSigninLimiterFactory::GetInstance() {
  return base::Singleton<SAMLOfflineSigninLimiterFactory>::get();
}

// static
SAMLOfflineSigninLimiter* SAMLOfflineSigninLimiterFactory::GetForProfile(
    Profile* profile) {
  return static_cast<SAMLOfflineSigninLimiter*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
void SAMLOfflineSigninLimiterFactory::SetClockForTesting(base::Clock* clock) {
  clock_for_testing_ = clock;
}

SAMLOfflineSigninLimiterFactory::SAMLOfflineSigninLimiterFactory()
    : BrowserContextKeyedServiceFactory(
          "SAMLOfflineSigninLimiter",
          BrowserContextDependencyManager::GetInstance()) {}

SAMLOfflineSigninLimiterFactory::~SAMLOfflineSigninLimiterFactory() {}

KeyedService* SAMLOfflineSigninLimiterFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SAMLOfflineSigninLimiter(static_cast<Profile*>(context),
                                      clock_for_testing_);
}

}  // namespace chromeos
