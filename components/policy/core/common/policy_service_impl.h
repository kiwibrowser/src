// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_POLICY_SERVICE_IMPL_H_
#define COMPONENTS_POLICY_CORE_COMMON_POLICY_SERVICE_IMPL_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "components/policy/core/common/configuration_policy_provider.h"
#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/policy_export.h"

namespace policy {

class PolicyMap;

class POLICY_EXPORT PolicyServiceImpl
    : public PolicyService,
      public ConfigurationPolicyProvider::Observer {
 public:
  using Providers = std::vector<ConfigurationPolicyProvider*>;

  // Creates a new PolicyServiceImpl with the list of
  // ConfigurationPolicyProviders, in order of decreasing priority.
  explicit PolicyServiceImpl(Providers providers);
  ~PolicyServiceImpl() override;

  // PolicyService overrides:
  void AddObserver(PolicyDomain domain,
                   PolicyService::Observer* observer) override;
  void RemoveObserver(PolicyDomain domain,
                      PolicyService::Observer* observer) override;
  const PolicyMap& GetPolicies(const PolicyNamespace& ns) const override;
  bool IsInitializationComplete(PolicyDomain domain) const override;
  void RefreshPolicies(const base::Closure& callback) override;

 private:
  using Observers = base::ObserverList<PolicyService::Observer, true>;

  // ConfigurationPolicyProvider::Observer overrides:
  void OnUpdatePolicy(ConfigurationPolicyProvider* provider) override;

  // Posts a task to notify observers of |ns| that its policies have changed,
  // passing along the |previous| and the |current| policies.
  void NotifyNamespaceUpdated(const PolicyNamespace& ns,
                              const PolicyMap& previous,
                              const PolicyMap& current);

  // Combines the policies from all the providers, and notifies the observers
  // of namespaces whose policies have been modified.
  void MergeAndTriggerUpdates();

  // Checks if all providers are initialized, and notifies the observers
  // if the service just became initialized.
  void CheckInitializationComplete();

  // Invokes all the refresh callbacks if there are no more refreshes pending.
  void CheckRefreshComplete();

  // The providers, in order of decreasing priority.
  Providers providers_;

  // Maps each policy namespace to its current policies.
  PolicyBundle policy_bundle_;

  // Maps each policy domain to its observer list.
  std::map<PolicyDomain, std::unique_ptr<Observers>> observers_;

  // True if all the providers are initialized for the indexed policy domain.
  bool initialization_complete_[POLICY_DOMAIN_SIZE];

  // Set of providers that have a pending update that was triggered by a
  // call to RefreshPolicies().
  std::set<ConfigurationPolicyProvider*> refresh_pending_;

  // List of callbacks to invoke once all providers refresh after a
  // RefreshPolicies() call.
  std::vector<base::Closure> refresh_callbacks_;

  // Used to verify thread-safe usage.
  base::ThreadChecker thread_checker_;

  // Used to create tasks to delay new policy updates while we may be already
  // processing previous policy updates.
  base::WeakPtrFactory<PolicyServiceImpl> update_task_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PolicyServiceImpl);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_POLICY_SERVICE_IMPL_H_
