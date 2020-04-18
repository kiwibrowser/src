// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_CREATION_POLICY_HANDLER_H_
#define CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_CREATION_POLICY_HANDLER_H_

#include "base/macros.h"
#include "components/policy/core/browser/configuration_policy_handler.h"

class PrefValueMap;

namespace policy {

class PolicyMap;

class SupervisedUserCreationPolicyHandler : public TypeCheckingPolicyHandler {
 public:
  SupervisedUserCreationPolicyHandler();
  ~SupervisedUserCreationPolicyHandler() override;

  void ApplyPolicySettings(const PolicyMap& policies,
                           PrefValueMap* prefs) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SupervisedUserCreationPolicyHandler);
};

}  // namespace policy

#endif  // CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_CREATION_POLICY_HANDLER_H_
