// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spellcheck_language_policy_handler.h"

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/spellcheck/common/spellcheck_common.h"

SpellcheckLanguagePolicyHandler::SpellcheckLanguagePolicyHandler()
    : TypeCheckingPolicyHandler(policy::key::kSpellcheckLanguage,
                                base::Value::Type::LIST) {}

SpellcheckLanguagePolicyHandler::~SpellcheckLanguagePolicyHandler() = default;

bool SpellcheckLanguagePolicyHandler::CheckPolicySettings(
    const policy::PolicyMap& policies,
    policy::PolicyErrorMap* errors) {
  const base::Value* value = nullptr;
  return CheckAndGetValue(policies, errors, &value);
}

void SpellcheckLanguagePolicyHandler::ApplyPolicySettings(
    const policy::PolicyMap& policies,
    PrefValueMap* prefs) {
  // Ignore this policy if the SpellcheckEnabled policy disables spellcheck.
  const base::Value* spellcheck_enabled_value =
      policies.GetValue(policy::key::kSpellcheckEnabled);
  if (spellcheck_enabled_value && spellcheck_enabled_value->GetBool() == false)
    return;

  const base::Value* value = policies.GetValue(policy_name());
  if (!value)
    return;

  const base::Value::ListStorage& languages = value->GetList();

  std::unique_ptr<base::ListValue> forced_language_list =
      std::make_unique<base::ListValue>();
  for (const base::Value& language : languages) {
    std::string current_language =
        spellcheck::GetCorrespondingSpellCheckLanguage(
            base::TrimWhitespaceASCII(language.GetString(), base::TRIM_ALL));
    if (!current_language.empty()) {
      forced_language_list->GetList().push_back(base::Value(current_language));
    } else {
      LOG(WARNING) << "Unknown language requested: \"" << language << "\"";
    }
  }

  prefs->SetValue(spellcheck::prefs::kSpellCheckEnable,
                  std::make_unique<base::Value>(true));
  prefs->SetValue(spellcheck::prefs::kSpellCheckForcedDictionaries,
                  std::move(forced_language_list));
}
