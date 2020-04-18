// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/settings_private/settings_private_delegate.h"
#include "chrome/browser/extensions/api/settings_private/settings_private_delegate_factory.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "chrome/common/pref_names.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_utils.h"
#include "extensions/common/switches.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "chromeos/chromeos_switches.h"
#endif

using testing::Mock;
using testing::Return;
using testing::_;

namespace extensions {

namespace {

class SettingsPrivateApiTest : public ExtensionApiTest {
 public:
  SettingsPrivateApiTest() {}
  ~SettingsPrivateApiTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
#if defined(OS_CHROMEOS)
    command_line->AppendSwitch(chromeos::switches::kStubCrosSettings);
#endif
  }

  void SetUpInProcessBrowserTestFixture() override {
    EXPECT_CALL(provider_, IsInitializationComplete(_))
        .WillRepeatedly(Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
    ExtensionApiTest::SetUpInProcessBrowserTestFixture();
  }

 protected:
  bool RunSettingsSubtest(const std::string& subtest) {
    return RunExtensionSubtest("settings_private", "main.html?" + subtest,
                               kFlagLoadAsComponent);
  }

  void SetPrefPolicy(const std::string& key, policy::PolicyLevel level) {
    policy::PolicyMap policies;
    policies.Set(key, level, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD,
                 base::WrapUnique(new base::Value(true)), nullptr);
    provider_.UpdateChromePolicy(policies);
    DCHECK(base::MessageLoopCurrent::Get());
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

 private:
  policy::MockConfigurationPolicyProvider provider_;

  DISALLOW_COPY_AND_ASSIGN(SettingsPrivateApiTest);
};


}  // namespace

IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, SetPref) {
  EXPECT_TRUE(RunSettingsSubtest("setPref")) << message_;
}

IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, GetPref) {
  EXPECT_TRUE(RunSettingsSubtest("getPref")) << message_;
}

IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, GetEnforcedPref) {
  SetPrefPolicy(policy::key::kHomepageIsNewTabPage,
                policy::POLICY_LEVEL_MANDATORY);
  EXPECT_TRUE(RunSettingsSubtest("getEnforcedPref")) << message_;
}

IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, GetRecommendedPref) {
  SetPrefPolicy(policy::key::kHomepageIsNewTabPage,
                policy::POLICY_LEVEL_RECOMMENDED);
  EXPECT_TRUE(RunSettingsSubtest("getRecommendedPref")) << message_;
}

IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, GetAllPrefs) {
  EXPECT_TRUE(RunSettingsSubtest("getAllPrefs")) << message_;
}

IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, OnPrefsChanged) {
  EXPECT_TRUE(RunSettingsSubtest("onPrefsChanged")) << message_;
}

#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, GetPref_CrOSSetting) {
  EXPECT_TRUE(RunSettingsSubtest("getPref_CrOSSetting")) << message_;
}

IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, SetPref_CrOSSetting) {
  EXPECT_TRUE(RunSettingsSubtest("setPref_CrOSSetting")) << message_;
}

IN_PROC_BROWSER_TEST_F(SettingsPrivateApiTest, OnPrefsChanged_CrOSSetting) {
  EXPECT_TRUE(RunSettingsSubtest("onPrefsChanged_CrOSSetting")) << message_;
}
#endif

}  // namespace extensions
