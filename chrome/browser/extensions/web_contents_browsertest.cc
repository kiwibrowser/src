// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/devtools/devtools_window_testing.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/browser/extension_navigation_ui_data.h"

namespace extensions {
namespace {

content::WebContents* GetActiveWebContents(const Browser* browser) {
  return browser->tab_strip_model()->GetActiveWebContents();
}

// Saves ExtensionNavigationUIData for each render frame which completes
// navigation.
class ExtensionNavigationUIDataObserver : public content::WebContentsObserver {
 public:
  explicit ExtensionNavigationUIDataObserver(content::WebContents* web_contents)
      : WebContentsObserver(web_contents) {}

  const ExtensionNavigationUIData* GetExtensionNavigationUIData(
      content::RenderFrameHost* rfh) const {
    auto iter = navigation_ui_data_map_.find(rfh);
    if (iter == navigation_ui_data_map_.end())
      return nullptr;
    return iter->second.get();
  }

 private:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (!navigation_handle->HasCommitted())
      return;

    content::RenderFrameHost* rfh = navigation_handle->GetRenderFrameHost();
    const auto* data = static_cast<const ChromeNavigationUIData*>(
        navigation_handle->GetNavigationUIData());
    navigation_ui_data_map_[rfh] =
        data->GetExtensionNavigationUIData()->DeepCopy();
  }

  std::map<content::RenderFrameHost*,
           std::unique_ptr<ExtensionNavigationUIData>>
      navigation_ui_data_map_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionNavigationUIDataObserver);
};

}  // namespace

// Tests that we can load extension pages into the tab area and they can call
// extension APIs.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, WebContents) {
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("good").AppendASCII("Extensions")
                    .AppendASCII("behllobkkfkfnphdnhnkndlbkcpglgmj")
                    .AppendASCII("1.0.0.0")));

  ui_test_utils::NavigateToURL(
      browser(),
      GURL("chrome-extension://behllobkkfkfnphdnhnkndlbkcpglgmj/page.html"));

  bool result = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetActiveWebContents(browser()), "testTabsAPI()", &result));
  EXPECT_TRUE(result);

  // There was a bug where we would crash if we navigated to a page in the same
  // extension because no new render view was getting created, so we would not
  // do some setup.
  ui_test_utils::NavigateToURL(
      browser(),
      GURL("chrome-extension://behllobkkfkfnphdnhnkndlbkcpglgmj/page.html"));
  result = false;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      GetActiveWebContents(browser()), "testTabsAPI()", &result));
  EXPECT_TRUE(result);
}

// Test that the frame data for the Devtools main frame is cached. Regression
// test for crbug.com/817075.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, DevToolsMainFrameIsCached) {
  auto test_devtools_main_frame_cached = [](Browser* browser, bool is_docked) {
    SCOPED_TRACE(base::StringPrintf("Testing a %s devtools window",
                                    is_docked ? "docked" : "undocked"));
    const auto* api_frame_id_map = ExtensionApiFrameIdMap::Get();
    size_t prior_count = api_frame_id_map->GetFrameDataCountForTesting();

    // Open a devtools window.
    DevToolsWindow* devtools_window =
        DevToolsWindowTesting::OpenDevToolsWindowSync(browser, is_docked);

    // Ensure that frame data for its main frame is cached.
    content::WebContents* devtools_web_contents =
        DevToolsWindow::GetInTabWebContents(
            devtools_window->GetInspectedWebContents(), nullptr);
    ASSERT_TRUE(devtools_web_contents);
    EXPECT_TRUE(api_frame_id_map->HasCachedFrameDataForTesting(
        devtools_web_contents->GetMainFrame()));
    EXPECT_GT(api_frame_id_map->GetFrameDataCountForTesting(), prior_count);

    DevToolsWindowTesting::CloseDevToolsWindowSync(devtools_window);

    // Ensure that the frame data for the devtools main frame, which might have
    // been destroyed by now, is deleted.
    EXPECT_EQ(prior_count, api_frame_id_map->GetFrameDataCountForTesting());
  };

  test_devtools_main_frame_cached(browser(), true /*is_docked*/);
  test_devtools_main_frame_cached(browser(), false /*is_docked*/);
}

