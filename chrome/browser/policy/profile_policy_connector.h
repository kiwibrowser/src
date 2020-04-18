// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_
#define CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"

namespace user_manager {
class User;
}

namespace policy {

class CloudPolicyStore;
class ConfigurationPolicyProvider;
class PolicyService;
class SchemaRegistry;

// A KeyedService that creates and manages the per-Profile policy components.
class ProfilePolicyConnector : public KeyedService {
 public:
  ProfilePolicyConnector();
  ~ProfilePolicyConnector() override;

  // |user| is only used in Chrome OS builds and should be set to nullptr
  // otherwise.  |configuration_policy_provider| and |policy_store| are nullptr
  // for non-regular users.
  // If |force_immediate_load| is true, DeviceLocalAccountPolicy is loaded
  // synchronously.
  void Init(const user_manager::User* user,
            SchemaRegistry* schema_registry,
            ConfigurationPolicyProvider* configuration_policy_provider,
            const CloudPolicyStore* policy_store,
            bool force_immediate_load);

  void InitForTesting(std::unique_ptr<PolicyService> service);
  void OverrideIsManagedForTesting(bool is_managed);

  // KeyedService:
  void Shutdown() override;

  // This is never NULL.
  PolicyService* policy_service() const { return policy_service_.get(); }

  // Returns true if this Profile is under any kind of policy management. You
  // must call this method only when the policies system is fully initialized.
  bool IsManaged() const;


  // Returns true if the |policy_key| user policy is currently set via the
  // |configuration_policy_provider_| and isn't being overridden by a
  // higher-level provider.
  bool IsProfilePolicy(const char* policy_key) const;

 private:
  // Returns the policy store which is actually used.
  const CloudPolicyStore* GetActualPolicyStore() const;

  // Find the policy provider that provides the |policy_key| policy, if any. In
  // case of multiple providers sharing the same policy, the one with the
  // highest priority will be returned.
  const ConfigurationPolicyProvider* DeterminePolicyProviderForPolicy(
      const char* policy_key) const;

#if defined(OS_CHROMEOS)
  // Some of the user policy configuration affects browser global state, and
  // can only come from one Profile. |is_primary_user_| is true if this
  // connector belongs to the first signed-in Profile, and in that case that
  // Profile's policy is the one that affects global policy settings in
  // local state.
  bool is_primary_user_ = false;

  std::unique_ptr<ConfigurationPolicyProvider> special_user_policy_provider_;
#endif  // defined(OS_CHROMEOS)

  std::unique_ptr<ConfigurationPolicyProvider>
      wrapped_platform_policy_provider_;
  const ConfigurationPolicyProvider* configuration_policy_provider_ = nullptr;
  const CloudPolicyStore* policy_store_ = nullptr;

  // |policy_providers_| contains a list of the policy providers available for
  // the PolicyService of this connector, in decreasing order of priority.
  //
  // Note: All the providers appended to this vector must eventually become
  // initialized for every policy domain, otherwise some subsystems will never
  // use the policies exposed by the PolicyService!
  // The default ConfigurationPolicyProvider::IsInitializationComplete()
  // result is true, so take care if a provider overrides that.
  std::vector<ConfigurationPolicyProvider*> policy_providers_;

  std::unique_ptr<PolicyService> policy_service_;
  std::unique_ptr<bool> is_managed_override_;

  DISALLOW_COPY_AND_ASSIGN(ProfilePolicyConnector);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_
