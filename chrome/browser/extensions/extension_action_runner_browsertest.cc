// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_action_runner.h"

#include <stddef.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/scripting_permissions_modifier.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/common/extension_features.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/test_extension_dir.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

const char kAllHostsScheme[] = "*://*/*";
const char kExplicitHostsScheme[] = "http://127.0.0.1/*";
const char kBackgroundScript[] =
    "\"background\": {\"scripts\": [\"script.js\"]}";
const char kBackgroundScriptSource[] =
    "var listener = function(tabId) {\n"
    "  chrome.tabs.onUpdated.removeListener(listener);\n"
    "  chrome.tabs.executeScript(tabId, {\n"
    "    code: \"chrome.test.sendMessage('inject succeeded');\"\n"
    "  });"
    "};\n"
    "chrome.tabs.onUpdated.addListener(listener);";
const char kContentScriptSource[] =
    "chrome.test.sendMessage('inject succeeded');";

const char kInjectSucceeded[] = "inject succeeded";

enum InjectionType { CONTENT_SCRIPT, EXECUTE_SCRIPT };

enum HostType { ALL_HOSTS, EXPLICIT_HOSTS };

enum RequiresConsent { REQUIRES_CONSENT, DOES_NOT_REQUIRE_CONSENT };

enum WithholdPermissions { WITHHOLD_PERMISSIONS, DONT_WITHHOLD_PERMISSIONS };

// Runs all pending tasks in the renderer associated with |web_contents|.
// Returns true on success.
bool RunAllPendingInRenderer(content::WebContents* web_contents) {
  // This is slight hack to achieve a RunPendingInRenderer() method. Since IPCs
  // are sent synchronously, anything started prior to this method will finish
  // before this method returns (as content::ExecuteScript() is synchronous).
  return content::ExecuteScript(web_contents, "1 == 1;");
}

// For use with blocked actions browsertests that put the result in
// window.localStorage. Returns the result or "undefined" if the result is not
// set.
std::string GetValue(content::WebContents* web_contents) {
  std::string out;
  if (!content::ExecuteScriptAndExtractString(
          web_contents,
          "var res = window.localStorage.getItem('extResult') || 'undefined';"
          "window.localStorage.removeItem('extResult');"
          "window.domAutomationController.send(res);",
          &out)) {
    out = "Failed to inject script";
  }
  return out;
}

}  // namespace

class ExtensionActionRunnerBrowserTest : public ExtensionBrowserTest {
 public:
  ExtensionActionRunnerBrowserTest() {}

  void SetUpCommandLine(base::CommandLine* command_line) override;
  void TearDownOnMainThread() override;

  // Returns an extension with the given |host_type| and |injection_type|. If
  // one already exists, the existing extension will be returned. Othewrwise,
  // one will be created.
  // This could potentially return NULL if LoadExtension() fails.
  const Extension* CreateExtension(HostType host_type,
                                   InjectionType injection_type,
                                   WithholdPermissions withhold_permissions);

 private:
  std::vector<std::unique_ptr<TestExtensionDir>> test_extension_dirs_;
  std::vector<const Extension*> extensions_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

void ExtensionActionRunnerBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  ExtensionBrowserTest::SetUpCommandLine(command_line);
  scoped_feature_list_.InitAndEnableFeature(features::kRuntimeHostPermissions);
}

void ExtensionActionRunnerBrowserTest::TearDownOnMainThread() {
  test_extension_dirs_.clear();
}

const Extension* ExtensionActionRunnerBrowserTest::CreateExtension(
    HostType host_type,
    InjectionType injection_type,
    WithholdPermissions withhold_permissions) {
  std::string name = base::StringPrintf(
      "%s %s",
      injection_type == CONTENT_SCRIPT ? "content_script" : "execute_script",
      host_type == ALL_HOSTS ? "all_hosts" : "explicit_hosts");

  const char* const permission_scheme =
      host_type == ALL_HOSTS ? kAllHostsScheme : kExplicitHostsScheme;

  std::string permissions = base::StringPrintf(
      "\"permissions\": [\"tabs\", \"%s\"]", permission_scheme);

  std::string scripts;
  std::string script_source;
  if (injection_type == CONTENT_SCRIPT) {
    scripts = base::StringPrintf(
        "\"content_scripts\": ["
        " {"
        "  \"matches\": [\"%s\"],"
        "  \"js\": [\"script.js\"],"
        "  \"run_at\": \"document_end\""
        " }"
        "]",
        permission_scheme);
  } else {
    scripts = kBackgroundScript;
  }

  std::string manifest = base::StringPrintf(
      "{"
      " \"name\": \"%s\","
      " \"version\": \"1.0\","
      " \"manifest_version\": 2,"
      " %s,"
      " %s"
      "}",
      name.c_str(), permissions.c_str(), scripts.c_str());

  std::unique_ptr<TestExtensionDir> dir(new TestExtensionDir);
  dir->WriteManifest(manifest);
  dir->WriteFile(FILE_PATH_LITERAL("script.js"),
                 injection_type == CONTENT_SCRIPT ? kContentScriptSource
                                                  : kBackgroundScriptSource);

  const Extension* extension = LoadExtension(dir->UnpackedPath());
  if (extension) {
    test_extension_dirs_.push_back(std::move(dir));
    extensions_.push_back(extension);

    ScriptingPermissionsModifier modifier(profile(), extension);
    if (withhold_permissions == WITHHOLD_PERMISSIONS &&
        modifier.CanAffectExtension()) {
      modifier.SetAllowedOnAllUrls(false);
    }
  }

  // If extension is NULL here, it will be caught later in the test.
  return extension;
}

