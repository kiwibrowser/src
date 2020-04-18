// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/dom_distiller/content/browser/distillable_page_utils.h"
#include "components/dom_distiller/core/dom_distiller_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace dom_distiller {

using ::testing::_;
using namespace switches::reader_mode_heuristics;

namespace {

const char kSimpleArticlePath[] = "/dom_distiller/simple_article.html";
const char kSimpleArticleIFramePath[] =
    "/dom_distiller/simple_article_iframe.html";
const char kArticlePath[] = "/dom_distiller/og_article.html";
const char kNonArticlePath[] = "/dom_distiller/non_og_article.html";

const char* kAllPaths[] = {
    kSimpleArticlePath,
    kSimpleArticleIFramePath,
    kArticlePath,
    kNonArticlePath
};

class Holder {
 public:
  virtual ~Holder() {}
  virtual void OnResult(bool, bool, bool) = 0;
};

class MockDelegate : public Holder {
 public:
  MOCK_METHOD3(OnResult, void(bool, bool, bool));

  base::RepeatingCallback<void(bool, bool, bool)> GetDelegate() {
    return base::BindRepeating(&MockDelegate::OnResult, base::Unretained(this));
  }
};

// Wait a bit to make sure there are no extra calls after the last expected
// call. All the expected calls happen within ~1ms on linux release build,
// so 100ms should be pretty safe to catch extra calls.
// If there are no extra calls, changing this doesn't change the test result.
const int kWaitAfterLastCallMs = 100;

// If there are no expected calls, the test wait for a while to make sure there
// are // no calls in this period of time. When there are expected calls, they
// happen within 100ms after content::WaitForLoadStop() on linux release build,
// and 10X safety margin is used.
// If there are no extra calls, changing this doesn't change the test result.
const int kWaitNoExpectedCallMs = 1000;

// QuitWhenIdleClosure() would become no-op if it is called before
// content::RunMessageLoop(). This timeout should be long enough to make sure
// at least one QuitWhenIdleClosure() is called after RunMessageLoop().
// All tests are limited by |kWaitAfterLastCall| or |kWaitNoExpectedCall|, so
// making this longer doesn't actually make tests run for longer, unless
// |kWaitAfterLastCall| or |kWaitNoExpectedCall| are so small or the test is so
// slow, for example, on Dr. Memory or Android, that QuitWhenIdleClosure()
// is called prematurely. 100X safety margin is used.
const int kDefaultTimeoutMs = 10000;

void QuitAfter(int time_ms) {
  DCHECK_GE(time_ms, 0);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated(),
      base::TimeDelta::FromMilliseconds(time_ms));
}

void QuitSoon() {
  QuitAfter(kWaitAfterLastCallMs);
}

}  // namespace

template<const char Option[]>
class DistillablePageUtilsBrowserTestOption : public InProcessBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kEnableDomDistiller);
    command_line->AppendSwitchASCII(switches::kReaderModeHeuristics,
        Option);
    command_line->AppendSwitch(switches::kEnableDistillabilityService);
  }

  void SetUpOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->Start());
    web_contents_ =
        browser()->tab_strip_model()->GetActiveWebContents();
    setDelegate(web_contents_, holder_.GetDelegate());
  }

  void NavigateAndWait(const char* url, int timeout_ms) {
    GURL article_url(url);
    if (base::StartsWith(url, "/", base::CompareCase::SENSITIVE)) {
      article_url = embedded_test_server()->GetURL(url);
    }

    // This blocks until the navigation has completely finished.
    ui_test_utils::NavigateToURL(browser(), article_url);
    content::WaitForLoadStop(web_contents_);

    QuitAfter(kDefaultTimeoutMs);
    if (timeout_ms) {
      // Local time-out for the tests that don't expect callbacks.
      QuitAfter(timeout_ms);
    }
    content::RunMessageLoop();
  }

  MockDelegate holder_;
  content::WebContents* web_contents_;
};


using DistillablePageUtilsBrowserTestAlways =
    DistillablePageUtilsBrowserTestOption<kAlwaysTrue>;

