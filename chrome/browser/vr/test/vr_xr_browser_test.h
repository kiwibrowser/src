// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_VR_XR_BROWSER_TEST_H_
#define CHROME_BROWSER_VR_TEST_VR_XR_BROWSER_TEST_H_

#include "base/callback.h"
#include "base/environment.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "url/gurl.h"

#define REQUIRES_GPU(x) DISABLED_##x

namespace vr {

// Base browser test class for running VR/XR-related tests.
// This is essentially a C++ port of the way Android does similar tests in
// //chrome/android/javatests/src/.../browser/vr_shell/TestFramework.java
// This must be subclassed for VR or XR in order to handle the few differences
// between WebVR and WebXR.
class VrXrBrowserTestBase : public InProcessBrowserTest {
 public:
  static constexpr base::TimeDelta kPollCheckIntervalShort =
      base::TimeDelta::FromMilliseconds(50);
  static constexpr base::TimeDelta kPollCheckIntervalLong =
      base::TimeDelta::FromMilliseconds(100);
  static constexpr base::TimeDelta kPollTimeoutShort =
      base::TimeDelta::FromMilliseconds(1000);
  static constexpr base::TimeDelta kPollTimeoutLong =
      base::TimeDelta::FromMilliseconds(10000);
  static constexpr char kVrOverrideEnvVar[] = "VR_OVERRIDE";
  static constexpr char kVrOverrideVal[] = "./mock_vr_clients/";
  static constexpr char kVrConfigPathEnvVar[] = "VR_CONFIG_PATH";
  static constexpr char kVrConfigPathVal[] = "./";
  static constexpr char kVrLogPathEnvVar[] = "VR_LOG_PATH";
  static constexpr char kVrLogPathVal[] = "./";
  enum class TestStatus {
    STATUS_RUNNING = 0,
    STATUS_PASSED = 1,
    STATUS_FAILED = 2
  };

  VrXrBrowserTestBase();
  ~VrXrBrowserTestBase() override;

  void SetUp() override;

  // Returns a GURL to the VR test HTML file of the given name, e.g.
  // GetHtmlTestFile("foo") returns a GURL for the foo.html file in the VR
  // test HTML directory.
  GURL GetHtmlTestFile(const std::string& test_name);

  // Convenience function for accessing the WebContents belonging to the first
  // tab open in the browser.
  content::WebContents* GetFirstTabWebContents();

  // Loads the given GURL and blocks until the JavaScript on the page has
  // signalled that pre-test initialization is complete.
  void LoadUrlAndAwaitInitialization(const GURL& url);

  // Retrieves the current status of the JavaScript test and returns an enum
  // corresponding to it.
  static TestStatus CheckTestStatus(content::WebContents* web_contents);

  // Asserts that the JavaScript test code completed successfully.
  static void EndTest(content::WebContents* web_contents);

  // Blocks until the given JavaScript expression evaluates to true or the
  // timeout is reached. Returns true if the expression evaluated to true or
  // false on timeout.
  static bool PollJavaScriptBoolean(const std::string& bool_expression,
                                    const base::TimeDelta& timeout,
                                    content::WebContents* web_contents);

  // Blocks until the JavaScript in the given WebContents signals that it is
  // finished.
  static void WaitOnJavaScriptStep(content::WebContents* web_contents);

  // Executes the given step/JavaScript expression and blocks until JavaScript
  // signals that it is finished.
  static void ExecuteStepAndWait(const std::string& step_function,
                                 content::WebContents* web_contents);

  // Blocks until the given callback returns true or the timeout is reached.
  // Returns true if the condition successfully resolved or false on timeout.
  // This is unsafe because it relies on the provided callback checking a result
  // from a different thread. This isn't an issue when blocking on some
  // JavaScript condition to be true, but could be problematic if forced into
  // use elsewhere.
  static bool BlockOnConditionUnsafe(
      base::RepeatingCallback<bool()> condition,
      const base::TimeDelta& timeout = kPollTimeoutLong,
      const base::TimeDelta& period = kPollCheckIntervalLong);

  // Convenience function for ensuring ExecuteScriptAndExtractBool runs
  // successfully and for directly getting the result instead of needing to pass
  // a pointer to be filled.
  static bool RunJavaScriptAndExtractBoolOrFail(
      const std::string& js_expression,
      content::WebContents* web_contents);

  // Convenience function for ensuring ExecuteScripteAndExtractString runs
  // successfully and for directly getting the result instead of needing to pass
  // a pointer to be filled.
  static std::string RunJavaScriptAndExtractStringOrFail(
      const std::string& js_expression,
      content::WebContents* web_contents);

  // Enters either WebVR presentation or WebXR presentation (exclusive session).
  virtual void EnterPresentation(content::WebContents* web_contents) = 0;

  // Enters either WebVR presentation or WebXR presentation and waits for the
  // JavaScript step to end.
  void EnterPresentationAndWait(content::WebContents* web_contents);

  // Enters either WebVR presentation or WebXR presentation, failing the test if
  // it is not able to.
  virtual void EnterPresentationOrFail(content::WebContents* web_contents) = 0;

  // Exits either WebVR presentation or WebXR presentation (exclusive session).
  virtual void ExitPresentation(content::WebContents* web_contents) = 0;

  // Exits either WebVR presentation or WebXR presentation and waits for the
  // JavaScript step to end.
  void ExitPresentationAndWait(content::WebContents* web_contents);

  // Exits either WebVR presentation or WebXR presentation, failing the test if
  // it is not able to.
  virtual void ExitPresentationOrFail(content::WebContents* web_contents) = 0;

  Browser* browser() { return InProcessBrowserTest::browser(); }

 protected:
  std::unique_ptr<base::Environment> env_;
  std::vector<base::Feature> enable_features_;
  std::vector<std::string> append_switches_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  DISALLOW_COPY_AND_ASSIGN(VrXrBrowserTestBase);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_VR_XR_BROWSER_TEST_H_
