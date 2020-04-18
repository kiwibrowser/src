// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/local_sync_policy_handler.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/values.h"
#include "chrome/browser/policy/policy_path_parser.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/sync/base/pref_names.h"

namespace policy {

LocalSyncPolicyHandler::LocalSyncPolicyHandler()
    : TypeCheckingPolicyHandler(key::kRoamingProfileLocation,
                                base::Value::Type::STRING) {}

LocalSyncPolicyHandler::~LocalSyncPolicyHandler() {}

void LocalSyncPolicyHandler::ApplyPolicySettings(const PolicyMap& policies,
                                                 PrefValueMap* prefs) {
  const base::Value* value = policies.GetValue(policy_name());
  base::FilePath::StringType string_value;
  if (value && value->GetAsString(&string_value)) {
    base::FilePath::StringType expanded_value =
        policy::path_parser::ExpandPathVariables(string_value);
    prefs->SetValue(syncer::prefs::kLocalSyncBackendDir,
                    std::make_unique<base::Value>(expanded_value));
  }
}

}  // namespace policy
