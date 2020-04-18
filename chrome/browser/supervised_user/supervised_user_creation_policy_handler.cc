// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_creation_policy_handler.h"

#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"

namespace policy {

SupervisedUserCreationPolicyHandler::SupervisedUserCreationPolicyHandler()
    : TypeCheckingPolicyHandler(key::kSupervisedUserCreationEnabled,
                                base::Value::Type::BOOLEAN) {}

SupervisedUserCreationPolicyHandler::~SupervisedUserCreationPolicyHandler() {}

void SupervisedUserCreationPolicyHandler::ApplyPolicySettings(
    const PolicyMap& policies,
    PrefValueMap* prefs) {
  // If force sign in is enabled, disable supervised user creation regardless.
  const base::Value* force_signin_value =
      policies.GetValue(key::kForceBrowserSignin);
  bool is_force_signin_enabled;
  if (force_signin_value &&
      force_signin_value->GetAsBoolean(&is_force_signin_enabled) &&
      is_force_signin_enabled) {
    prefs->SetBoolean(prefs::kSupervisedUserCreationAllowed, false);
    return;
  }

  const base::Value* creation_value = policies.GetValue(policy_name());
  bool is_creation_enabled;
  if (creation_value && creation_value->GetAsBoolean(&is_creation_enabled)) {
    prefs->SetBoolean(prefs::kSupervisedUserCreationAllowed,
                      is_creation_enabled);
  }
}

}  // namespace policy
