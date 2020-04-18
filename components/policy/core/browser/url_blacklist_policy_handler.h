// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_BROWSER_URL_BLACKLIST_POLICY_HANDLER_H_
#define COMPONENTS_POLICY_CORE_BROWSER_URL_BLACKLIST_POLICY_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_export.h"

namespace policy {

// Handles URLBlacklist policies.
class POLICY_EXPORT URLBlacklistPolicyHandler
    : public ConfigurationPolicyHandler {
 public:
  URLBlacklistPolicyHandler();
  ~URLBlacklistPolicyHandler() override;

  // ConfigurationPolicyHandler methods:
  bool CheckPolicySettings(const PolicyMap& policies,
                           PolicyErrorMap* errors) override;
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(URLBlacklistPolicyHandler);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_BROWSER_URL_BLACKLIST_POLICY_HANDLER_H_
