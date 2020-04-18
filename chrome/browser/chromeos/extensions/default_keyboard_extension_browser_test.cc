/*
 * Copyright 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "chrome/browser/chromeos/extensions/default_keyboard_extension_browser_test.h"

#include <vector>

#include "ash/shell.h"
#include "base/command_line.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/common/extension.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/input_method.h"
#include "ui/keyboard/keyboard_switches.h"

namespace {
const base::FilePath::CharType kWebuiTestDir[] = FILE_PATH_LITERAL("webui");

const base::FilePath::CharType kMockController[] =
    FILE_PATH_LITERAL("mock_controller.js");

const base::FilePath::CharType kMockTimer[] =
    FILE_PATH_LITERAL("mock_timer.js");

const char kVirtualKeyboardExtensionTestDir[] =
    "chromeos/virtual_keyboard/default_extension";

const char kBaseKeyboardExtensionTestFramework[] =
    "virtual_keyboard_test_base.js";

const char kExtensionId[] = "mppnpdlheglhdfmldimlhpnegondlapf";

// Loading the virtual keyboard with id=none suppresses asynchronous loading of
// layout and configuration assets. This allows the javascript test code to be
// injected ahead of the keyboard initialization.
const char kVirtualKeyboardURL[] = "chrome://keyboard?id=none";

}  // namespace

DefaultKeyboardExtensionBrowserTestConfig::
    DefaultKeyboardExtensionBrowserTestConfig()
    : base_framework_(kBaseKeyboardExtensionTestFramework),
      extension_id_(kExtensionId),
      test_dir_(kVirtualKeyboardExtensionTestDir),
      url_(kVirtualKeyboardURL) {}

DefaultKeyboardExtensionBrowserTestConfig::
    ~DefaultKeyboardExtensionBrowserTestConfig() {}

void DefaultKeyboardExtensionBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  command_line->AppendSwitch(keyboard::switches::kEnableVirtualKeyboard);
}

void DefaultKeyboardExtensionBrowserTest::RunTest(
    const base::FilePath& file,
    const DefaultKeyboardExtensionBrowserTestConfig& config) {
  ui_test_utils::NavigateToURL(browser(), GURL(config.url_));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  ASSERT_TRUE(web_contents);

  // Inject testing scripts.
  InjectJavascript(base::FilePath(kWebuiTestDir),
                   base::FilePath(kMockController));
  InjectJavascript(base::FilePath(kWebuiTestDir), base::FilePath(kMockTimer));
  InjectJavascript(base::FilePath(FILE_PATH_LITERAL(config.test_dir_)),
                   base::FilePath(FILE_PATH_LITERAL(config.base_framework_)));
  InjectJavascript(base::FilePath(FILE_PATH_LITERAL(config.test_dir_)), file);

  ASSERT_TRUE(content::ExecuteScript(web_contents, utf8_content_));

  // Inject DOM-automation test harness and run tests.
  std::vector<int> resource_ids;
  EXPECT_TRUE(ExecuteWebUIResourceTest(web_contents, resource_ids));
}

void DefaultKeyboardExtensionBrowserTest::ShowVirtualKeyboard() {
  aura::Window* window = ash::Shell::GetPrimaryRootWindow();
  ui::InputMethod* input_method = window->GetHost()->GetInputMethod();
  ASSERT_TRUE(input_method);
  input_method->ShowImeIfNeeded();
}

content::RenderViewHost*
DefaultKeyboardExtensionBrowserTest::GetKeyboardRenderViewHost(
    const std::string& id) {
  ShowVirtualKeyboard();
  GURL url = extensions::Extension::GetBaseURLFromExtensionId(id);
  std::unique_ptr<content::RenderWidgetHostIterator> widgets(
      content::RenderWidgetHost::GetRenderWidgetHosts());
  while (content::RenderWidgetHost* widget = widgets->GetNextHost()) {
    content::RenderViewHost* view = content::RenderViewHost::From(widget);
    if (view && url == view->GetSiteInstance()->GetSiteURL()) {
      content::WebContents* wc = content::WebContents::FromRenderViewHost(view);
      // Waits for virtual keyboard to load.
      EXPECT_TRUE(content::WaitForLoadStop(wc));
      return view;
    }
  }
  LOG(ERROR) << "Extension not found:" << url;
  return NULL;
}

void DefaultKeyboardExtensionBrowserTest::InjectJavascript(
    const base::FilePath& dir,
    const base::FilePath& file) {
  base::FilePath path = ui_test_utils::GetTestFilePath(dir, file);
  std::string library_content;
  ASSERT_TRUE(base::ReadFileToString(path, &library_content)) << path.value();
  utf8_content_.append(library_content);
  utf8_content_.append(";\n");
}

IN_PROC_BROWSER_TEST_F(DefaultKeyboardExtensionBrowserTest, TypingTest) {
  RunTest(base::FilePath(FILE_PATH_LITERAL("typing_test.js")),
          DefaultKeyboardExtensionBrowserTestConfig());
}

IN_PROC_BROWSER_TEST_F(DefaultKeyboardExtensionBrowserTest, LayoutTest) {
  RunTest(base::FilePath(FILE_PATH_LITERAL("layout_test.js")),
          DefaultKeyboardExtensionBrowserTestConfig());
}

IN_PROC_BROWSER_TEST_F(DefaultKeyboardExtensionBrowserTest, ModifierTest) {
  RunTest(base::FilePath(FILE_PATH_LITERAL("modifier_test.js")),
          DefaultKeyboardExtensionBrowserTestConfig());
}

IN_PROC_BROWSER_TEST_F(DefaultKeyboardExtensionBrowserTest,
                       HideKeyboardKeyTest) {
  RunTest(base::FilePath(FILE_PATH_LITERAL("hide_keyboard_key_test.js")),
          DefaultKeyboardExtensionBrowserTestConfig());
}

IN_PROC_BROWSER_TEST_F(DefaultKeyboardExtensionBrowserTest, IsKeyboardLoaded) {
  content::RenderViewHost* keyboard_rvh =
      GetKeyboardRenderViewHost(kExtensionId);
  ASSERT_TRUE(keyboard_rvh);
  bool loaded = false;
  std::string script = "!!chrome.virtualKeyboardPrivate";
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      keyboard_rvh, "window.domAutomationController.send(" + script + ");",
      &loaded));
  // Catches the regression in crbug.com/308653.
  ASSERT_TRUE(loaded);
}

// Disabled; http://crbug.com/515596
IN_PROC_BROWSER_TEST_F(DefaultKeyboardExtensionBrowserTest,
                       DISABLED_EndToEndTest) {
  // Get the virtual keyboard's render view host.
  content::RenderViewHost* keyboard_rvh =
      GetKeyboardRenderViewHost(kExtensionId);
  ASSERT_TRUE(keyboard_rvh);

  // Get the test page's render view host.
  content::RenderViewHost* browser_rvh =
      browser()->tab_strip_model()->GetActiveWebContents()->GetRenderViewHost();
  ASSERT_TRUE(browser_rvh);

  // Set up the test page.
  GURL url = ui_test_utils::GetTestUrl(
      base::FilePath(),
      base::FilePath(FILE_PATH_LITERAL(
          "chromeos/virtual_keyboard/default_extension/end_to_end_test.html")));
  ui_test_utils::NavigateToURL(browser(), url);

  // Press 'a' on keyboard.
  base::FilePath path = ui_test_utils::GetTestFilePath(
      base::FilePath(FILE_PATH_LITERAL(kVirtualKeyboardExtensionTestDir)),
      base::FilePath(FILE_PATH_LITERAL("end_to_end_test.js")));
  std::string script;
  ASSERT_TRUE(base::ReadFileToString(path, &script));
  EXPECT_TRUE(content::ExecuteScript(keyboard_rvh, script));
  // Verify 'a' appeared on test page.
  bool success = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      browser_rvh, "success ? verifyInput('a') : waitForInput('a');",
      &success));
  ASSERT_TRUE(success);
}

// TODO(kevers|rsadam|bshe):  Add UI tests for remaining virtual keyboard
// functionality.
