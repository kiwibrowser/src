// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/vr/test/vr_browser_test.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace vr {

constexpr base::TimeDelta VrXrBrowserTestBase::kPollCheckIntervalShort;
constexpr base::TimeDelta VrXrBrowserTestBase::kPollCheckIntervalLong;
constexpr base::TimeDelta VrXrBrowserTestBase::kPollTimeoutShort;
constexpr base::TimeDelta VrXrBrowserTestBase::kPollTimeoutLong;
constexpr char VrXrBrowserTestBase::kVrOverrideEnvVar[];
constexpr char VrXrBrowserTestBase::kVrOverrideVal[];
constexpr char VrXrBrowserTestBase::kVrConfigPathEnvVar[];
constexpr char VrXrBrowserTestBase::kVrConfigPathVal[];
constexpr char VrXrBrowserTestBase::kVrLogPathEnvVar[];
constexpr char VrXrBrowserTestBase::kVrLogPathVal[];

VrXrBrowserTestBase::VrXrBrowserTestBase()
    : env_(base::Environment::Create()) {}

VrXrBrowserTestBase::~VrXrBrowserTestBase() = default;

// We need an std::string that is an absolute file path, which requires
// platform-specific logic since Windows uses std::wstring instead of
// std::string for FilePaths, but SetVar only accepts std::string
#ifdef OS_WIN
#define MAKE_ABSOLUTE(x) \
  base::WideToUTF8(      \
      base::MakeAbsoluteFilePath(base::FilePath(base::UTF8ToWide(x))).value())
#else
#define MAKE_ABSOLUTE(x) base::MakeAbsoluteFilePath(base::FilePath(x)).value()
#endif

void VrXrBrowserTestBase::SetUp() {
  // Set the environment variable to use the mock OpenVR client
  EXPECT_TRUE(env_->SetVar(kVrOverrideEnvVar, MAKE_ABSOLUTE(kVrOverrideVal)));
  EXPECT_TRUE(
      env_->SetVar(kVrConfigPathEnvVar, MAKE_ABSOLUTE(kVrConfigPathVal)));
  EXPECT_TRUE(env_->SetVar(kVrLogPathEnvVar, MAKE_ABSOLUTE(kVrLogPathVal)));

  // Set any command line flags that subclasses have set, e.g. enabling WebVR
  // and OpenVR support
  for (const auto& switch_string : append_switches_) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(switch_string);
  }
  scoped_feature_list_.InitWithFeatures(enable_features_, {});

  InProcessBrowserTest::SetUp();
}

GURL VrXrBrowserTestBase::GetHtmlTestFile(const std::string& test_name) {
  return ui_test_utils::GetTestUrl(
      base::FilePath(FILE_PATH_LITERAL("vr/e2e_test_files/html")),
#ifdef OS_WIN
      base::FilePath(base::UTF8ToWide(test_name + ".html")
#else
      base::FilePath(test_name + ".html")
#endif
                         ));
}

content::WebContents* VrXrBrowserTestBase::GetFirstTabWebContents() {
  return browser()->tab_strip_model()->GetWebContentsAt(0);
}

void VrXrBrowserTestBase::LoadUrlAndAwaitInitialization(const GURL& url) {
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_TRUE(PollJavaScriptBoolean(
      "isInitializationComplete()", kPollTimeoutShort,
      browser()->tab_strip_model()->GetActiveWebContents()))
      << "Timed out waiting for JavaScript test initialization.";
}

VrXrBrowserTestBase::TestStatus VrXrBrowserTestBase::CheckTestStatus(
    content::WebContents* web_contents) {
  std::string result_string =
      RunJavaScriptAndExtractStringOrFail("resultString", web_contents);
  bool test_passed =
      RunJavaScriptAndExtractBoolOrFail("testPassed", web_contents);
  if (test_passed) {
    return VrXrBrowserTestBase::TestStatus::STATUS_PASSED;
  } else if (!test_passed && result_string == "") {
    return VrXrBrowserTestBase::TestStatus::STATUS_RUNNING;
  }
  // !test_passed && result_string != ""
  return VrXrBrowserTestBase::TestStatus::STATUS_FAILED;
}