// Test that we correctly cache frame data for all frames on creation.
// Regression test for crbug.com/810614.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FrameDataCached) {
  // Load an extension with a web accessible resource.
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("web_accessible_resources"));
  ASSERT_TRUE(extension);

  ASSERT_TRUE(embedded_test_server()->Start());

  // Some utility functions for the test.

  // Returns whether the frame data for |rfh| is cached.
  auto has_cached_frame_data = [](content::RenderFrameHost* rfh) {
    return ExtensionApiFrameIdMap::Get()->HasCachedFrameDataForTesting(rfh);
  };

  // Returns the cached frame data for |rfh|.
  auto get_frame_data = [](content::RenderFrameHost* rfh) {
    return ExtensionApiFrameIdMap::Get()->GetFrameData(rfh);
  };

  // Adds an iframe with the given |name| and |src| to the given |web_contents|
  // and waits till it loads. Returns true if successful.
  auto add_iframe = [](content::WebContents* web_contents,
                       const std::string& name, const GURL& src) {
    content::TestNavigationObserver observer(web_contents,
                                             1 /*number_of_navigations*/);
    const char* code = R"(
      var iframe = document.createElement('iframe');
      iframe.name = '%s';
      iframe.src = '%s';
      document.body.appendChild(iframe);
    )";
    content::ExecuteScriptAsync(
        web_contents,
        base::StringPrintf(code, name.c_str(), src.spec().c_str()));

    observer.WaitForNavigationFinished();
    return observer.last_navigation_succeeded() &&
           observer.last_navigation_url() == src;
  };

  // Returns the frame with the given |name| in |web_contents|.
  auto get_frame_by_name = [](content::WebContents* web_contents,
                              const std::string& name) {
    return content::FrameMatchingPredicate(
        web_contents, base::Bind(&content::FrameMatchesName, name));
  };

  // Navigates the browser to |url|. Injects a web-frame and an extension frame
  // into the page and ensures that extension frame data is correctly cached for
  // each created frame.
  auto load_page_and_test = [&](const GURL& url) {
    using FrameData = ExtensionApiFrameIdMap::FrameData;

    SCOPED_TRACE(base::StringPrintf("Testing %s", url.spec().c_str()));

    ui_test_utils::NavigateToURL(browser(), url);
    content::WebContents* web_contents = GetActiveWebContents(browser());
    SessionTabHelper* session_tab_helper =
        SessionTabHelper::FromWebContents(web_contents);
    ASSERT_TRUE(session_tab_helper);
    int expected_tab_id = session_tab_helper->session_id().id();
    int expected_window_id = session_tab_helper->window_id().id();

    // Ensure that the frame data for the main frame is cached.
    content::RenderFrameHost* rfh = web_contents->GetMainFrame();
    EXPECT_TRUE(has_cached_frame_data(rfh));
    FrameData main_frame_data = get_frame_data(rfh);
    EXPECT_EQ(ExtensionApiFrameIdMap::kTopFrameId, main_frame_data.frame_id);
    EXPECT_EQ(ExtensionApiFrameIdMap::kInvalidFrameId,
              main_frame_data.parent_frame_id);
    EXPECT_EQ(expected_tab_id, main_frame_data.tab_id);
    EXPECT_EQ(expected_window_id, main_frame_data.window_id);
    EXPECT_EQ(url, main_frame_data.last_committed_main_frame_url);
    EXPECT_FALSE(main_frame_data.pending_main_frame_url);

    // Add an extension iframe to the page and ensure its frame data is cached.
    ASSERT_TRUE(
        add_iframe(web_contents, "extension_frame",
                   extension->GetResourceURL("web_accessible_page.html")));
    rfh = get_frame_by_name(web_contents, "extension_frame");
    EXPECT_TRUE(has_cached_frame_data(rfh));
    FrameData extension_frame_data = get_frame_data(rfh);
    EXPECT_NE(ExtensionApiFrameIdMap::kInvalidFrameId,
              extension_frame_data.frame_id);
    EXPECT_NE(ExtensionApiFrameIdMap::kTopFrameId,
              extension_frame_data.frame_id);
    EXPECT_EQ(ExtensionApiFrameIdMap::kTopFrameId,
              extension_frame_data.parent_frame_id);
    EXPECT_EQ(expected_tab_id, extension_frame_data.tab_id);
    EXPECT_EQ(expected_window_id, extension_frame_data.window_id);
    EXPECT_EQ(url, extension_frame_data.last_committed_main_frame_url);
    EXPECT_FALSE(extension_frame_data.pending_main_frame_url);

    // Add a web frame to the page and ensure its frame data is cached.
    ASSERT_TRUE(add_iframe(web_contents, "web_frame",
                           embedded_test_server()->GetURL("/empty.html")));
    rfh = get_frame_by_name(web_contents, "web_frame");
    EXPECT_TRUE(has_cached_frame_data(rfh));
    FrameData web_frame_data = get_frame_data(rfh);
    EXPECT_NE(ExtensionApiFrameIdMap::kInvalidFrameId, web_frame_data.frame_id);
    EXPECT_NE(ExtensionApiFrameIdMap::kTopFrameId, web_frame_data.frame_id);
    EXPECT_NE(extension_frame_data.frame_id, web_frame_data.frame_id);
    EXPECT_EQ(ExtensionApiFrameIdMap::kTopFrameId,
              web_frame_data.parent_frame_id);
    EXPECT_EQ(expected_tab_id, web_frame_data.tab_id);
    EXPECT_EQ(expected_window_id, web_frame_data.window_id);
    EXPECT_EQ(url, web_frame_data.last_committed_main_frame_url);
    EXPECT_FALSE(web_frame_data.pending_main_frame_url);
  };
  // End utility functions.

  // Test an extension page.
  load_page_and_test(extension->GetResourceURL("extension_page.html"));

  // Test a non-extension page.
  load_page_and_test(embedded_test_server()->GetURL("/empty.html"));
}

