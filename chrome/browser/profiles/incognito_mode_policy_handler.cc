// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/incognito_mode_policy_handler.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/common/pref_names.h"
#include "components/policy/core/browser/policy_error_map.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/strings/grit/components_strings.h"

namespace policy {

IncognitoModePolicyHandler::IncognitoModePolicyHandler() {}

IncognitoModePolicyHandler::~IncognitoModePolicyHandler() {}

bool IncognitoModePolicyHandler::CheckPolicySettings(const PolicyMap& policies,
                                                     PolicyErrorMap* errors) {
  const base::Value* availability =
      policies.GetValue(key::kIncognitoModeAvailability);
  if (availability) {
    int int_value = IncognitoModePrefs::ENABLED;
    if (!availability->GetAsInteger(&int_value)) {
      errors->AddError(key::kIncognitoModeAvailability, IDS_POLICY_TYPE_ERROR,
                       base::Value::GetTypeName(base::Value::Type::INTEGER));
      return false;
    }
    IncognitoModePrefs::Availability availability_enum_value;
    if (!IncognitoModePrefs::IntToAvailability(int_value,
                                               &availability_enum_value)) {
      errors->AddError(key::kIncognitoModeAvailability,
                       IDS_POLICY_OUT_OF_RANGE_ERROR,
                       base::IntToString(int_value));
      return false;
    }
    return true;
  }

  const base::Value* deprecated_enabled =
      policies.GetValue(key::kIncognitoEnabled);
  if (deprecated_enabled && !deprecated_enabled->is_bool()) {
    errors->AddError(key::kIncognitoEnabled, IDS_POLICY_TYPE_ERROR,
                     base::Value::GetTypeName(base::Value::Type::BOOLEAN));
    return false;
  }
  return true;
}

void IncognitoModePolicyHandler::ApplyPolicySettings(const PolicyMap& policies,
                                                     PrefValueMap* prefs) {
  const base::Value* availability =
      policies.GetValue(key::kIncognitoModeAvailability);
  const base::Value* deprecated_enabled =
      policies.GetValue(key::kIncognitoEnabled);
  if (availability) {
    int int_value = IncognitoModePrefs::ENABLED;
    IncognitoModePrefs::Availability availability_enum_value;
    if (availability->GetAsInteger(&int_value) &&
        IncognitoModePrefs::IntToAvailability(int_value,
                                              &availability_enum_value)) {
      prefs->SetInteger(prefs::kIncognitoModeAvailability,
                        availability_enum_value);
    } else {
      NOTREACHED();
    }
  } else if (deprecated_enabled) {
    // If kIncognitoModeAvailability is not specified, check the obsolete
    // kIncognitoEnabled.
    bool enabled = true;
    if (deprecated_enabled->GetAsBoolean(&enabled)) {
      prefs->SetInteger(
          prefs::kIncognitoModeAvailability,
          enabled ? IncognitoModePrefs::ENABLED : IncognitoModePrefs::DISABLED);
    } else {
      NOTREACHED();
    }
  }
}

}  // namespace policy
