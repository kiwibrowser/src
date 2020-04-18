// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/secondary_google_account_signin_policy_handler.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/signin/core/browser/signin_pref_names.h"

namespace policy {

SecondaryGoogleAccountSigninPolicyHandler::
    SecondaryGoogleAccountSigninPolicyHandler()
    : TypeCheckingPolicyHandler(key::kSecondaryGoogleAccountSigninAllowed,
                                base::Value::Type::BOOLEAN) {}

SecondaryGoogleAccountSigninPolicyHandler::
    ~SecondaryGoogleAccountSigninPolicyHandler() {}

void SecondaryGoogleAccountSigninPolicyHandler::ApplyPolicySettings(
    const PolicyMap& policies,
    PrefValueMap* prefs) {
  const base::Value* value =
      policies.GetValue(key::kSecondaryGoogleAccountSigninAllowed);

  // If the policy is unset, or set to true, do not override the pref value.
  // The default value of the preference is |true| for child accounts and
  // |false| for regular accounts.
  if (!value || value->GetBool()) {
    return;
  }

  // Disallow secondary sign-in by enabling Mirror consistency.
  prefs->SetBoolean(prefs::kAccountConsistencyMirrorRequired, true);
}

}  // namespace policy
