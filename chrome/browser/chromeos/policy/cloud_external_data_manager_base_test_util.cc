// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/cloud_external_data_manager_base_test_util.h"

#include <utility>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/policy/core/common/cloud/cloud_external_data_manager.h"
#include "components/policy/core/common/cloud/cloud_policy_core.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/core/common/external_data_fetcher.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {
namespace test {

void ExternalDataFetchCallback(std::unique_ptr<std::string>* destination,
                               const base::Closure& done_callback,
                               std::unique_ptr<std::string> data) {
  *destination = std::move(data);
  done_callback.Run();
}

std::unique_ptr<base::DictionaryValue> ConstructExternalDataReference(
    const std::string& url,
    const std::string& data) {
  const std::string hash = crypto::SHA256HashString(data);
  std::unique_ptr<base::DictionaryValue> metadata(new base::DictionaryValue);
  metadata->SetKey("url", base::Value(url));
  metadata->SetKey("hash",
                   base::Value(base::HexEncode(hash.c_str(), hash.size())));
  return metadata;
}

void SetExternalDataReference(CloudPolicyCore* core,
                              const std::string& policy,
                              std::unique_ptr<base::DictionaryValue> metadata) {
  CloudPolicyStore* store = core->store();
  ASSERT_TRUE(store);
  PolicyMap policy_map;
  policy_map.Set(policy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::move(metadata),
                 std::make_unique<ExternalDataFetcher>(
                     store->external_data_manager(), policy));
  store->SetPolicyMapForTesting(policy_map);
}

}  // namespace test
}  // namespace policy
