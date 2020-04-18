// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_CLOUD_POLICY_HEADER_SERVICE_H_
#define COMPONENTS_POLICY_CORE_COMMON_CLOUD_POLICY_HEADER_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/policy_export.h"

namespace base {
class SequencedTaskRunner;
}

namespace policy {

class PolicyHeaderIOHelper;

// Per-profile service used to generate PolicyHeaderIOHelper objects, and
// keep them up to date as policy changes.
// TODO(atwilson): Move to components/policy once CloudPolicyStore is moved.
class POLICY_EXPORT PolicyHeaderService : public CloudPolicyStore::Observer {
 public:
  // |device_policy_store| can be null on platforms that do not support
  // device policy. Both |user_policy_store| and |device_policy_store| must
  // outlive this object.
  PolicyHeaderService(const std::string& server_url,
                      const std::string& verification_key_hash,
                      CloudPolicyStore* user_policy_store);
  ~PolicyHeaderService() override;

  // Creates a PolicyHeaderIOHelper object to be run on the IO thread and
  // add policy headers to outgoing requests. The caller takes ownership of
  // this object and must ensure it outlives ProfileHeaderService (in practice,
  // this is called by ProfileIOData, which is shutdown *after* all
  // ProfileKeyedServices are shutdown).
  std::unique_ptr<PolicyHeaderIOHelper> CreatePolicyHeaderIOHelper(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Overridden CloudPolicyStore::Observer methods:
  void OnStoreLoaded(CloudPolicyStore* store) override;
  void OnStoreError(CloudPolicyStore* store) override;

  // Returns a list of all PolicyHeaderIOHelpers created by this object.
  std::vector<PolicyHeaderIOHelper*> GetHelpersForTest();

 private:
  // Generate a policy header based on the currently loaded policy.
  std::string CreateHeaderValue();

  // Weak pointer to created PolicyHeaderIOHelper objects.
  std::vector<PolicyHeaderIOHelper*> helpers_;

  // URL of the policy server.
  std::string server_url_;

  // Identifier for the verification key this Chrome instance is using.
  std::string verification_key_hash_;

  // Weak pointer to the User-level policy store.
  CloudPolicyStore* user_policy_store_;

  DISALLOW_COPY_AND_ASSIGN(PolicyHeaderService);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_CLOUD_POLICY_HEADER_SERVICE_H_
