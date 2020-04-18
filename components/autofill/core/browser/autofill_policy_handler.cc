// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_policy_handler.h"

#include "base/values.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"

namespace autofill {

AutofillPolicyHandler::AutofillPolicyHandler()
    : policy::TypeCheckingPolicyHandler(policy::key::kAutoFillEnabled,
                                        base::Value::Type::BOOLEAN) {}

AutofillPolicyHandler::~AutofillPolicyHandler() {}

void AutofillPolicyHandler::ApplyPolicySettings(
    const policy::PolicyMap& policies,
    PrefValueMap* prefs) {
  const base::Value* value = policies.GetValue(policy_name());
  bool auto_fill_enabled;
  if (value && value->GetAsBoolean(&auto_fill_enabled) && !auto_fill_enabled)
    prefs->SetBoolean(autofill::prefs::kAutofillEnabled, false);
}

}  // namespace autofill
