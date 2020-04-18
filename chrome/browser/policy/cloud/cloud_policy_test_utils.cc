// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "chrome/browser/policy/cloud/cloud_policy_test_utils.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"

namespace policy {

void GetExpectedDefaultPolicy(PolicyMap* policy_map) {
  policy_map->Set(key::kNTPContentSuggestionsEnabled, POLICY_LEVEL_MANDATORY,
                  POLICY_SCOPE_USER, POLICY_SOURCE_ENTERPRISE_DEFAULT,
                  base::WrapUnique(new base::Value(false)), nullptr);
#if defined(OS_CHROMEOS)
  SetEnterpriseUsersDefaults(policy_map);
#endif
}


}  // namespace policy
