// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/content/policy_blacklist_navigation_throttle.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"

PolicyBlacklistService::PolicyBlacklistService(
    std::unique_ptr<policy::URLBlacklistManager> url_blacklist_manager)
    : url_blacklist_manager_(std::move(url_blacklist_manager)) {}

PolicyBlacklistService::~PolicyBlacklistService() {}

bool PolicyBlacklistService::IsURLBlocked(const GURL& url) const {
  return url_blacklist_manager_->IsURLBlocked(url);
}

policy::URLBlacklist::URLBlacklistState
PolicyBlacklistService::GetURLBlacklistState(const GURL& url) const {
  return url_blacklist_manager_->GetURLBlacklistState(url);
}

// static
PolicyBlacklistFactory* PolicyBlacklistFactory::GetInstance() {
  return base::Singleton<PolicyBlacklistFactory>::get();
}

// static
PolicyBlacklistService* PolicyBlacklistFactory::GetForProfile(
    content::BrowserContext* context) {
  return static_cast<PolicyBlacklistService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

PolicyBlacklistFactory::PolicyBlacklistFactory()
    : BrowserContextKeyedServiceFactory(
          "PolicyBlacklist",
          BrowserContextDependencyManager::GetInstance()) {}

PolicyBlacklistFactory::~PolicyBlacklistFactory() {}

void PolicyBlacklistFactory::SetBlacklistOverride(
    policy::URLBlacklistManager::OverrideBlacklistCallback callback) {
  override_blacklist_ = callback;
}

KeyedService* PolicyBlacklistFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  PrefService* pref_service = user_prefs::UserPrefs::Get(context);
  auto url_blacklist_manager = std::make_unique<policy::URLBlacklistManager>(
      pref_service, override_blacklist_);
  return new PolicyBlacklistService(std::move(url_blacklist_manager));
}

content::BrowserContext* PolicyBlacklistFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // TODO(crbug.com/701326): This DCHECK should be moved to GetContextToUse().
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return context;
}

PolicyBlacklistNavigationThrottle::PolicyBlacklistNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    content::BrowserContext* context)
    : NavigationThrottle(navigation_handle) {
  blacklist_service_ = PolicyBlacklistFactory::GetForProfile(context);
}

PolicyBlacklistNavigationThrottle::~PolicyBlacklistNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
PolicyBlacklistNavigationThrottle::WillStartRequest() {
  if (blacklist_service_->IsURLBlocked(navigation_handle()->GetURL())) {
    return ThrottleCheckResult(BLOCK_REQUEST,
                               net::ERR_BLOCKED_BY_ADMINISTRATOR);
  }
  return PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
PolicyBlacklistNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

const char* PolicyBlacklistNavigationThrottle::GetNameForLogging() {
  return "PolicyBlacklistNavigationThrottle";
}