class ActiveScriptTester {
 public:
  ActiveScriptTester(const std::string& name,
                     const Extension* extension,
                     Browser* browser,
                     RequiresConsent requires_consent,
                     InjectionType type);
  ~ActiveScriptTester();

  testing::AssertionResult Verify();

  std::string name() const;

 private:
  // Returns the ExtensionActionRunner, or null if one does not exist.
  ExtensionActionRunner* GetExtensionActionRunner();

  // Returns true if the extension has a pending request with the
  // ExtensionActionRunner.
  bool WantsToRun();

  // The name of the extension, and also the message it sends.
  std::string name_;

  // The extension associated with this tester.
  const Extension* extension_;

  // The browser the tester is running in.
  Browser* browser_;

  // Whether or not the extension has permission to run the script without
  // asking the user.
  RequiresConsent requires_consent_;

  // All of these extensions should inject a script (either through content
  // scripts or through chrome.tabs.executeScript()) that sends a message with
  // the |kInjectSucceeded| message.
  std::unique_ptr<ExtensionTestMessageListener> inject_success_listener_;
};

ActiveScriptTester::ActiveScriptTester(const std::string& name,
                                       const Extension* extension,
                                       Browser* browser,
                                       RequiresConsent requires_consent,
                                       InjectionType type)
    : name_(name),
      extension_(extension),
      browser_(browser),
      requires_consent_(requires_consent),
      inject_success_listener_(
          new ExtensionTestMessageListener(kInjectSucceeded,
                                           false /* won't reply */)) {
  inject_success_listener_->set_extension_id(extension->id());
}

ActiveScriptTester::~ActiveScriptTester() {}

std::string ActiveScriptTester::name() const {
  return name_;
}

testing::AssertionResult ActiveScriptTester::Verify() {
  if (!extension_)
    return testing::AssertionFailure() << "Could not load extension: " << name_;

  content::WebContents* web_contents =
      browser_ ? browser_->tab_strip_model()->GetActiveWebContents() : NULL;
  if (!web_contents)
    return testing::AssertionFailure() << "No web contents.";

  // Give the extension plenty of time to inject.
  if (!RunAllPendingInRenderer(web_contents))
    return testing::AssertionFailure() << "Could not run pending in renderer.";

  // Make sure all running tasks are complete.
  content::RunAllPendingInMessageLoop();

  ExtensionActionRunner* runner = GetExtensionActionRunner();
  if (!runner)
    return testing::AssertionFailure() << "Could not find runner.";

  bool wants_to_run = WantsToRun();

  // An extension should have an action displayed iff it requires user consent.
  if ((requires_consent_ == REQUIRES_CONSENT && !wants_to_run) ||
      (requires_consent_ == DOES_NOT_REQUIRE_CONSENT && wants_to_run)) {
    return testing::AssertionFailure()
           << "Improper wants to run for " << name_ << ": expected "
           << (requires_consent_ == REQUIRES_CONSENT) << ", found "
           << wants_to_run;
  }

  // If the extension has permission, we should be able to simply wait for it
  // to execute.
  if (requires_consent_ == DOES_NOT_REQUIRE_CONSENT) {
    EXPECT_TRUE(inject_success_listener_->WaitUntilSatisfied());
    return testing::AssertionSuccess();
  }

  // Otherwise, we don't have permission, and have to grant it. Ensure the
  // script has *not* already executed.
  if (inject_success_listener_->was_satisfied()) {
    return testing::AssertionFailure() << name_
                                       << "'s script ran without permission.";
  }

  // If we reach this point, we should always want to run.
  DCHECK(wants_to_run);

  // Grant permission by clicking on the extension action.
  runner->RunAction(extension_, true);

  // Now, the extension should be able to inject the script.
  EXPECT_TRUE(inject_success_listener_->WaitUntilSatisfied());

  // The extension should no longer want to run.
  wants_to_run = WantsToRun();
  if (wants_to_run) {
    return testing::AssertionFailure()
           << "Extension " << name_ << " still wants to run after injecting.";
  }

  return testing::AssertionSuccess();
}

