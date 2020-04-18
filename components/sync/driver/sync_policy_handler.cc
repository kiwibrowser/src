// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_policy_handler.h"

#include "base/values.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/sync/base/pref_names.h"

namespace syncer {

SyncPolicyHandler::SyncPolicyHandler()
    : policy::TypeCheckingPolicyHandler(policy::key::kSyncDisabled,
                                        base::Value::Type::BOOLEAN) {}

SyncPolicyHandler::~SyncPolicyHandler() {}

void SyncPolicyHandler::ApplyPolicySettings(const policy::PolicyMap& policies,
                                            PrefValueMap* prefs) {
  const base::Value* value = policies.GetValue(policy_name());
  bool disable_sync;
  if (value && value->GetAsBoolean(&disable_sync) && disable_sync)
    prefs->SetValue(prefs::kSyncManaged, value->CreateDeepCopy());
}

}  // namespace syncer
