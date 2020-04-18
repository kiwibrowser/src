// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/common/extension_features.h"
#include "extensions/test/test_extension_dir.h"

namespace extensions {
namespace {

// TODO(jbroman, devlin): This should ultimately be replaced with some more
// sophisticated testing (e.g. in Telemetry) which is tracked on the perf bots.

// These tests are designed to exercise the extension API bindings
// system and measure performance with and without native bindings.
// They are designed to be run locally, and there isn't much benefit to
// running them on the bots. For this reason, they are all disabled.
// To run them, append the --gtest_also_run_disabled_tests flag to the
// test executable.
#define LOCAL_TEST(TestName) DISABLED_ ## TestName

enum BindingsType { NATIVE_BINDINGS, JAVASCRIPT_BINDINGS };

class APIBindingPerfBrowserTest
    : public ExtensionBrowserTest,
      public ::testing::WithParamInterface<BindingsType> {
 protected:
  APIBindingPerfBrowserTest() {}
  ~APIBindingPerfBrowserTest() override {}

  void SetUp() override {
    if (GetParam() == NATIVE_BINDINGS) {
      scoped_feature_list_.InitAndEnableFeature(features::kNativeCrxBindings);
    } else {
      DCHECK_EQ(JAVASCRIPT_BINDINGS, GetParam());
      scoped_feature_list_.InitAndDisableFeature(features::kNativeCrxBindings);
    }
    ExtensionBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    embedded_test_server()->ServeFilesFromDirectory(
        base::FilePath(FILE_PATH_LITERAL("chrome/test/data")));
    EXPECT_TRUE(embedded_test_server()->Start());
  }

  base::TimeDelta RunTestAndReportTime() {
    double time_elapsed_ms = 0;
    EXPECT_TRUE(content::ExecuteScriptAndExtractDouble(
        browser()->tab_strip_model()->GetActiveWebContents(),
        "runTest(time => window.domAutomationController.send(time))",
        &time_elapsed_ms));
    return base::TimeDelta::FromMillisecondsD(time_elapsed_ms);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(APIBindingPerfBrowserTest);
};

const char kSimpleContentScriptManifest[] =
    "{"
    "  'name': 'Perf test extension',"
    "  'version': '0',"
    "  'manifest_version': 2,"
    "  'content_scripts': [ {"
    "    'all_frames': true,"
    "    'matches': [ '<all_urls>' ],"
    "    'run_at': 'document_end',"
    "    'js': [ 'content_script.js' ]"
    "  } ],"
    "  'permissions': [ 'storage' ]"
    "}";

IN_PROC_BROWSER_TEST_P(APIBindingPerfBrowserTest,
                       LOCAL_TEST(ManyFramesWithNoContentScript)) {
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL(
                                   "/extensions/perf_tests/many_frames.html"));

  base::TimeDelta time_elapsed = RunTestAndReportTime();
  LOG(INFO) << "Executed in " << time_elapsed.InMillisecondsF() << " ms";
}

IN_PROC_BROWSER_TEST_P(APIBindingPerfBrowserTest,
                       LOCAL_TEST(ManyFramesWithEmptyContentScript)) {
  TestExtensionDir extension_dir;
  extension_dir.WriteManifestWithSingleQuotes(kSimpleContentScriptManifest);
  extension_dir.WriteFile(FILE_PATH_LITERAL("content_script.js"),
                          "// This space intentionally left blank.");
  ASSERT_TRUE(LoadExtension(extension_dir.UnpackedPath()));

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL(
                                   "/extensions/perf_tests/many_frames.html"));

  base::TimeDelta time_elapsed = RunTestAndReportTime();
  LOG(INFO) << "Executed in " << time_elapsed.InMillisecondsF() << " ms";
}

IN_PROC_BROWSER_TEST_P(APIBindingPerfBrowserTest,
                       LOCAL_TEST(ManyFramesWithStorageAndRuntime)) {
  TestExtensionDir extension_dir;
  extension_dir.WriteManifestWithSingleQuotes(kSimpleContentScriptManifest);
  extension_dir.WriteFile(FILE_PATH_LITERAL("content_script.js"),
                          "chrome.storage.onChanged.addListener;"
                          "chrome.runtime.onMessage.addListener;");
  ASSERT_TRUE(LoadExtension(extension_dir.UnpackedPath()));

  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL(
                                   "/extensions/perf_tests/many_frames.html"));

  base::TimeDelta time_elapsed = RunTestAndReportTime();
  LOG(INFO) << "Executed in " << time_elapsed.InMillisecondsF() << " ms";
}

INSTANTIATE_TEST_CASE_P(Native,
                        APIBindingPerfBrowserTest,
                        ::testing::Values(NATIVE_BINDINGS));
INSTANTIATE_TEST_CASE_P(JavaScript,
                        APIBindingPerfBrowserTest,
                        ::testing::Values(JAVASCRIPT_BINDINGS));

}  // namespace
}  // namespace extensions
