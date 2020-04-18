// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_IOS_H_
#define COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_IOS_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/policy_export.h"

namespace base {
class SequencedTaskRunner;
}

namespace policy {

// Extends BrowserPolicyConnector with the setup for iOS builds.
class POLICY_EXPORT BrowserPolicyConnectorIOS : public BrowserPolicyConnector {
 public:
  BrowserPolicyConnectorIOS(
      const HandlerListFactory& handler_list_factory,
      const std::string& user_agent,
      scoped_refptr<base::SequencedTaskRunner> background_task_runner);

  ~BrowserPolicyConnectorIOS() override;

  void Init(
      PrefService* local_state,
      scoped_refptr<net::URLRequestContextGetter> request_context) override;

 private:
  std::string user_agent_;

  DISALLOW_COPY_AND_ASSIGN(BrowserPolicyConnectorIOS);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_BROWSER_BROWSER_POLICY_CONNECTOR_IOS_H_
