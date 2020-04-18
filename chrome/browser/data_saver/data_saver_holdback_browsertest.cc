// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/metrics/field_trial_param_associator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test_base.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

// Tests if the save data header holdback works as expected.
class DataSaverHoldbackBrowserTest : public InProcessBrowserTest,
                                     public testing::WithParamInterface<bool> {
 protected:
  DataSaverHoldbackBrowserTest() { ConfigureHoldbackExperiment(); }
  void SetUp() override {
    test_server_.ServeFilesFromSourceDirectory("content/test/data");
    ASSERT_TRUE(test_server_.Start());
    InProcessBrowserTest::SetUp();
  }

  void EnableDataSaver(bool enabled) {
    PrefService* prefs = browser()->profile()->GetPrefs();
    prefs->SetBoolean(prefs::kDataSaverEnabled, enabled);
    content::RunAllPendingInMessageLoop();
  }

  void VerifySaveDataHeader(const std::string& expected_header_value) {
    ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/echoheader?Save-Data"));
    std::string header_value;
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(
        browser()->tab_strip_model()->GetActiveWebContents(),
        "window.domAutomationController.send(document.body.textContent);",
        &header_value));
    EXPECT_EQ(expected_header_value, header_value);
  }

  void VerifySaveDataAPI(bool expected_header_set) {
    ui_test_utils::NavigateToURL(browser(),
                                 test_server_.GetURL("/net_info.html"));
    EXPECT_EQ(expected_header_set, RunScriptExtractBool("getSaveData()"));
  }

  void ConfigureHoldbackExperiment() {
    base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
    const std::string kTrialName = "TrialFoo";
    const std::string kGroupName = "GroupFoo";  // Value not used

    scoped_refptr<base::FieldTrial> trial =
        base::FieldTrialList::CreateFieldTrial(kTrialName, kGroupName);

    std::map<std::string, std::string> params;
    params["holdback_web"] = GetParam() ? "true" : "false";
    ASSERT_TRUE(
        base::FieldTrialParamAssociator::GetInstance()
            ->AssociateFieldTrialParams(kTrialName, kGroupName, params));

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(
        features::kDataSaverHoldback.name,
        base::FeatureList::OVERRIDE_ENABLE_FEATURE, trial.get());
    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
  }

 private:
  bool RunScriptExtractBool(const std::string& script) {
    bool data;
    EXPECT_TRUE(ExecuteScriptAndExtractBool(
        browser()->tab_strip_model()->GetActiveWebContents(), script, &data));
    return data;
  }

  net::EmbeddedTestServer test_server_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// The data saver holdback is enabled only if the first param is true.
INSTANTIATE_TEST_CASE_P(, DataSaverHoldbackBrowserTest, testing::Bool());

IN_PROC_BROWSER_TEST_P(DataSaverHoldbackBrowserTest,
                       DataSaverEnabledWithHoldbackEnabled) {
  ASSERT_TRUE(embedded_test_server()->Start());
  EnableDataSaver(true);

  // If holdback is enabled, then the save-data header should not be set.
  if (GetParam()) {
    VerifySaveDataHeader("None");
  } else {
    VerifySaveDataHeader("on");
  }
  VerifySaveDataAPI(!GetParam());
}
