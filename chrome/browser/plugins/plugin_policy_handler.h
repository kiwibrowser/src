// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PLUGIN_POLICY_HANDLER_H_
#define CHROME_BROWSER_PLUGINS_PLUGIN_POLICY_HANDLER_H_

#include <map>
#include <set>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/plugins/plugin_prefs.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"

// Migrates values of deprecated plugin policies into their new counterpars.
// It reads the set of EnabledPlugins and DisabledPlugins and updates the Flash
// default behavior and PDF plugin behavior. All other values in the list are
// ignored.
class PluginPolicyHandler : public policy::ConfigurationPolicyHandler {
 public:
  PluginPolicyHandler();
  ~PluginPolicyHandler() override;

 protected:
  bool CheckPolicySettings(const policy::PolicyMap& policies,
                           policy::PolicyErrorMap* errors) override;
  void ApplyPolicySettings(const policy::PolicyMap& policies,
                           PrefValueMap* prefs) override;

 private:
  void ProcessPolicy(const policy::PolicyMap& policies,
                     PrefValueMap* prefs,
                     const std::string& policy,
                     bool disable_pdf_plugin,
                     ContentSetting flash_content_setting);

  DISALLOW_COPY_AND_ASSIGN(PluginPolicyHandler);
};

#endif  // CHROME_BROWSER_PLUGINS_PLUGIN_POLICY_HANDLER_H_
