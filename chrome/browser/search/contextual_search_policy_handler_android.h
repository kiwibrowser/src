// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_CONTEXTUAL_SEARCH_POLICY_HANDLER_ANDROID_H_
#define CHROME_BROWSER_SEARCH_CONTEXTUAL_SEARCH_POLICY_HANDLER_ANDROID_H_

#include "base/macros.h"
#include "components/policy/core/browser/configuration_policy_handler.h"

namespace policy {

class PolicyMap;

// ConfigurationPolicyHandler for the ContextualSearchEnabled policy.
class ContextualSearchPolicyHandlerAndroid
  : public TypeCheckingPolicyHandler {
 public:
  ContextualSearchPolicyHandlerAndroid();
  ~ContextualSearchPolicyHandlerAndroid() override;

  // ConfigurationPolicyHandler methods:
  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContextualSearchPolicyHandlerAndroid);
};

}  // namespace policy

#endif  // CHROME_BROWSER_SEARCH_CONTEXTUAL_SEARCH_POLICY_HANDLER_ANDROID_H_
