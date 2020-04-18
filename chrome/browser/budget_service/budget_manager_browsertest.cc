// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "chrome/browser/budget_service/budget_manager.h"
#include "chrome/browser/budget_service/budget_manager_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/engagement/site_engagement_score.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/platform/modules/budget_service/budget_service.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

const char kTestURL[] = "/budget_service/test.html";

class BudgetManagerBrowserTest : public InProcessBrowserTest {
 public:
  BudgetManagerBrowserTest() = default;
  ~BudgetManagerBrowserTest() override = default;

  // InProcessBrowserTest:
  void SetUp() override {
    https_server_.reset(
        new net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_->Start());

    InProcessBrowserTest::SetUp();
  }

  // InProcessBrowserTest:
  void SetUpOnMainThread() override {
    SiteEngagementScore::SetParamValuesForTesting();

    // Grant Notification permission for these tests. See the privacy
    // requirement for this outlined in https://crbug.com/710809.
    HostContentSettingsMapFactory::GetForProfile(browser()->profile())
        ->SetContentSettingDefaultScope(https_server_->base_url(), GURL(),
                                        CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
                                        std::string(), CONTENT_SETTING_ALLOW);

    LoadTestPage();
    budget_manager_ = BudgetManagerFactory::GetForProfile(browser()->profile());
  }

  // InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // TODO(harkness): Remove switch once Budget API ships. (crbug.com/617971)
    command_line->AppendSwitch(
        switches::kEnableExperimentalWebPlatformFeatures);
  }

  // Sets the absolute Site Engagement |score| for the testing origin.
  void SetSiteEngagementScore(double score) {
    SiteEngagementService* service =
        SiteEngagementService::Get(browser()->profile());

    service->ResetBaseScoreForURL(https_server_->GetURL(kTestURL), score);
  }

  bool RunScript(const std::string& script, std::string* result) {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    return content::ExecuteScriptAndExtractString(web_contents->GetMainFrame(),
                                                  script, result);
  }

  void LoadTestPage() {
    ui_test_utils::NavigateToURL(browser(), https_server_->GetURL(kTestURL));
  }

  void DidConsume(base::Closure run_loop_closure, bool success) {
    success_ = success;
    run_loop_closure.Run();
  }

  void ConsumeReservation() {
    base::RunLoop run_loop;
    budget_manager()->Consume(
        url::Origin::Create(https_server_->GetURL(kTestURL)),
        blink::mojom::BudgetOperationType::SILENT_PUSH,
        base::BindOnce(&BudgetManagerBrowserTest::DidConsume,
                       base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  BudgetManager* budget_manager() const { return budget_manager_; }
  bool success() const { return success_; }

 private:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  // Lifetime of the BudgetManager is tied to the profile of the test.
  BudgetManager* budget_manager_ = nullptr;
  bool success_ = false;
};

IN_PROC_BROWSER_TEST_F(BudgetManagerBrowserTest, BudgetInDocument) {
  std::string script_result;

  SetSiteEngagementScore(5);

  // Site Engagement score of 5 gives a budget of 2.
  ASSERT_TRUE(RunScript("documentGetBudget()", &script_result));
  EXPECT_EQ("ok - budget returned value of 2", script_result);

  ASSERT_TRUE(RunScript("documentReserveBudget()", &script_result));
  EXPECT_EQ("ok - reserved budget", script_result);

  // After reserving budget, the new budget should be at 0.
  ASSERT_TRUE(RunScript("documentGetBudget()", &script_result));
  EXPECT_EQ("ok - budget returned value of 0", script_result);

  // A second reserve should fail because there is not enough budget.
  ASSERT_TRUE(RunScript("documentReserveBudget()", &script_result));
  EXPECT_EQ("failed - not able to reserve budget", script_result);

  // Consume should succeed because there is an existing reservation.
  ConsumeReservation();
  ASSERT_TRUE(success());

  // Second consume should fail because the reservation is consumed.
  ConsumeReservation();
  ASSERT_FALSE(success());
}

IN_PROC_BROWSER_TEST_F(BudgetManagerBrowserTest, BudgetInWorker) {
  std::string script_result;

  ASSERT_TRUE(RunScript("registerServiceWorker()", &script_result));
  ASSERT_EQ("ok - service worker registered", script_result);

  LoadTestPage();  // Reload to become controlled.
  SetSiteEngagementScore(12);

  ASSERT_TRUE(RunScript("isControlled()", &script_result));
  ASSERT_EQ("true - is controlled", script_result);

  // Site engagement score of 12 gives a budget of 5.
  ASSERT_TRUE(RunScript("workerGetBudget()", &script_result));
  EXPECT_EQ("ok - budget returned value of 5", script_result);

  // With a budget of 5, two reservations should succeed.
  ASSERT_TRUE(RunScript("workerReserveBudget()", &script_result));
  EXPECT_EQ("ok - reserved budget", script_result);

  ASSERT_TRUE(RunScript("workerReserveBudget()", &script_result));
  EXPECT_EQ("ok - reserved budget", script_result);

  // After reserving budget, the new budget should be at 1.
  ASSERT_TRUE(RunScript("workerGetBudget()", &script_result));
  EXPECT_EQ("ok - budget returned value of 1", script_result);

  // A second reserve should fail because there is not enough budget.
  ASSERT_TRUE(RunScript("workerReserveBudget()", &script_result));
  EXPECT_EQ("failed - not able to reserve budget", script_result);

  // Two consumes should succeed because there are existing reservations.
  ConsumeReservation();
  ASSERT_TRUE(success());

  ConsumeReservation();
  ASSERT_TRUE(success());

  // One more consume should fail, because all reservations are consumed.
  ConsumeReservation();
  ASSERT_FALSE(success());
}

}  // namespace
