// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/platform_keys/key_permissions_policy_handler.h"

#include "components/policy/core/common/schema.h"
#include "components/policy/policy_constants.h"

namespace chromeos {

KeyPermissionsPolicyHandler::KeyPermissionsPolicyHandler(
    const policy::Schema& chrome_schema)
    : policy::SchemaValidatingPolicyHandler(
          policy::key::kKeyPermissions,
          chrome_schema.GetKnownProperty(policy::key::kKeyPermissions),
          policy::SCHEMA_ALLOW_UNKNOWN) {
}

void KeyPermissionsPolicyHandler::ApplyPolicySettings(
    const policy::PolicyMap& /* policies */,
    PrefValueMap* /* prefs */) {
}

}  // namespace chromeos
