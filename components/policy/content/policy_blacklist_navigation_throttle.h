// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CONTENT_POLICY_BLACKLIST_NAVIGATION_THROTTLE_H_
#define COMPONENTS_POLICY_CONTENT_POLICY_BLACKLIST_NAVIGATION_THROTTLE_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/policy/core/browser/url_blacklist_manager.h"
#include "content/public/browser/navigation_throttle.h"

// PolicyBlacklistService and PolicyBlacklistFactory provide a way for
// us to access URLBlacklistManager, a policy block list service based on
// the Preference Service. The URLBlacklistManager responses to permission
// changes and is per Profile.  From the PolicyBlacklistNavigationThrottle,
// we access this service to provide information about what we should block.
class PolicyBlacklistService : public KeyedService {
 public:
  explicit PolicyBlacklistService(
      std::unique_ptr<policy::URLBlacklistManager> url_blacklist_manager);
  ~PolicyBlacklistService() override;

  bool IsURLBlocked(const GURL& url) const;
  policy::URLBlacklist::URLBlacklistState GetURLBlacklistState(
      const GURL& url) const;

 private:
  std::unique_ptr<policy::URLBlacklistManager> url_blacklist_manager_;

  DISALLOW_COPY_AND_ASSIGN(PolicyBlacklistService);
};

class PolicyBlacklistFactory : public BrowserContextKeyedServiceFactory {
 public:
  static PolicyBlacklistFactory* GetInstance();
  static PolicyBlacklistService* GetForProfile(
      content::BrowserContext* context);

  // Sets the OverrideBlacklistCallback for the underlying URLBlacklistManager.
  // Must be called before BuildServiceInstanceFor().
  void SetBlacklistOverride(
      policy::URLBlacklistManager::OverrideBlacklistCallback);

 private:
  PolicyBlacklistFactory();
  ~PolicyBlacklistFactory() override;
  friend struct base::DefaultSingletonTraits<PolicyBlacklistFactory>;

  // BrowserContextKeyedServiceFactory implementation
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  // Finds which browser context (if any) to use.
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  policy::URLBlacklistManager::OverrideBlacklistCallback override_blacklist_;

  DISALLOW_COPY_AND_ASSIGN(PolicyBlacklistFactory);
};

// PolicyBlacklistNavigationThrottle provides a simple way to block a navigation
// based on the URLBlacklistManager.
class PolicyBlacklistNavigationThrottle : public content::NavigationThrottle {
 public:
  PolicyBlacklistNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      content::BrowserContext* context);
  ~PolicyBlacklistNavigationThrottle() override;

  // NavigationThrottle overrides.
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;

  const char* GetNameForLogging() override;

 private:
  PolicyBlacklistService* blacklist_service_;
  DISALLOW_COPY_AND_ASSIGN(PolicyBlacklistNavigationThrottle);
};

#endif  // COMPONENTS_POLICY_CONTENT_POLICY_BLACKLIST_NAVIGATION_THROTTLE_H_