ExtensionActionRunner* ActiveScriptTester::GetExtensionActionRunner() {
  return ExtensionActionRunner::GetForWebContents(
      browser_ ? browser_->tab_strip_model()->GetActiveWebContents() : nullptr);
}

bool ActiveScriptTester::WantsToRun() {
  ExtensionActionRunner* runner = GetExtensionActionRunner();
  return runner ? runner->WantsToRun(extension_) : false;
}

IN_PROC_BROWSER_TEST_F(ExtensionActionRunnerBrowserTest,
                       ActiveScriptsAreDisplayedAndDelayExecution) {
  // First, we load up three extensions:
  // - An extension that injects scripts into all hosts,
  // - An extension that injects scripts into explicit hosts,
  // - An extension with a content script that runs on all hosts,
  // - An extension with a content script that runs on explicit hosts.
  // The extensions that operate on explicit hosts have permission; the ones
  // that request all hosts require user consent.
  std::vector<std::unique_ptr<ActiveScriptTester>> testers;
  testers.push_back(std::make_unique<ActiveScriptTester>(
      "inject_scripts_all_hosts",
      CreateExtension(ALL_HOSTS, EXECUTE_SCRIPT, WITHHOLD_PERMISSIONS),
      browser(), REQUIRES_CONSENT, EXECUTE_SCRIPT));
  testers.push_back(std::make_unique<ActiveScriptTester>(
      "inject_scripts_explicit_hosts",
      CreateExtension(EXPLICIT_HOSTS, EXECUTE_SCRIPT, WITHHOLD_PERMISSIONS),
      browser(), DOES_NOT_REQUIRE_CONSENT, EXECUTE_SCRIPT));
  testers.push_back(std::make_unique<ActiveScriptTester>(
      "content_scripts_all_hosts",
      CreateExtension(ALL_HOSTS, CONTENT_SCRIPT, WITHHOLD_PERMISSIONS),
      browser(), REQUIRES_CONSENT, CONTENT_SCRIPT));
  testers.push_back(std::make_unique<ActiveScriptTester>(
      "content_scripts_explicit_hosts",
      CreateExtension(EXPLICIT_HOSTS, CONTENT_SCRIPT, WITHHOLD_PERMISSIONS),
      browser(), DOES_NOT_REQUIRE_CONSENT, CONTENT_SCRIPT));

  // Navigate to an URL (which matches the explicit host specified in the
  // extension content_scripts_explicit_hosts). All four extensions should
  // inject the script.
  ASSERT_TRUE(embedded_test_server()->Start());
  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/extensions/test_file.html"));

  for (const auto& tester : testers)
    EXPECT_TRUE(tester->Verify()) << tester->name();
}

// Test that removing an extension with pending injections a) removes the
// pending injections for that extension, and b) does not affect pending
// injections for other extensions.
IN_PROC_BROWSER_TEST_F(ExtensionActionRunnerBrowserTest,
                       RemoveExtensionWithPendingInjections) {
  // Load up two extensions, each with content scripts.
  const Extension* extension1 =
      CreateExtension(ALL_HOSTS, CONTENT_SCRIPT, WITHHOLD_PERMISSIONS);
  ASSERT_TRUE(extension1);
  const Extension* extension2 =
      CreateExtension(ALL_HOSTS, CONTENT_SCRIPT, WITHHOLD_PERMISSIONS);
  ASSERT_TRUE(extension2);

  ASSERT_NE(extension1->id(), extension2->id());

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  ExtensionActionRunner* action_runner =
      ExtensionActionRunner::GetForWebContents(web_contents);
  ASSERT_TRUE(action_runner);

  ASSERT_TRUE(embedded_test_server()->Start());
  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/extensions/test_file.html"));

  // Both extensions should have pending requests.
  EXPECT_TRUE(action_runner->WantsToRun(extension1));
  EXPECT_TRUE(action_runner->WantsToRun(extension2));

  // Unload one of the extensions.
  UnloadExtension(extension2->id());

  EXPECT_TRUE(RunAllPendingInRenderer(web_contents));

  // We should have pending requests for extension1, but not the removed
  // extension2.
  EXPECT_TRUE(action_runner->WantsToRun(extension1));
  EXPECT_FALSE(action_runner->WantsToRun(extension2));

  // We should still be able to run the request for extension1.
  ExtensionTestMessageListener inject_success_listener(
      new ExtensionTestMessageListener(kInjectSucceeded,
                                       false /* won't reply */));
  inject_success_listener.set_extension_id(extension1->id());
  action_runner->RunAction(extension1, true);
  EXPECT_TRUE(inject_success_listener.WaitUntilSatisfied());
}

