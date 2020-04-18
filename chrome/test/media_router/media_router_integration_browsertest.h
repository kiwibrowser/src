// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_MEDIA_ROUTER_MEDIA_ROUTER_INTEGRATION_BROWSERTEST_H_
#define CHROME_TEST_MEDIA_ROUTER_MEDIA_ROUTER_INTEGRATION_BROWSERTEST_H_

#include <memory>
#include <string>

#include "base/debug/stack_trace.h"
#include "base/files/file_path.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/media_router/media_cast_mode.h"
#include "chrome/browser/ui/toolbar/media_router_action.h"
#include "chrome/test/media_router/media_router_base_browsertest.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"

namespace media_router {

class MediaRouterDialogControllerWebUIImpl;
class MediaRouterUI;
struct IssueInfo;

class MediaRouterIntegrationBrowserTest : public MediaRouterBaseBrowserTest {
 public:
  MediaRouterIntegrationBrowserTest();
  ~MediaRouterIntegrationBrowserTest() override;

 protected:
  // InProcessBrowserTest Overrides
  void TearDownOnMainThread() override;

  // MediaRouterBaseBrowserTest Overrides
  void ParseCommandLine() override;

  // Simulate user action to choose one sink in the popup dialog.
  // |web_contents|: The web contents of the test page which invokes the popup
  //                 dialog.
  // |sink_name|: The sink's human readable name.
  void ChooseSink(content::WebContents* web_contents,
                  const std::string& sink_name);

  // Simulate user action to choose the local media cast mode in the popup
  // dialog. This will not work unless the dialog is in the choose cast mode
  // view. |dialog_contents|: The web contents of the popup dialog.
  // |cast_mode_text|: The cast mode's dialog name.
  void ClickCastMode(content::WebContents* dialog_contents, MediaCastMode mode);

  // Checks that the request initiated from |web_contents| to start presentation
  // failed with expected |error_name| and |error_message_substring|.
  void CheckStartFailed(content::WebContents* web_contents,
                        const std::string& error_name,
                        const std::string& error_message_substring);

  // Execute javascript and check the return value.
  static void ExecuteJavaScriptAPI(content::WebContents* web_contents,
                            const std::string& script);

  static int ExecuteScriptAndExtractInt(
      const content::ToRenderFrameHost& adapter,
      const std::string& script);

  static std::string ExecuteScriptAndExtractString(
      const content::ToRenderFrameHost& adapter, const std::string& script);

  void ClickDialog();

  // Clicks on the header of the dialog. If in sinks view, will change to cast
  // mode view, if in cast mode view, will change to sinks view.
  void ClickHeader(content::WebContents* dialog_contents);

  static bool ExecuteScriptAndExtractBool(
      const content::ToRenderFrameHost& adapter,
      const std::string& script);

  static void ExecuteScript(const content::ToRenderFrameHost& adapter,
                            const std::string& script);

  // Get the chrome modal dialog.
  // |web_contents|: The web contents of the test page which invokes the popup
  //                 dialog.
  content::WebContents* GetMRDialog(content::WebContents* web_contents);

  // Checks that the chrome modal dialog does not exist.
  bool IsDialogClosed(content::WebContents* web_contents);
  void WaitUntilDialogClosed(content::WebContents* web_contents);

  void CheckDialogRemainsOpen(content::WebContents* web_contents);

  // Opens "basic_test.html" and asserts that attempting to start a presentation
  // fails with NotFoundError due to no sinks available.
  void StartSessionAndAssertNotFoundError();

  // Opens "basic_test.html," waits for sinks to be available, and starts a
  // presentation.
  content::WebContents* StartSessionWithTestPageAndSink();

  // Opens "basic_test.html," waits for sinks to be available, starts a
  // presentation, and chooses a sink with the name |kTestSinkName|. Also checks
  // that the presentation has successfully started if |should_succeed| is true.
  content::WebContents* StartSessionWithTestPageAndChooseSink();

  // Opens the MR dialog and clicks through the motions of casting a file. Sets
  // up the route provider to succeed or otherwise based on |route_success|.
  // Note: The system dialog portion has to be mocked out as it cannot be
  // simulated.
  void OpenDialogAndCastFile(bool route_success = true);

  // Opens the MR dialog and clicks through the motions of choosing to cast
  // file, file returns an issue. Note: The system dialog portion has to be
  // mocked out as it cannot be simulated.
  void OpenDialogAndCastFileFails();

