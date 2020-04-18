// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/view_messages.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/frame_navigate_params.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/base/filename_util.h"
#include "net/base/host_port_pair.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace content {

class RenderViewHostTest : public ContentBrowserTest {
 public:
  RenderViewHostTest() {}
};

class RenderViewHostTestWebContentsObserver : public WebContentsObserver {
 public:
  explicit RenderViewHostTestWebContentsObserver(WebContents* web_contents)
      : WebContentsObserver(web_contents),
        navigation_count_(0) {}
  ~RenderViewHostTestWebContentsObserver() override {}

  void DidFinishNavigation(NavigationHandle* navigation_handle) override {
    if (!navigation_handle->IsInMainFrame() ||
        !navigation_handle->HasCommitted()) {
      return;
    }

    NavigationHandleImpl* impl =
        static_cast<NavigationHandleImpl*>(navigation_handle);
    observed_socket_address_ = navigation_handle->GetSocketAddress();
    base_url_ = impl->base_url();
    ++navigation_count_;
  }

  const net::HostPortPair& observed_socket_address() const {
    return observed_socket_address_;
  }

  GURL base_url() const {
    return base_url_;
  }

  int navigation_count() const { return navigation_count_; }

 private:
  net::HostPortPair observed_socket_address_;
  GURL base_url_;
  int navigation_count_;

  DISALLOW_COPY_AND_ASSIGN(RenderViewHostTestWebContentsObserver);
};

IN_PROC_BROWSER_TEST_F(RenderViewHostTest, FrameNavigateSocketAddress) {
  ASSERT_TRUE(embedded_test_server()->Start());
  RenderViewHostTestWebContentsObserver observer(shell()->web_contents());

  GURL test_url = embedded_test_server()->GetURL("/simple_page.html");
  NavigateToURL(shell(), test_url);

  EXPECT_EQ(net::HostPortPair::FromURL(
                embedded_test_server()->base_url()).ToString(),
            observer.observed_socket_address().ToString());
  EXPECT_EQ(1, observer.navigation_count());
}

IN_PROC_BROWSER_TEST_F(RenderViewHostTest, BaseURLParam) {
  ASSERT_TRUE(embedded_test_server()->Start());
  RenderViewHostTestWebContentsObserver observer(shell()->web_contents());

  // Base URL is not set if it is the same as the URL.
  GURL test_url = embedded_test_server()->GetURL("/simple_page.html");
  NavigateToURL(shell(), test_url);
  EXPECT_TRUE(observer.base_url().is_empty());
  EXPECT_EQ(1, observer.navigation_count());

  // But should be set to the original page when reading MHTML.
  NavigateToURL(shell(), GetTestUrl(nullptr, "google.mht"));
  EXPECT_EQ("http://www.google.com/", observer.base_url().spec());
}

// This test ensures a RenderFrameHost object is created for the top level frame
// in each RenderViewHost.
IN_PROC_BROWSER_TEST_F(RenderViewHostTest, BasicRenderFrameHost) {
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL test_url = embedded_test_server()->GetURL("/simple_page.html");
  NavigateToURL(shell(), test_url);

  FrameTreeNode* old_root = static_cast<WebContentsImpl*>(
      shell()->web_contents())->GetFrameTree()->root();
  EXPECT_TRUE(old_root->current_frame_host());

  ShellAddedObserver new_shell_observer;
  EXPECT_TRUE(ExecuteScript(shell(), "window.open();"));
  Shell* new_shell = new_shell_observer.GetShell();
  FrameTreeNode* new_root = static_cast<WebContentsImpl*>(
      new_shell->web_contents())->GetFrameTree()->root();

  EXPECT_TRUE(new_root->current_frame_host());
  EXPECT_NE(old_root->current_frame_host()->routing_id(),
            new_root->current_frame_host()->routing_id());
}

IN_PROC_BROWSER_TEST_F(RenderViewHostTest, IsFocusedElementEditable) {
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL test_url = embedded_test_server()->GetURL("/touch_selection.html");
  NavigateToURL(shell(), test_url);

  WebContents* contents = shell()->web_contents();
  EXPECT_FALSE(contents->IsFocusedElementEditable());
  EXPECT_TRUE(ExecuteScript(shell(), "focus_textfield();"));
  EXPECT_TRUE(contents->IsFocusedElementEditable());
}

// Flaky on Linux (https://crbug.com/559192).
#if defined(OS_LINUX)
#define MAYBE_ReleaseSessionOnCloseACK DISABLED_ReleaseSessionOnCloseACK
#else
#define MAYBE_ReleaseSessionOnCloseACK ReleaseSessionOnCloseACK
#endif
IN_PROC_BROWSER_TEST_F(RenderViewHostTest, MAYBE_ReleaseSessionOnCloseACK) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL test_url = embedded_test_server()->GetURL(
      "/access-session-storage.html");
  NavigateToURL(shell(), test_url);

  // Make a new Shell, a seperate tab with it's own session namespace and
  // have it start loading a url but still be in progress.
  ShellAddedObserver new_shell_observer;
  EXPECT_TRUE(ExecuteScript(shell(), "window.open();"));
  Shell* new_shell = new_shell_observer.GetShell();
  new_shell->LoadURL(test_url);
  RenderViewHost* rvh = new_shell->web_contents()->GetRenderViewHost();
  SiteInstance* site_instance = rvh->GetSiteInstance();
  scoped_refptr<SessionStorageNamespace> session_namespace =
      rvh->GetDelegate()->GetSessionStorageNamespace(site_instance);
  EXPECT_FALSE(session_namespace->HasOneRef());

  // Close it, or rather start the close operation. The session namespace
  // should remain until RPH gets an ACK from the renderer about having
  // closed the view.
  new_shell->Close();
  EXPECT_FALSE(session_namespace->HasOneRef());

  // Do something that causes ipc queues to flush and tasks in
  // flight to complete such that we should have received the ACK.
  NavigateToURL(shell(), test_url);

  // Verify we have the only remaining reference to the namespace.
  EXPECT_TRUE(session_namespace->HasOneRef());
}

}  // namespace content