// Test that granting the extension all urls permission allows it to run on
// pages, and that the permission update is sent to existing renderers.
IN_PROC_BROWSER_TEST_F(ExtensionActionRunnerBrowserTest,
                       GrantExtensionAllUrlsPermission) {
  // Loadup an extension and navigate.
  const Extension* extension =
      CreateExtension(ALL_HOSTS, CONTENT_SCRIPT, WITHHOLD_PERMISSIONS);
  ASSERT_TRUE(extension);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  ExtensionActionRunner* action_runner =
      ExtensionActionRunner::GetForWebContents(web_contents);
  ASSERT_TRUE(action_runner);

  ExtensionTestMessageListener inject_success_listener(
      new ExtensionTestMessageListener(kInjectSucceeded,
                                       false /* won't reply */));
  inject_success_listener.set_extension_id(extension->id());

  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL("/extensions/test_file.html");
  ui_test_utils::NavigateToURL(browser(), url);

  // The extension shouldn't be allowed to run.
  EXPECT_TRUE(action_runner->WantsToRun(extension));
  EXPECT_EQ(1, action_runner->num_page_requests());
  EXPECT_FALSE(inject_success_listener.was_satisfied());

  // Enable the extension to run on all urls.
  ScriptingPermissionsModifier modifier(profile(), extension);
  modifier.SetAllowedOnAllUrls(true);
  EXPECT_TRUE(RunAllPendingInRenderer(web_contents));

  // Navigate again - this time, the extension should execute immediately (and
  // should not need to ask the script controller for permission).
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(action_runner->WantsToRun(extension));
  EXPECT_EQ(0, action_runner->num_page_requests());
  EXPECT_TRUE(inject_success_listener.WaitUntilSatisfied());

  // Revoke all urls permissions.
  inject_success_listener.Reset();
  modifier.SetAllowedOnAllUrls(false);
  EXPECT_TRUE(RunAllPendingInRenderer(web_contents));

  // Re-navigate; the extension should again need permission to run.
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_TRUE(action_runner->WantsToRun(extension));
  EXPECT_EQ(1, action_runner->num_page_requests());
  EXPECT_FALSE(inject_success_listener.was_satisfied());
}