void VrXrBrowserTestBase::EndTest(content::WebContents* web_contents) {
  switch (CheckTestStatus(web_contents)) {
    case VrXrBrowserTestBase::TestStatus::STATUS_PASSED:
      break;
    case VrXrBrowserTestBase::TestStatus::STATUS_FAILED:
      FAIL() << "JavaScript testharness failed with result: "
             << RunJavaScriptAndExtractStringOrFail("resultString",
                                                    web_contents);
      break;
    case VrXrBrowserTestBase::TestStatus::STATUS_RUNNING:
      FAIL() << "Attempted to end test in C++ without finishing in JavaScript";
      break;
    default:
      FAIL() << "Received unknown test status.";
  }
}

bool VrXrBrowserTestBase::PollJavaScriptBoolean(
    const std::string& bool_expression,
    const base::TimeDelta& timeout,
    content::WebContents* web_contents) {
  return BlockOnConditionUnsafe(
      base::BindRepeating(RunJavaScriptAndExtractBoolOrFail, bool_expression,
                          web_contents),
      timeout);
}

void VrXrBrowserTestBase::WaitOnJavaScriptStep(
    content::WebContents* web_contents) {
  // Make sure we aren't trying to wait on a JavaScript test step without the
  // code to do so.
  bool code_available = RunJavaScriptAndExtractBoolOrFail(
      "typeof javascriptDone !== 'undefined'", web_contents);
  EXPECT_TRUE(code_available) << "Attempted to wait on a JavaScript test step "
                              << "without the code to do so. You either forgot "
                              << "to import webvr_e2e.js or "
                              << "are incorrectly using a C++ function.";

  // Actually wait for the step to finish
  bool success =
      PollJavaScriptBoolean("javascriptDone", kPollTimeoutLong, web_contents);

  // Check what state we're in to make sure javascriptDone wasn't called
  // because the test failed.
  VrXrBrowserTestBase::TestStatus test_status = CheckTestStatus(web_contents);
  if (!success ||
      test_status == VrXrBrowserTestBase::TestStatus::STATUS_FAILED) {
    // Failure states: Either polling failed or polling succeeded, but because
    // the test failed.
    std::string reason;
    if (!success) {
      reason = "Polling JavaScript boolean javascriptDone timed out.";
    } else {
      reason =
          "Polling JavaScript boolean javascriptDone succeeded, but test "
          "failed.";
    }

    std::string result_string =
        RunJavaScriptAndExtractStringOrFail("resultString", web_contents);
    if (result_string == "") {
      reason += " Did not obtain specific reason from testharness.";
    } else {
      reason += " Testharness reported failure: " + result_string;
    }
    FAIL() << reason;
  }

  // Reset the synchronization boolean
  EXPECT_TRUE(content::ExecuteScript(web_contents, "javascriptDone = false"));
}

void VrXrBrowserTestBase::ExecuteStepAndWait(
    const std::string& step_function,
    content::WebContents* web_contents) {
  EXPECT_TRUE(content::ExecuteScript(web_contents, step_function));
  WaitOnJavaScriptStep(web_contents);
}

bool VrXrBrowserTestBase::BlockOnConditionUnsafe(
    base::RepeatingCallback<bool()> condition,
    const base::TimeDelta& timeout,
    const base::TimeDelta& period) {
  base::Time start = base::Time::Now();
  bool successful = false;
  while (base::Time::Now() - start < timeout) {
    successful = condition.Run();
    if (successful) {
      break;
    }
    base::PlatformThread::Sleep(period);
  }
  return successful;
}

bool VrXrBrowserTestBase::RunJavaScriptAndExtractBoolOrFail(
    const std::string& js_expression,
    content::WebContents* web_contents) {
  bool result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents,
      "window.domAutomationController.send(" + js_expression + ")", &result));
  return result;
}

std::string VrXrBrowserTestBase::RunJavaScriptAndExtractStringOrFail(
    const std::string& js_expression,
    content::WebContents* web_contents) {
  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents,
      "window.domAutomationController.send(" + js_expression + ")", &result));
  return result;
}

void VrXrBrowserTestBase::EnterPresentationAndWait(
    content::WebContents* web_contents) {
  EnterPresentation(web_contents);
  WaitOnJavaScriptStep(web_contents);
}

}  // namespace vr
