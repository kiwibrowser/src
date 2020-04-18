// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/default_download_dir_policy_handler.h"

#include "base/files/file_path.h"
#include "base/values.h"
#include "chrome/browser/download/download_dir_util.h"
#include "chrome/common/pref_names.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"

DefaultDownloadDirPolicyHandler::DefaultDownloadDirPolicyHandler()
    : TypeCheckingPolicyHandler(policy::key::kDefaultDownloadDirectory,
                                base::Value::Type::STRING) {}

DefaultDownloadDirPolicyHandler::~DefaultDownloadDirPolicyHandler() = default;

bool DefaultDownloadDirPolicyHandler::CheckPolicySettings(
    const policy::PolicyMap& policies,
    policy::PolicyErrorMap* errors) {
  const base::Value* value = nullptr;
  if (!CheckAndGetValue(policies, errors, &value))
    return false;
  return true;
}

void DefaultDownloadDirPolicyHandler::ApplyPolicySettingsWithParameters(
    const policy::PolicyMap& policies,
    const policy::PolicyHandlerParameters& parameters,
    PrefValueMap* prefs) {
  const base::Value* value = policies.GetValue(policy_name());
  base::FilePath::StringType string_value;
  if (!value || !value->GetAsString(&string_value))
    return;

  base::FilePath::StringType expanded_value =
      download_dir_util::ExpandDownloadDirectoryPath(string_value, parameters);

  if (policies.Get(policy_name())->level == policy::POLICY_LEVEL_RECOMMENDED) {
    prefs->SetValue(prefs::kDownloadDefaultDirectory,
                    std::make_unique<base::Value>(expanded_value));
    prefs->SetValue(prefs::kSaveFileDefaultDirectory,
                    std::make_unique<base::Value>(expanded_value));
  }
}

void DefaultDownloadDirPolicyHandler::ApplyPolicySettings(
    const policy::PolicyMap& /* policies */,
    PrefValueMap* /* prefs */) {
  NOTREACHED();
}
