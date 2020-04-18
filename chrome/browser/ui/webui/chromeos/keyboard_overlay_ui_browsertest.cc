// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/accelerators.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

namespace {

class TestWebUIMessageHandler : public content::WebUIMessageHandler {
 public:
  TestWebUIMessageHandler() = default;
  ~TestWebUIMessageHandler() override = default;

  // content::WebUIMessageHandler:
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "didPaint",
        base::BindRepeating(&TestWebUIMessageHandler::HandleDidPaint,
                            base::Unretained(this)));
  }

 private:
  void HandleDidPaint(const base::ListValue*) {}

  DISALLOW_COPY_AND_ASSIGN(TestWebUIMessageHandler);
};

content::WebContents* StartKeyboardOverlayUI(Browser* browser) {
  ui_test_utils::NavigateToURL(browser,
                               GURL(chrome::kChromeUIKeyboardOverlayURL));
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  web_contents->GetWebUI()->AddMessageHandler(
      std::make_unique<TestWebUIMessageHandler>());
  return web_contents;
}

bool IsDisplayUIScalingEnabled(content::WebContents* web_contents) {
  bool is_display_ui_scaling_enabled;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents,
      "domAutomationController.send(isDisplayUIScalingEnabled());",
      &is_display_ui_scaling_enabled));
  return is_display_ui_scaling_enabled;
}

// Skip some accelerators in the tests:
// 1. If the accelerator has no modifier, i.e. ui::EF_NONE, or for "Caps
// Lock", such as ui::VKEY_MENU and ui::VKEY_LWIN, the logic to show it on
// the keyboard overlay is not by the mapping of
// keyboardOverlayData['shortcut'], so it can not be tested by this test.
// 2. If it has debug modifiers: ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN |
// ui::EF_SHIFT_DOWN
bool ShouldSkip(const ash::AcceleratorData& accelerator) {
  return accelerator.keycode == ui::VKEY_MENU ||
         accelerator.keycode == ui::VKEY_LWIN ||
         accelerator.modifiers == ui::EF_NONE ||
         accelerator.modifiers ==
             (ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN | ui::EF_SHIFT_DOWN);
}

std::string KeyboardCodeToLabel(const ash::AcceleratorData& accelerator,
                                content::WebContents* web_contents) {
  std::string label;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "domAutomationController.send("
      "  (function(number) {"
      "       if (!!KEYCODE_TO_LABEL[number]) {"
      "             return KEYCODE_TO_LABEL[number];"
      "       } else {"
      "             return 'NONE';"
      "       }"
      "  })(" +
          std::to_string(static_cast<unsigned int>(accelerator.keycode)) +
      "    )"
      ");",
      &label));
  if (label == "NONE") {
    label = base::ToLowerASCII(static_cast<char>(
        LocatedToNonLocatedKeyboardCode(accelerator.keycode)));
  }
  return label;
}

std::string GenerateShortcutKey(const ash::AcceleratorData& accelerator,
                                content::WebContents* web_contents) {
  std::string shortcut = KeyboardCodeToLabel(accelerator, web_contents);
  // The order of the "if" conditions should not be changed because the
  // modifiers are expected to be alphabetical sorted in the generated
  // shortcut.
  if (accelerator.modifiers & ui::EF_ALT_DOWN)
    shortcut.append("<>ALT");
  if (accelerator.modifiers & ui::EF_CONTROL_DOWN)
    shortcut.append("<>CTRL");
  if (accelerator.modifiers & ui::EF_COMMAND_DOWN)
    shortcut.append("<>SEARCH");
  if (accelerator.modifiers & ui::EF_SHIFT_DOWN)
    shortcut.append("<>SHIFT");
  return shortcut;
}

bool ContainsShortcut(const std::string& shortcut,
                      content::WebContents* web_contents) {
  bool contains;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents,
      "domAutomationController.send("
      "  !!keyboardOverlayData['shortcut']['" + shortcut + "']"
      ");",
      &contains));
  return contains;
}

}  // namespace

using KeyboardOverlayUIBrowserTest = InProcessBrowserTest;

// This test verifies two things:
//
// 1. That all accelerators in kAcceleratorData appear in the keyboard overlay
// UI. This will fail when a new shortcut is added (or replaced) in
// kAcceleratorData but not the overlay UI.
//
// 2. That the number of accelerators shared by the Ash table and the UI is the
// expected value. This will fail when a new shortcut is added to
// kAcceleratorData but not the overlay UI.
IN_PROC_BROWSER_TEST_F(KeyboardOverlayUIBrowserTest,
                       AcceleratorsShouldHaveKeyboardOverlay) {
  content::WebContents* const web_contents = StartKeyboardOverlayUI(browser());
  const bool is_display_ui_scaling_enabled =
      IsDisplayUIScalingEnabled(web_contents);
  int found_accelerators = 0;
  for (size_t i = 0; i < ash::kAcceleratorDataLength; ++i) {
    const ash::AcceleratorData& entry = ash::kAcceleratorData[i];
    if (ShouldSkip(entry))
      continue;

    const std::string shortcut = GenerateShortcutKey(entry, web_contents);
    if (!is_display_ui_scaling_enabled) {
      if (shortcut == "-<>CTRL<>SHIFT" || shortcut == "+<>CTRL<>SHIFT" ||
          shortcut == "0<>CTRL<>SHIFT") {
        continue;
      }
    }

    if (ContainsShortcut(shortcut, web_contents)) {
      found_accelerators++;
    } else {
      ADD_FAILURE() << "Please add the new accelerators to keyboard "
                       "overlay. Add one entry '" +
                           shortcut +
                           "' in the 'shortcut' section"
                           " at the bottom of the file of "
                           "'/chrome/browser/resources/chromeos/"
                           "keyboard_overlay_data.js'. Please keep it in "
                           "alphabetical order.";
    }
  }

  constexpr int kExpectedFoundAccelerators = 62;
  DCHECK_EQ(kExpectedFoundAccelerators, found_accelerators)
      << "It seems ash::kAcceleratorData or the 'shortcut' section of "
         "'/chrome/browser/resources/chromeos/keyboard_overlay_data.js' has "
         "changed. Please keep the two in sync. If you've deprecated an "
         "accelerator, remove it from keyboard_overlay_data.js. If you have "
         "added the accelerator in both places, update "
         "kExpectedFoundAccelerators.";
}
