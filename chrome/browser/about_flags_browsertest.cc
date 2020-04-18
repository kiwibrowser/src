// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/base/window_open_disposition.h"

namespace {

const char kSwitchName[] = "unsafely-treat-insecure-origin-as-secure";

void SimulateTextType(content::WebContents* contents,
                      const char* experiment_id,
                      const char* text) {
  EXPECT_TRUE(content::ExecuteScript(
      contents, base::StringPrintf(
                    "var parent = document.getElementById('%s');"
                    "var textarea = parent.getElementsByTagName('textarea')[0];"
                    "textarea.focus();"
                    "textarea.value = `%s`;"
                    "textarea.onchange();",
                    experiment_id, text)));
}

void ToggleEnableDropdown(content::WebContents* contents,
                          const char* experiment_id,
                          bool enable) {
  EXPECT_TRUE(content::ExecuteScript(
      contents,
      base::StringPrintf(
          "var k = "
          "document.getElementById('%s');"
          "var s = k.getElementsByClassName('experiment-enable-disable')[0];"
          "s.focus();"
          "s.selectedIndex = %d;"
          "s.onchange();",
          experiment_id, enable ? 1 : 0)));
}

void SetSwitch(base::CommandLine::SwitchMap* switch_map,
               const std::string& switch_name,
               const std::string& switch_value) {
#if defined(OS_WIN)
  (*switch_map)[switch_name] = base::ASCIIToUTF16(switch_value.c_str());
#else
  (*switch_map)[switch_name] = switch_value;
#endif
}

class AboutFlagsBrowserTest : public InProcessBrowserTest {};

// Tests experiments with origin values in chrome://flags page.
IN_PROC_BROWSER_TEST_F(AboutFlagsBrowserTest, OriginFlag) {
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://flags"));

  const base::CommandLine::SwitchMap switches =
      base::CommandLine::ForCurrentProcess()->GetSwitches();

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Type a value in the experiment's textarea. Since the flag state is
  // "Disabled" by default, command line shouldn't change.
  SimulateTextType(contents, kSwitchName, "http://example.test");
  EXPECT_EQ(switches, base::CommandLine::ForCurrentProcess()->GetSwitches());

  // Enable the experiment. Command line should change.
  ToggleEnableDropdown(contents, kSwitchName, true);
  base::CommandLine::SwitchMap expected_switches = switches;
  SetSwitch(&expected_switches, kSwitchName, "http://example.test");
  EXPECT_EQ(expected_switches,
            base::CommandLine::ForCurrentProcess()->GetSwitches());

  // Typing while enabled should immediately change the flag.
  SimulateTextType(contents, kSwitchName, "http://example.test.com");
  SetSwitch(&expected_switches, kSwitchName, "http://example.test.com");
  EXPECT_EQ(expected_switches,
            base::CommandLine::ForCurrentProcess()->GetSwitches());

  // Disable the experiment. Command line switch should be cleared.
  ToggleEnableDropdown(contents, kSwitchName, false);
  expected_switches.erase(kSwitchName);
  EXPECT_EQ(expected_switches,
            base::CommandLine::ForCurrentProcess()->GetSwitches());

  // Enable again. Command line switch should be added back.
  ToggleEnableDropdown(contents, kSwitchName, true);
  SetSwitch(&expected_switches, kSwitchName, "http://example.test.com");
  EXPECT_EQ(expected_switches,
            base::CommandLine::ForCurrentProcess()->GetSwitches());

  // Disable again and type. Command line switch should stay cleared.
  ToggleEnableDropdown(contents, kSwitchName, false);
  SimulateTextType(contents, kSwitchName, "http://example.test2.com");
  expected_switches.erase(kSwitchName);
  EXPECT_EQ(expected_switches,
            base::CommandLine::ForCurrentProcess()->GetSwitches());

  // Enable one last time. Command line should pick up the last typed value.
  ToggleEnableDropdown(contents, kSwitchName, true);
  SetSwitch(&expected_switches, kSwitchName, "http://example.test2.com");
  EXPECT_EQ(expected_switches,
            base::CommandLine::ForCurrentProcess()->GetSwitches());
}

// Tests that only valid http and https origins should be added to the command
// line when modified from chrome://flags.
IN_PROC_BROWSER_TEST_F(AboutFlagsBrowserTest, StringFlag) {
  ui_test_utils::NavigateToURL(browser(), GURL("chrome://flags"));

  const base::CommandLine::SwitchMap switches =
      base::CommandLine::ForCurrentProcess()->GetSwitches();

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  const char kValue[] =
      "http://example.test/path    http://example2.test/?query\n"
      "invalid-value, filesystem:http://example.test.file, "
      "ws://example3.test http://&^.com";

  ToggleEnableDropdown(contents, kSwitchName, true);
  SimulateTextType(contents, kSwitchName, kValue);
  base::CommandLine::SwitchMap expected_switches = switches;
  SetSwitch(&expected_switches, kSwitchName,
            "http://example.test,http://example2.test,ws://example3.test");
  EXPECT_EQ(expected_switches,
            base::CommandLine::ForCurrentProcess()->GetSwitches());
}

}  // namespace