IN_PROC_BROWSER_TEST_F(DistillablePageUtilsBrowserTestAlways,
                       TestDelegate) {
  for (unsigned i = 0; i < sizeof(kAllPaths) / sizeof(kAllPaths[0]); ++i) {
    testing::InSequence dummy;
    EXPECT_CALL(holder_, OnResult(true, true, _))
        .WillOnce(testing::InvokeWithoutArgs(QuitSoon));
    NavigateAndWait(kAllPaths[i], 0);
  }
  // Test pages that we don't care about its distillability.
  {
    testing::InSequence dummy;
    EXPECT_CALL(holder_, OnResult(_, _, _)).Times(0);
    NavigateAndWait("about:blank", kWaitNoExpectedCallMs);
  }
}


using DistillablePageUtilsBrowserTestNone =
    DistillablePageUtilsBrowserTestOption<kNone>;

IN_PROC_BROWSER_TEST_F(DistillablePageUtilsBrowserTestNone,
                       TestDelegate) {
  EXPECT_CALL(holder_, OnResult(_, _, _)).Times(0);
  NavigateAndWait(kSimpleArticlePath, kWaitNoExpectedCallMs);
}


using DistillablePageUtilsBrowserTestOG =
    DistillablePageUtilsBrowserTestOption<kOGArticle>;

IN_PROC_BROWSER_TEST_F(DistillablePageUtilsBrowserTestOG,
                       TestDelegate) {
  {
    testing::InSequence dummy;
    EXPECT_CALL(holder_, OnResult(true, true, _))
        .WillOnce(testing::InvokeWithoutArgs(QuitSoon));
    NavigateAndWait(kArticlePath, 0);
  }
  {
    testing::InSequence dummy;
    EXPECT_CALL(holder_, OnResult(false, true, _))
        .WillOnce(testing::InvokeWithoutArgs(QuitSoon));
    NavigateAndWait(kNonArticlePath, 0);
  }
}


using DistillablePageUtilsBrowserTestAdaboost =
    DistillablePageUtilsBrowserTestOption<kAdaBoost>;

IN_PROC_BROWSER_TEST_F(DistillablePageUtilsBrowserTestAdaboost,
                       TestDelegate) {
  const char* paths[] = {kSimpleArticlePath, kSimpleArticleIFramePath};
  for (unsigned i = 0; i < sizeof(paths)/sizeof(paths[0]); ++i) {
    testing::InSequence dummy;
    EXPECT_CALL(holder_, OnResult(true, false, false)).Times(1);
    EXPECT_CALL(holder_, OnResult(true, true, false))
        .WillOnce(testing::InvokeWithoutArgs(QuitSoon));
    NavigateAndWait(paths[i], 0);
  }
  {
    testing::InSequence dummy;
    EXPECT_CALL(holder_, OnResult(false, false, false)).Times(1);
    EXPECT_CALL(holder_, OnResult(false, true, false))
        .WillOnce(testing::InvokeWithoutArgs(QuitSoon));
    NavigateAndWait(kNonArticlePath, 0);
  }
}

using DistillablePageUtilsBrowserTestAllArticles =
    DistillablePageUtilsBrowserTestOption<kAllArticles>;

IN_PROC_BROWSER_TEST_F(DistillablePageUtilsBrowserTestAllArticles,
                       TestDelegate) {
  const char* paths[] = {kSimpleArticlePath, kSimpleArticleIFramePath};
  for (unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
    testing::InSequence dummy;
    EXPECT_CALL(holder_, OnResult(true, false, false)).Times(1);
    EXPECT_CALL(holder_, OnResult(true, true, false))
        .WillOnce(testing::InvokeWithoutArgs(QuitSoon));
    NavigateAndWait(paths[i], 0);
  }
  {
    testing::InSequence dummy;
    EXPECT_CALL(holder_, OnResult(false, false, false)).Times(1);
    EXPECT_CALL(holder_, OnResult(false, true, false))
        .WillOnce(testing::InvokeWithoutArgs(QuitSoon));
    NavigateAndWait(kNonArticlePath, 0);
  }
}

}  // namespace dom_distiller