// Test that we correctly set up the ExtensionNavigationUIData for each
// navigation.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, ExtensionNavigationUIData) {
  ASSERT_TRUE(embedded_test_server()->Start());
  content::WebContents* web_contents = GetActiveWebContents(browser());
  GURL last_committed_main_frame_url = web_contents->GetLastCommittedURL();
  ExtensionNavigationUIDataObserver observer(web_contents);

  // Load a page with an iframe.
  const GURL url = embedded_test_server()->GetURL("/iframe.html");
  ui_test_utils::NavigateToURL(browser(), url);

  SessionTabHelper* session_tab_helper =
      SessionTabHelper::FromWebContents(web_contents);
  ASSERT_TRUE(session_tab_helper);
  int expected_tab_id = session_tab_helper->session_id().id();
  int expected_window_id = session_tab_helper->window_id().id();

  // Test ExtensionNavigationUIData for the main frame.
  {
    const auto* extension_navigation_ui_data =
        observer.GetExtensionNavigationUIData(web_contents->GetMainFrame());
    ASSERT_TRUE(extension_navigation_ui_data);
    EXPECT_FALSE(extension_navigation_ui_data->is_web_view());

    ExtensionApiFrameIdMap::FrameData frame_data =
        extension_navigation_ui_data->frame_data();
    EXPECT_EQ(ExtensionApiFrameIdMap::kTopFrameId, frame_data.frame_id);
    EXPECT_EQ(ExtensionApiFrameIdMap::kInvalidFrameId,
              frame_data.parent_frame_id);
    EXPECT_EQ(expected_tab_id, frame_data.tab_id);
    EXPECT_EQ(expected_window_id, frame_data.window_id);
    EXPECT_EQ(last_committed_main_frame_url,
              frame_data.last_committed_main_frame_url);
    EXPECT_FALSE(frame_data.pending_main_frame_url);
  }

  // Test ExtensionNavigationUIData for the sub-frame.
  {
    const auto* extension_navigation_ui_data =
        observer.GetExtensionNavigationUIData(
            content::ChildFrameAt(web_contents->GetMainFrame(), 0));
    ASSERT_TRUE(extension_navigation_ui_data);
    EXPECT_FALSE(extension_navigation_ui_data->is_web_view());

    ExtensionApiFrameIdMap::FrameData frame_data =
        extension_navigation_ui_data->frame_data();
    EXPECT_NE(ExtensionApiFrameIdMap::kInvalidFrameId, frame_data.frame_id);
    EXPECT_NE(ExtensionApiFrameIdMap::kTopFrameId, frame_data.frame_id);
    EXPECT_EQ(ExtensionApiFrameIdMap::kTopFrameId, frame_data.parent_frame_id);
    EXPECT_EQ(expected_tab_id, frame_data.tab_id);
    EXPECT_EQ(expected_window_id, frame_data.window_id);
    EXPECT_EQ(url, frame_data.last_committed_main_frame_url);
    EXPECT_FALSE(frame_data.pending_main_frame_url);
  }
}

}  // namespace extensions
