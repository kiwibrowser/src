// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_FACTORY_H_
#define CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_FACTORY_H_

#include <list>
#include <map>
#include <memory>

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_base_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;

}  // namespace base

namespace content {
class BrowserContext;
}

namespace policy {

class ConfigurationPolicyProvider;
class ProfilePolicyConnector;

// Creates ProfilePolicyConnectors for Profiles, which manage the common
// policy providers and other policy components.
// TODO(joaodasilva): convert this class to a proper BCKS once the PrefService,
// which depends on this class, becomes a BCKS too.
class ProfilePolicyConnectorFactory : public BrowserContextKeyedBaseFactory {
 public:
  // Returns the ProfilePolicyConnectorFactory singleton.
  static ProfilePolicyConnectorFactory* GetInstance();

  // Returns the ProfilePolicyConnector associated with |context|. This is only
  // valid before |context| is shut down.
  static ProfilePolicyConnector* GetForBrowserContext(
      content::BrowserContext* context);

  // Creates a new ProfilePolicyConnector for |context|, which must be managed
  // by the caller. Subsequent calls to GetForBrowserContext() will return the
  // instance created, as long as it lives.
  // If |force_immediate_load| is true then policy is loaded synchronously on
  // startup.
  static std::unique_ptr<ProfilePolicyConnector> CreateForBrowserContext(
      content::BrowserContext* context,
      bool force_immediate_load);

  // Returns true if |context| is a managed profile. CHECK-fails if the profile
  // does not have a ProfilePolicyConnector yet (i.e. if
  // ProfilePolicyConnectorFactory::CreateForBrowserContext has not been called
  // for it yet).
  static bool IsProfileManaged(const content::BrowserContext* context);

  // Overrides the |connector| for the given |context|; use only in tests.
  // Once this class becomes a proper BCKS then it can reuse the testing
  // methods of BrowserContextKeyedServiceFactory.
  void SetServiceForTesting(content::BrowserContext* context,
                            ProfilePolicyConnector* connector);

  // The next Profile to call CreateForBrowserContext() will get a PolicyService
  // with |provider| as its sole policy provider. This can be called multiple
  // times to override the policy providers for more than 1 Profile.
  void PushProviderForTesting(ConfigurationPolicyProvider* provider);

 private:
  friend struct base::DefaultSingletonTraits<ProfilePolicyConnectorFactory>;

  ProfilePolicyConnectorFactory();
  ~ProfilePolicyConnectorFactory() override;

  ProfilePolicyConnector* GetForBrowserContextInternal(
      const content::BrowserContext* context) const;

  std::unique_ptr<ProfilePolicyConnector> CreateForBrowserContextInternal(
      content::BrowserContext* context,
      bool force_immediate_load);

  // BrowserContextKeyedBaseFactory:
  void BrowserContextShutdown(content::BrowserContext* context) override;
  void BrowserContextDestroyed(content::BrowserContext* context) override;
  void SetEmptyTestingFactory(content::BrowserContext* context) override;
  bool HasTestingFactory(content::BrowserContext* context) override;
  void CreateServiceNow(content::BrowserContext* context) override;

  typedef std::map<const content::BrowserContext*, ProfilePolicyConnector*>
      ConnectorMap;
  ConnectorMap connectors_;
  std::list<ConfigurationPolicyProvider*> test_providers_;

  DISALLOW_COPY_AND_ASSIGN(ProfilePolicyConnectorFactory);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_FACTORY_H_
