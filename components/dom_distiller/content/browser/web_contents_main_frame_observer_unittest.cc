// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/content/browser/web_contents_main_frame_observer.h"

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/test/test_renderer_host.h"

namespace dom_distiller {

class WebContentsMainFrameObserverTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    // This needed to keep the WebContentsObserverSanityChecker checks happy for
    // when AppendChild is called.
    NavigateAndCommit(GURL("about:blank"));
    dom_distiller::WebContentsMainFrameObserver::CreateForWebContents(
        web_contents());
    main_frame_observer_ =
        WebContentsMainFrameObserver::FromWebContents(web_contents());
    ASSERT_FALSE(main_frame_observer_->is_document_loaded_in_main_frame());
  }

  void Navigate(bool main_frame, bool same_document) {
    content::RenderFrameHost* rfh = main_rfh();
    content::RenderFrameHostTester* rfh_tester =
        content::RenderFrameHostTester::For(rfh);
    if (!main_frame)
      rfh = rfh_tester->AppendChild("subframe");
    std::unique_ptr<content::NavigationHandle> navigation_handle =
        content::NavigationHandle::CreateNavigationHandleForTesting(
            GURL(), rfh, true, net::OK, same_document);
    // Destructor calls DidFinishNavigation.
  }

 protected:
  WebContentsMainFrameObserver* main_frame_observer_;  // weak
};

TEST_F(WebContentsMainFrameObserverTest, ListensForMainFrameNavigation) {
  Navigate(true, false);
  ASSERT_TRUE(main_frame_observer_->is_initialized());
  ASSERT_FALSE(main_frame_observer_->is_document_loaded_in_main_frame());

  main_frame_observer_->DocumentLoadedInFrame(main_rfh());
  ASSERT_TRUE(main_frame_observer_->is_document_loaded_in_main_frame());
}

TEST_F(WebContentsMainFrameObserverTest, IgnoresChildFrameNavigation) {
  Navigate(false, false);
  ASSERT_FALSE(main_frame_observer_->is_initialized());
  ASSERT_FALSE(main_frame_observer_->is_document_loaded_in_main_frame());
}

TEST_F(WebContentsMainFrameObserverTest, IgnoresSameDocumentNavigation) {
  Navigate(true, true);
  ASSERT_FALSE(main_frame_observer_->is_initialized());
  ASSERT_FALSE(main_frame_observer_->is_document_loaded_in_main_frame());
}

TEST_F(WebContentsMainFrameObserverTest,
       IgnoresSameDocumentavigationUnlessMainFrameLoads) {
  Navigate(true, true);
  ASSERT_FALSE(main_frame_observer_->is_initialized());
  ASSERT_FALSE(main_frame_observer_->is_document_loaded_in_main_frame());

  // Even if we didn't acknowledge a same-document navigation, if the main
  // frame loads, consider a load complete.
  main_frame_observer_->DocumentLoadedInFrame(main_rfh());
  ASSERT_TRUE(main_frame_observer_->is_document_loaded_in_main_frame());
}

TEST_F(WebContentsMainFrameObserverTest, ResetOnPageNavigation) {
  Navigate(true, false);
  main_frame_observer_->DocumentLoadedInFrame(main_rfh());
  ASSERT_TRUE(main_frame_observer_->is_document_loaded_in_main_frame());

  // Another navigation should result in waiting for a page load.
  Navigate(true, false);
  ASSERT_TRUE(main_frame_observer_->is_initialized());
  ASSERT_FALSE(main_frame_observer_->is_document_loaded_in_main_frame());
}

TEST_F(WebContentsMainFrameObserverTest, DoesNotResetOnInPageNavigation) {
  Navigate(true, false);
  main_frame_observer_->DocumentLoadedInFrame(main_rfh());
  ASSERT_TRUE(main_frame_observer_->is_document_loaded_in_main_frame());

  // Navigating withing the page should not result in waiting for a page load.
  Navigate(true, true);
  ASSERT_TRUE(main_frame_observer_->is_initialized());
  ASSERT_TRUE(main_frame_observer_->is_document_loaded_in_main_frame());
}

}  // namespace dom_distiller