  void OpenTestPage(base::FilePath::StringPieceType file);
  void OpenTestPageInNewTab(base::FilePath::StringPieceType file);
  virtual GURL GetTestPageUrl(const base::FilePath& full_path);

  void SetTestData(base::FilePath::StringPieceType test_data_file);

  // Start presentation and wait until the pop dialog shows up.
  // |web_contents|: The web contents of the test page which invokes the popup
  //                 dialog.
  void StartSession(content::WebContents* web_contents);

  // Open the chrome modal dialog.
  // |web_contents|: The web contents of the test page which invokes the popup
  //                 dialog.
  content::WebContents* OpenMRDialog(content::WebContents* web_contents);
  bool IsRouteCreatedOnUI();

  bool IsRouteClosedOnUI();

  bool IsSinkDiscoveredOnUI();

  // Close route through clicking 'Stop casting' button in route details dialog.
  void CloseRouteOnUI();

  // Wait for the route to show up in the UI with a timeout. Fails if the
  // route did not show up before the timeout.
  void WaitUntilRouteCreated();

  // Wait until there is an issue showing in the UI.
  void WaitUntilIssue();

  // Returns true if there is an issue showing in the UI.
  bool IsUIShowingIssue();

  // Returns the title of issue showing in UI. It is an error to call this if
  // there are no issues showing in UI.
  std::string GetIssueTitle();

  // Returns the route ID for the specific sink.
  std::string GetRouteId(const std::string& sink_id);

  // Wait for the specific sink shows up in UI with a timeout. Fails if the sink
  // doesn't show up before the timeout.
  void WaitUntilSinkDiscoveredOnUI();

  // Checks if media router dialog is fully loaded.
  bool IsDialogLoaded(content::WebContents* dialog_contents);

  // Wait until media router dialog is fully loaded.
  void WaitUntilDialogFullyLoaded(content::WebContents* dialog_contents);

  // Checks that the presentation started for |web_contents| has connected and
  // is the default presentation.
  void CheckSessionValidity(content::WebContents* web_contents);

  // Checks that a Media Router dialog is shown for |web_contents|, and returns
  // its controller.
  MediaRouterDialogControllerWebUIImpl* GetControllerForShownDialog(
      content::WebContents* web_contents);

  // Returns the active WebContents for the current window.
  content::WebContents* GetActiveWebContents();

  // Gets the MediaRouterUI that is associated with the open dialog.
  // This is needed to bypass potential issues that may arise when trying to
  // test code that uses the native file dialog.
  MediaRouterUI* GetMediaRouterUI(content::WebContents* media_router_dialog);

  // Sets the MediaRouterFileDialog to act like a valid file was selected on
  // opening the dialog.
  void FileDialogSelectsFile(MediaRouterUI* media_router_ui, GURL file_url);

  // Sets the MediaRouterFileDialog to act like a bad file was selected on
  // opening the dialog.
  void FileDialogSelectFails(MediaRouterUI* media_router_ui,
                             const IssueInfo& issue);

  // Runs a basic test in which a presentation is created through the
  // MediaRouter dialog, then terminated.
  void RunBasicTest();

  // Runs a test in which we start a presentation and send a message.
  void RunSendMessageTest(const std::string& message);

  // Runs a test in which we start a presentation, close it and send a message.
  void RunFailToSendMessageTest();

  // Runs a test in which we start a presentation and reconnect to it from
  // another tab.
  void RunReconnectSessionTest();

  // Runs a test in which we start a presentation and reconnect to it from the
  // same tab.
  void RunReconnectSessionSameTabTest();

  std::string receiver() const { return receiver_; }

  // Enabled features
  base::test::ScopedFeatureList scoped_feature_list_;

 private:
  // Get the full path of the resource file.
  // |relative_path|: The relative path to
  //                  <chromium src>/out/<build config>/media_router/
  //                  browser_test_resources/
  base::FilePath GetResourceFile(
      base::FilePath::StringPieceType relative_path) const;

  std::unique_ptr<content::TestNavigationObserver> test_navigation_observer_;

  // Fields
  std::string receiver_;
};

class MediaRouterIntegrationIncognitoBrowserTest
    : public MediaRouterIntegrationBrowserTest {
 public:
  void InstallAndEnableMRExtension() override;
  void UninstallMRExtension() override;
  Browser* browser() override;

 private:
  Browser* incognito_browser_ = nullptr;
  std::string incognito_extension_id_;
};

}  // namespace media_router

#endif  // CHROME_TEST_MEDIA_ROUTER_MEDIA_ROUTER_INTEGRATION_BROWSERTEST_H_
