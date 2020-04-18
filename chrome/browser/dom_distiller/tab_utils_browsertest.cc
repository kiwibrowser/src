// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/dom_distiller/tab_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/dom_distiller/content/browser/distiller_javascript_utils.h"
#include "components/dom_distiller/content/browser/web_contents_main_frame_observer.h"
#include "components/dom_distiller/core/dom_distiller_service.h"
#include "components/dom_distiller/core/dom_distiller_switches.h"
#include "components/dom_distiller/core/task_tracker.h"
#include "components/dom_distiller/core/url_constants.h"
#include "components/dom_distiller/core/url_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace dom_distiller {

namespace {
const char* kSimpleArticlePath = "/dom_distiller/simple_article.html";
}  // namespace

class DomDistillerTabUtilsBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    if (!DistillerJavaScriptWorldIdIsSet()) {
      SetDistillerJavaScriptWorldId(content::ISOLATED_WORLD_ID_CONTENT_END);
    }
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kEnableDomDistiller);
  }
};

// WebContentsMainFrameHelper is used to detect if a distilled page has
// finished loading. This is done by checking how many times the title has
// been set rather than using "DidFinishLoad" directly due to the content
// being set by JavaScript.
class WebContentsMainFrameHelper : public content::WebContentsObserver {
 public:
  WebContentsMainFrameHelper(content::WebContents* web_contents,
                             const base::Closure& callback)
      : callback_(callback),
        title_set_count_(0),
        loaded_distiller_page_(false) {
    content::WebContentsObserver::Observe(web_contents);
  }

  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override {
    if (!render_frame_host->GetParent() &&
        validated_url.scheme() == kDomDistillerScheme)
      loaded_distiller_page_ = true;
  }

  void TitleWasSet(content::NavigationEntry* entry) override {
    // The title will be set twice on distilled pages; once for the placeholder
    // and once when the distillation has finished. Watch for the second time
    // as a signal that the JavaScript that sets the content has run.
    title_set_count_++;
    if (title_set_count_ >= 2 && loaded_distiller_page_) {
      callback_.Run();
    }
  }

 private:
  base::Closure callback_;
  int title_set_count_;
  bool loaded_distiller_page_;
};

// https://crbug.com/751730.
#if defined(OS_CHROMEOS) || defined(OS_LINUX)
#define MAYBE_TestSwapWebContents DISABLED_TestSwapWebContents
#else
#define MAYBE_TestSwapWebContents TestSwapWebContents
#endif
IN_PROC_BROWSER_TEST_F(DomDistillerTabUtilsBrowserTest,
                       MAYBE_TestSwapWebContents) {
  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* initial_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  const GURL& article_url = embedded_test_server()->GetURL(kSimpleArticlePath);

  // This blocks until the navigation has completely finished.
  ui_test_utils::NavigateToURL(browser(), article_url);

  DistillCurrentPageAndView(initial_web_contents);

  // Wait until the new WebContents has fully navigated.
  content::WebContents* after_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(after_web_contents != NULL);
  base::RunLoop new_url_loaded_runner;
  std::unique_ptr<WebContentsMainFrameHelper> distilled_page_loaded(
      new WebContentsMainFrameHelper(after_web_contents,
                                     new_url_loaded_runner.QuitClosure()));
  new_url_loaded_runner.Run();

  std::string page_title;
  content::ExecuteScriptAndGetValue(after_web_contents->GetMainFrame(),
                                    "document.title")->GetAsString(&page_title);

  // Verify the new URL is showing distilled content in a new WebContents.
  EXPECT_NE(initial_web_contents, after_web_contents);
  EXPECT_TRUE(
      after_web_contents->GetLastCommittedURL().SchemeIs(kDomDistillerScheme));
  EXPECT_EQ("Test Page Title", page_title);
}

IN_PROC_BROWSER_TEST_F(DomDistillerTabUtilsBrowserTest,
                       TestDistillIntoWebContents) {
  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* source_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  const GURL& article_url = embedded_test_server()->GetURL(kSimpleArticlePath);

  // This blocks until the navigation has completely finished.
  ui_test_utils::NavigateToURL(browser(), article_url);

  // Create destination WebContents.
  content::WebContents::CreateParams create_params(
      source_web_contents->GetBrowserContext());
  std::unique_ptr<content::WebContents> destination_web_contents =
      content::WebContents::Create(create_params);
  content::WebContents* raw_destination_web_contents =
      destination_web_contents.get();
  DCHECK(raw_destination_web_contents);

  browser()->tab_strip_model()->AppendWebContents(
      std::move(destination_web_contents), true);
  ASSERT_EQ(raw_destination_web_contents,
            browser()->tab_strip_model()->GetWebContentsAt(1));

  DistillAndView(source_web_contents, raw_destination_web_contents);

  // Wait until the destination WebContents has fully navigated.
  base::RunLoop new_url_loaded_runner;
  std::unique_ptr<WebContentsMainFrameHelper> distilled_page_loaded(
      new WebContentsMainFrameHelper(raw_destination_web_contents,
                                     new_url_loaded_runner.QuitClosure()));
  new_url_loaded_runner.Run();

  // Verify that the source WebContents is showing the original article.
  EXPECT_EQ(article_url, source_web_contents->GetLastCommittedURL());
  std::string page_title;
  content::ExecuteScriptAndGetValue(source_web_contents->GetMainFrame(),
                                    "document.title")->GetAsString(&page_title);
  EXPECT_EQ("Test Page Title", page_title);

  // Verify the destination WebContents is showing distilled content.
  EXPECT_TRUE(raw_destination_web_contents->GetLastCommittedURL().SchemeIs(
      kDomDistillerScheme));
  content::ExecuteScriptAndGetValue(
      raw_destination_web_contents->GetMainFrame(), "document.title")
      ->GetAsString(&page_title);
  EXPECT_EQ("Test Page Title", page_title);

  content::WebContentsDestroyedWatcher destroyed_watcher(
      raw_destination_web_contents);
  browser()->tab_strip_model()->CloseWebContentsAt(1, 0);
  destroyed_watcher.Wait();
}

}  // namespace dom_distiller
