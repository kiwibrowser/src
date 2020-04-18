// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "chrome/browser/chromeos/input_method/textinput_test_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/aura/window_tree_host.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_resource_util.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_test_util.h"
#include "ui/keyboard/keyboard_util.h"

namespace {

// Simulates a click on the middle of the DOM element with the given |id|.
void ClickElementWithId(content::WebContents* web_contents,
                        const std::string& id) {
  int x;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      web_contents,
      "var bounds = document.getElementById('" + id +
          "').getBoundingClientRect();"
          "domAutomationController.send("
          "    Math.floor(bounds.left + bounds.width / 2));",
      &x));
  int y;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      web_contents,
      "var bounds = document.getElementById('" + id +
          "').getBoundingClientRect();"
          "domAutomationController.send("
          "    Math.floor(bounds.top + bounds.height / 2));",
      &y));
  content::SimulateMouseClickAt(
      web_contents, 0, blink::WebMouseEvent::Button::kLeft, gfx::Point(x, y));
}

}  // namespace

namespace keyboard {

class KeyboardEndToEndTest : public InProcessBrowserTest {
 public:
  KeyboardEndToEndTest() {}
  ~KeyboardEndToEndTest() override {}

  // Ensure that the virtual keyboard is enabled.
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(keyboard::switches::kEnableVirtualKeyboard);
  }

  void SetUpOnMainThread() override {
    GURL test_url =
        ui_test_utils::GetTestUrl(base::FilePath("chromeos/virtual_keyboard"),
                                  base::FilePath("inputs.html"));
    ui_test_utils::NavigateToURL(browser(), test_url);
    web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(web_contents);

    EXPECT_FALSE(IsKeyboardVisible());
  }

 protected:
  // Initialized in |SetUpOnMainThread|.
  content::WebContents* web_contents;

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardEndToEndTest);
};

IN_PROC_BROWSER_TEST_F(KeyboardEndToEndTest, OpenIfFocusedOnClick) {
  ClickElementWithId(web_contents, "text");

  ASSERT_TRUE(keyboard::WaitUntilShown());
  EXPECT_TRUE(IsKeyboardVisible());

  ClickElementWithId(web_contents, "blur");
  ASSERT_TRUE(keyboard::WaitUntilHidden());
  EXPECT_FALSE(IsKeyboardVisible());
}

IN_PROC_BROWSER_TEST_F(KeyboardEndToEndTest, OpenOnlyOnSyncFocus) {
  auto* controller = keyboard::KeyboardController::GetInstance();
  EXPECT_FALSE(IsKeyboardVisible());

  chromeos::TextInputTestHelper helper;

  ClickElementWithId(web_contents, "blur");
  helper.WaitForTextInputStateChanged(ui::TEXT_INPUT_TYPE_NONE);

  ClickElementWithId(web_contents, "sync");
  helper.WaitForTextInputStateChanged(ui::TEXT_INPUT_TYPE_TEXT);

  EXPECT_EQ(controller->GetStateForTest(),
            keyboard::KeyboardControllerState::LOADING_EXTENSION);

  EXPECT_TRUE(keyboard::WaitUntilShown());

  ClickElementWithId(web_contents, "blur");
  ASSERT_TRUE(keyboard::WaitUntilHidden());

  // If async focus occurs quickly after blur, then it should still invoke the
  // keyboard.
  ClickElementWithId(web_contents, "async");
  helper.WaitForTextInputStateChanged(ui::TEXT_INPUT_TYPE_TEXT);
  EXPECT_TRUE(IsKeyboardVisible());

  ClickElementWithId(web_contents, "blur");
  ASSERT_TRUE(keyboard::WaitUntilHidden());
  helper.WaitForPassageOfTimeMillis(3600);

  ClickElementWithId(web_contents, "async");
  helper.WaitForTextInputStateChanged(ui::TEXT_INPUT_TYPE_TEXT);
  EXPECT_EQ(controller->GetStateForTest(),
            keyboard::KeyboardControllerState::HIDDEN);
}

}  // namespace keyboard
