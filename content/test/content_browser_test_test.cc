// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/content_browser_test.h"

#include <string>

#include "base/command_line.h"
#include "base/location.h"
#include "base/process/launch.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/launcher/test_launcher.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_launcher.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/shell/common/shell_switches.h"
#include "services/service_manager/sandbox/switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

// Disabled on official builds because symbolization in sandboxes processes
// opens up security holes.
// On Android symbolization happens in one step after all the tests ran, so this
// test doesn't work there.
// TODO(mac): figure out why symbolization doesn't happen in the renderer.
// http://crbug.com/521456
// TODO(win): send PDB files for component build. http://crbug.com/521459
#if !defined(OFFICIAL_BUILD) && !defined(OS_ANDROID) && !defined(OS_MACOSX) && \
    !(defined(COMPONENT_BUILD) && defined(OS_WIN))

IN_PROC_BROWSER_TEST_F(ContentBrowserTest, MANUAL_ShouldntRun) {
  // Ensures that tests with MANUAL_ prefix don't run automatically.
  ASSERT_TRUE(false);
}

IN_PROC_BROWSER_TEST_F(ContentBrowserTest, MANUAL_RendererCrash) {
  content::RenderProcessHostWatcher renderer_shutdown_observer(
      shell()->web_contents()->GetMainFrame()->GetProcess(),
      content::RenderProcessHostWatcher::WATCH_FOR_PROCESS_EXIT);

  NavigateToURL(shell(), GURL("chrome:crash"));
  renderer_shutdown_observer.Wait();

  EXPECT_FALSE(renderer_shutdown_observer.did_exit_normally());
}

// Non-Windows sanitizer builds do not symbolize stack traces internally, so use
// this macro to avoid looking for symbols from the stack trace.
#if !defined(OS_WIN) &&                                       \
    (defined(ADDRESS_SANITIZER) || defined(LEAK_SANITIZER) || \
     defined(MEMORY_SANITIZER) || defined(THREAD_SANITIZER))
#define USE_EXTERNAL_SYMBOLIZER 1
#else
#define USE_EXTERNAL_SYMBOLIZER 0
#endif

// Tests that browser tests print the callstack when a child process crashes.
IN_PROC_BROWSER_TEST_F(ContentBrowserTest, RendererCrashCallStack) {
  base::ThreadRestrictions::ScopedAllowIO allow_io_for_temp_dir;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::CommandLine new_test =
      base::CommandLine(base::CommandLine::ForCurrentProcess()->GetProgram());
  new_test.AppendSwitchASCII(base::kGTestFilterFlag,
                             "ContentBrowserTest.MANUAL_RendererCrash");
  new_test.AppendSwitch(kRunManualTestsFlag);
  new_test.AppendSwitch(kSingleProcessTestsFlag);

#if defined(THREAD_SANITIZER)
  // TSan appears to not be able to report intentional crashes from sandboxed
  // renderer processes.
  new_test.AppendSwitch(service_manager::switches::kNoSandbox);
#endif

  std::string output;
  base::GetAppOutputAndError(new_test, &output);

  // In sanitizer builds, an external script is responsible for symbolizing,
  // so the stack that the tests sees here looks like:
  // "#0 0x0000007ea911 (...content_browsertests+0x7ea910)"
  std::string crash_string =
#if !USE_EXTERNAL_SYMBOLIZER
      "content::RenderFrameImpl::HandleRendererDebugURL";
#else
      "#0 ";
#endif

  if (output.find(crash_string) == std::string::npos) {
    GTEST_FAIL() << "Couldn't find\n" << crash_string << "\n in output\n "
                 << output;
  }
}

IN_PROC_BROWSER_TEST_F(ContentBrowserTest, MANUAL_BrowserCrash) {
  CHECK(false);
}