IN_PROC_BROWSER_TEST_F(ExtensionActionRunnerBrowserTest,
                       BlockedActionBrowserTest) {
  // Load an extension that wants to run on every page at document start, and
  // load a test page.
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url = embedded_test_server()->GetURL("/simple.html");
  const Extension* extension = LoadExtension(
      test_data_dir_.AppendASCII("blocked_actions/content_scripts"));
  ASSERT_TRUE(extension);
  ScriptingPermissionsModifier(profile(), extension).SetAllowedOnAllUrls(false);

  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));

  // The extension should want to run on the page, and should not have
  // injected.
  ExtensionActionRunner* runner =
      ExtensionActionRunner::GetForWebContents(web_contents);
  ASSERT_TRUE(runner);
  EXPECT_TRUE(runner->WantsToRun(extension));
  EXPECT_EQ("undefined", GetValue(web_contents));

  // Wire up the runner to automatically accept the bubble to prompt for page
  // refresh.
  runner->set_default_bubble_close_action_for_testing(
      std::make_unique<ToolbarActionsBarBubbleDelegate::CloseAction>(
          ToolbarActionsBarBubbleDelegate::CLOSE_EXECUTE));

  content::NavigationEntry* entry =
      web_contents->GetController().GetLastCommittedEntry();
  ASSERT_TRUE(entry);
  const int first_nav_id = entry->GetUniqueID();

  // Run the extension action, which should cause a page refresh (since we
  // automatically accepted the bubble prompting us), and the extension should
  // have injected at document start.
  runner->RunAction(extension, true);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  entry = web_contents->GetController().GetLastCommittedEntry();
  ASSERT_TRUE(entry);
  // Confirm that we refreshed the page.
  EXPECT_GE(entry->GetUniqueID(), first_nav_id);
  EXPECT_EQ("success", GetValue(web_contents));
  EXPECT_FALSE(runner->WantsToRun(extension));

  // Revoke permission and reload to try different bubble options.
  ActiveTabPermissionGranter* active_tab_granter =
      TabHelper::FromWebContents(web_contents)->active_tab_permission_granter();
  ASSERT_TRUE(active_tab_granter);
  active_tab_granter->RevokeForTesting();
  web_contents->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));

  // The extension should again want to run. Automatically dismiss the bubble
  // that pops up prompting for page refresh.
  EXPECT_TRUE(runner->WantsToRun(extension));
  EXPECT_EQ("undefined", GetValue(web_contents));
  const int next_nav_id =
      web_contents->GetController().GetLastCommittedEntry()->GetUniqueID();
  runner->set_default_bubble_close_action_for_testing(
      std::make_unique<ToolbarActionsBarBubbleDelegate::CloseAction>(
          ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_USER_ACTION));

  // Try running the extension. Nothing should happen, because the user
  // didn't agree to refresh the page. The extension should still want to run.
  runner->RunAction(extension, true);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ("undefined", GetValue(web_contents));
  EXPECT_EQ(
      next_nav_id,
      web_contents->GetController().GetLastCommittedEntry()->GetUniqueID());

  // Repeat with a dismissal from bubble deactivation - same story.
  runner->set_default_bubble_close_action_for_testing(
      std::make_unique<ToolbarActionsBarBubbleDelegate::CloseAction>(
          ToolbarActionsBarBubbleDelegate::CLOSE_DISMISS_DEACTIVATION));
  runner->RunAction(extension, true);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ("undefined", GetValue(web_contents));
  EXPECT_EQ(
      next_nav_id,
      web_contents->GetController().GetLastCommittedEntry()->GetUniqueID());
}

IN_PROC_BROWSER_TEST_F(ExtensionActionRunnerBrowserTest,
                       ScriptsExecuteWhenNoPermissionsWithheld) {
  // If we don't withhold permissions, extensions should execute normally.
  std::vector<std::unique_ptr<ActiveScriptTester>> testers;
  testers.push_back(std::make_unique<ActiveScriptTester>(
      "content_scripts_all_hosts",
      CreateExtension(ALL_HOSTS, CONTENT_SCRIPT, DONT_WITHHOLD_PERMISSIONS),
      browser(), DOES_NOT_REQUIRE_CONSENT, CONTENT_SCRIPT));
  testers.push_back(std::make_unique<ActiveScriptTester>(
      "inject_scripts_all_hosts",
      CreateExtension(ALL_HOSTS, EXECUTE_SCRIPT, DONT_WITHHOLD_PERMISSIONS),
      browser(), DOES_NOT_REQUIRE_CONSENT, EXECUTE_SCRIPT));

  ASSERT_TRUE(embedded_test_server()->Start());
  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/extensions/test_file.html"));

  for (const auto& tester : testers)
    EXPECT_TRUE(tester->Verify()) << tester->name();
}

// A version of the test with the flag off, in order to test that everything
// still works as expected.
class FlagOffExtensionActionRunnerBrowserTest
    : public ExtensionActionRunnerBrowserTest {
 private:
  // Simply don't append the flag.
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionBrowserTest::SetUpCommandLine(command_line);
  }
};

IN_PROC_BROWSER_TEST_F(FlagOffExtensionActionRunnerBrowserTest,
                       ScriptsExecuteWhenFlagAbsent) {
  std::vector<std::unique_ptr<ActiveScriptTester>> testers;
  testers.push_back(std::make_unique<ActiveScriptTester>(
      "content_scripts_all_hosts",
      CreateExtension(ALL_HOSTS, CONTENT_SCRIPT, DONT_WITHHOLD_PERMISSIONS),
      browser(), DOES_NOT_REQUIRE_CONSENT, CONTENT_SCRIPT));
  testers.push_back(std::make_unique<ActiveScriptTester>(
      "inject_scripts_all_hosts",
      CreateExtension(ALL_HOSTS, EXECUTE_SCRIPT, DONT_WITHHOLD_PERMISSIONS),
      browser(), DOES_NOT_REQUIRE_CONSENT, EXECUTE_SCRIPT));

  ASSERT_TRUE(embedded_test_server()->Start());
  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/extensions/test_file.html"));

  for (const auto& tester : testers)
    EXPECT_TRUE(tester->Verify()) << tester->name();
}

}  // namespace extensions
