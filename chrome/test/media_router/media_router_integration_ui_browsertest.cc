// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/media_router/media_router_integration_browsertest.h"

#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace media_router {

namespace {
const char kTestSinkName[] = "test-sink-1";
}

IN_PROC_BROWSER_TEST_F(MediaRouterIntegrationBrowserTest, Dialog_Basic) {
  OpenTestPage(FILE_PATH_LITERAL("basic_test.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::WebContents* dialog_contents = OpenMRDialog(web_contents);
  WaitUntilSinkDiscoveredOnUI();
  // Verify the sink list.
  std::string sink_length_script = base::StringPrintf(
      "domAutomationController.send("
      "window.document.getElementById('media-router-container')."
      "sinksToShow_.length)");
  ASSERT_GT(ExecuteScriptAndExtractInt(dialog_contents, sink_length_script), 0);
  LOG(INFO) << "Choose Sink";
  ChooseSink(web_contents, kTestSinkName);

// Linux and Windows bots run browser tests without a physical display, which
// is causing flaky event dispatching of mouseenter and mouseleave events. This
// causes the dialog to sometimes close prematurely even though a mouseenter
// event is explicitly dispatched in the test.
// Here, we still dispatch the mouseenter event for OSX, but close
// the dialog and reopen it on Linux and Windows.
// The test succeeds fine when run with a physical display.
// http://crbug.com/577943 http://crbug.com/591779
#if defined(OS_MACOSX)
  // Simulate keeping the mouse on the dialog to prevent it from automatically
  // closing after the route has been created. Then, check that the dialog
  // remains open.
  std::string mouse_enter_script = base::StringPrintf(
      "window.document.getElementById('media-router-container')"
      "    .dispatchEvent(new Event('mouseenter'));");
  ASSERT_TRUE(content::ExecuteScript(dialog_contents, mouse_enter_script));
#endif
  WaitUntilRouteCreated();

#if defined(OS_MACOSX)
  CheckDialogRemainsOpen(web_contents);
#elif defined(OS_LINUX) || defined(OS_WIN)
  Wait(base::TimeDelta::FromSeconds(5));
  LOG(INFO) << "Waiting for dialog to be closed";
  WaitUntilDialogClosed(web_contents);
  LOG(INFO) << "Reopen MR dialog";
  dialog_contents = OpenMRDialog(web_contents);
#endif

  LOG(INFO) << "Check route details dialog";
  std::string route_script;
  // Verify the route details is not undefined.
  route_script = base::StringPrintf(
      "domAutomationController.send("
      "window.document.getElementById('media-router-container').shadowRoot."
      "getElementById('route-details') != undefined)");
  ASSERT_TRUE(ConditionalWait(
      base::TimeDelta::FromSeconds(30), base::TimeDelta::FromSeconds(1),
      base::Bind(
        &MediaRouterIntegrationBrowserTest::ExecuteScriptAndExtractBool,
        dialog_contents, route_script)));
  route_script = base::StringPrintf(
      "domAutomationController.send("
      "window.document.getElementById('media-router-container').currentView_ "
      "== media_router.MediaRouterView.ROUTE_DETAILS)");
  ASSERT_TRUE(ExecuteScriptAndExtractBool(dialog_contents, route_script));

  // Verify the route details page.
  route_script = base::StringPrintf(
      "domAutomationController.send("
      "window.document.getElementById('media-router-container').shadowRoot."
      "getElementById('route-details').shadowRoot.getElementById("
      "'route-description').innerText)");
  std::string route_information = ExecuteScriptAndExtractString(
      dialog_contents, route_script);
  ASSERT_EQ("Test Route", route_information);

  std::string sink_script;
  // Verify the container header is not undefined.
  sink_script = base::StringPrintf(
      "domAutomationController.send("
      "window.document.getElementById('media-router-container').shadowRoot."
      "getElementById('container-header') != undefined)");
  LOG(INFO) << "Checking container-header";
  ASSERT_TRUE(ConditionalWait(
      base::TimeDelta::FromSeconds(30), base::TimeDelta::FromSeconds(1),
      base::Bind(
        &MediaRouterIntegrationBrowserTest::ExecuteScriptAndExtractBool,
        dialog_contents, sink_script)));

  sink_script = base::StringPrintf(
      "domAutomationController.send("
      "window.document.getElementById('media-router-container').shadowRoot."
      "getElementById('container-header').shadowRoot.getElementById("
      "'header-text').innerText)");
  std::string sink_name = ExecuteScriptAndExtractString(
      dialog_contents, sink_script);
  ASSERT_EQ(kTestSinkName, sink_name);
  LOG(INFO) << "Finish verification";

#if defined(OS_MACOSX)
  // Simulate moving the mouse off the dialog. Confirm that the dialog closes
  // automatically after the route is closed.
  // In tests, it sometimes takes too long to CloseRouteOnUI() to finish so
  // the timer started when the route is initially closed times out before the
  // mouseleave event is dispatched. In that case, the dialog remains open.
  std::string mouse_leave_script = base::StringPrintf(
      "window.document.getElementById('media-router-container')"
      "    .dispatchEvent(new Event('mouseleave'));");
  ASSERT_TRUE(content::ExecuteScript(dialog_contents, mouse_leave_script));
#endif
  LOG(INFO) << "Closing route on UI";
  CloseRouteOnUI();
#if defined(OS_MACOSX)
  LOG(INFO) << "Waiting for dialog to be closed";
  WaitUntilDialogClosed(web_contents);
#endif
  LOG(INFO) << "Closed dialog, end of test";
}

// TODO(crbug.com/822301): Flaky in Chromium waterfall.
IN_PROC_BROWSER_TEST_F(MediaRouterIntegrationBrowserTest,
                       MANUAL_Dialog_RouteCreationTimedOut) {
  SetTestData(FILE_PATH_LITERAL("route_creation_timed_out.json"));
  OpenTestPage(FILE_PATH_LITERAL("basic_test.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::WebContents* dialog_contents = OpenMRDialog(web_contents);

  // Verify the sink list.
  std::string sink_length_script = base::StringPrintf(
      "domAutomationController.send("
      "window.document.getElementById('media-router-container')."
      "sinksToShow_.length)");
  ASSERT_GT(ExecuteScriptAndExtractInt(dialog_contents, sink_length_script), 0);

  base::TimeTicks start_time(base::TimeTicks::Now());
  ChooseSink(web_contents, kTestSinkName);
  WaitUntilIssue();

  base::TimeDelta elapsed(base::TimeTicks::Now() - start_time);
  // The hardcoded timeout route creation timeout for the UI.
  // See kCreateRouteTimeoutSeconds in media_router_ui.cc.
  base::TimeDelta expected_timeout(base::TimeDelta::FromSeconds(20));

  EXPECT_GE(elapsed, expected_timeout);
  EXPECT_LE(elapsed - expected_timeout, base::TimeDelta::FromSeconds(5));

  std::string issue_title = GetIssueTitle();
  // TODO(imcheng): Fix host name for file schemes (crbug.com/560576).
  ASSERT_EQ(l10n_util::GetStringFUTF8(
                IDS_MEDIA_ROUTER_ISSUE_CREATE_ROUTE_TIMEOUT, base::string16()),
            issue_title);

  // Route will still get created, it just takes longer than usual.
  WaitUntilRouteCreated();
}

}  // namespace media_router
