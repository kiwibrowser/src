// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/values.h"
#include "chrome/browser/policy/file_selection_dialogs_policy_handler.h"
#include "chrome/common/pref_names.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

// Test cases for how the AllowFileSelectionDialogs policy influences the
// PromptForDownload preference.
class FileSelectionDialogsPolicyTest : public testing::Test {
 protected:
  PolicyMap policy_;
  FileSelectionDialogsPolicyHandler handler_;
  PrefValueMap prefs_;
};

TEST_F(FileSelectionDialogsPolicyTest, Default) {
  handler_.ApplyPolicySettings(policy_, &prefs_);
  EXPECT_FALSE(prefs_.GetValue(prefs::kPromptForDownload, NULL));
}

TEST_F(FileSelectionDialogsPolicyTest, EnableFileSelectionDialogs) {
  policy_.Set(key::kAllowFileSelectionDialogs, POLICY_LEVEL_MANDATORY,
              POLICY_SCOPE_USER, POLICY_SOURCE_CLOUD,
              std::make_unique<base::Value>(true), nullptr);
  handler_.ApplyPolicySettings(policy_, &prefs_);

  // Allowing file-selection dialogs should not influence the PromptForDownload
  // pref.
  EXPECT_FALSE(prefs_.GetValue(prefs::kPromptForDownload, NULL));
}

TEST_F(FileSelectionDialogsPolicyTest, DisableFileSelectionDialogs) {
  policy_.Set(key::kAllowFileSelectionDialogs, POLICY_LEVEL_MANDATORY,
              POLICY_SCOPE_USER, POLICY_SOURCE_CLOUD,
              std::make_unique<base::Value>(false), nullptr);
  handler_.ApplyPolicySettings(policy_, &prefs_);

  // Disabling file-selection dialogs should disable the PromptForDownload pref.
  const base::Value* value = NULL;
  EXPECT_TRUE(prefs_.GetValue(prefs::kPromptForDownload, &value));
  ASSERT_TRUE(value);
  bool prompt_for_download = true;
  bool result = value->GetAsBoolean(&prompt_for_download);
  EXPECT_TRUE(result);
  EXPECT_FALSE(prompt_for_download);
}

}  // namespace policy