// Tests that browser tests print the callstack on asserts.
IN_PROC_BROWSER_TEST_F(ContentBrowserTest, BrowserCrashCallStack) {
  base::ThreadRestrictions::ScopedAllowIO allow_io_for_temp_dir;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::CommandLine new_test =
      base::CommandLine(base::CommandLine::ForCurrentProcess()->GetProgram());
  new_test.AppendSwitchASCII(base::kGTestFilterFlag,
                             "ContentBrowserTest.MANUAL_BrowserCrash");
  new_test.AppendSwitch(kRunManualTestsFlag);
  new_test.AppendSwitch(kSingleProcessTestsFlag);
  std::string output;
  base::GetAppOutputAndError(new_test, &output);

  // In sanitizer builds, an external script is responsible for symbolizing,
  // so the stack that the test sees here looks like:
  // "#0 0x0000007ea911 (...content_browsertests+0x7ea910)"
  std::string crash_string =
#if !USE_EXTERNAL_SYMBOLIZER
      "content::ContentBrowserTest_MANUAL_BrowserCrash_Test::"
      "RunTestOnMainThread";
#else
      "#0 ";
#endif

  if (output.find(crash_string) == std::string::npos) {
    GTEST_FAIL() << "Couldn't find\n" << crash_string << "\n in output\n "
                 << output;
  }
}

#endif

class ContentBrowserTestSanityTest : public ContentBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    if (std::string(test_info->name()) == "SingleProcess")
      command_line->AppendSwitch(switches::kSingleProcess);
  }

  void Test() {
    GURL url = GetTestUrl(".", "simple_page.html");

    base::string16 expected_title(base::ASCIIToUTF16("OK"));
    TitleWatcher title_watcher(shell()->web_contents(), expected_title);
    NavigateToURL(shell(), url);
    base::string16 title = title_watcher.WaitAndGetTitle();
    EXPECT_EQ(expected_title, title);
  }
};

IN_PROC_BROWSER_TEST_F(ContentBrowserTestSanityTest, Basic) {
  Test();
}

IN_PROC_BROWSER_TEST_F(ContentBrowserTestSanityTest, SingleProcess) {
  Test();
}

namespace {

const base::Feature kTestFeatureForBrowserTest1{
    "TestFeatureForBrowserTest1", base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureForBrowserTest2{
    "TestFeatureForBrowserTest2", base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kTestFeatureForBrowserTest3{
    "TestFeatureForBrowserTest3", base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureForBrowserTest4{
    "TestFeatureForBrowserTest4", base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace

class ContentBrowserTestScopedFeatureListTest : public ContentBrowserTest {
 public:
  ContentBrowserTestScopedFeatureListTest() {
    scoped_feature_list_.InitWithFeatures({kTestFeatureForBrowserTest3},
                                          {kTestFeatureForBrowserTest4});
  }

  ~ContentBrowserTestScopedFeatureListTest() override {}

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ContentBrowserTestScopedFeatureListTest);
};

IN_PROC_BROWSER_TEST_F(ContentBrowserTestScopedFeatureListTest,
                       FeatureListTest) {
  EXPECT_TRUE(base::FeatureList::IsEnabled(kTestFeatureForBrowserTest1));
  EXPECT_FALSE(base::FeatureList::IsEnabled(kTestFeatureForBrowserTest2));
  EXPECT_TRUE(base::FeatureList::IsEnabled(kTestFeatureForBrowserTest3));
  EXPECT_FALSE(base::FeatureList::IsEnabled(kTestFeatureForBrowserTest4));
}

namespace {

void CallbackChecker(bool* non_nested_task_ran) {
  *non_nested_task_ran = true;
}

}  // namespace

IN_PROC_BROWSER_TEST_F(ContentBrowserTest, NonNestableTask) {
  bool non_nested_task_ran = false;
  base::ThreadTaskRunnerHandle::Get()->PostNonNestableTask(
      FROM_HERE, base::BindOnce(&CallbackChecker, &non_nested_task_ran));
  content::RunAllPendingInMessageLoop();
  ASSERT_TRUE(non_nested_task_ran);
}

}  // namespace content
