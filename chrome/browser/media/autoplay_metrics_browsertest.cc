// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_frame_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "services/metrics/public/cpp/ukm_builders.h"

namespace chrome {

namespace {

class AutoplayMetricsBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TryAutoplay(const content::ToRenderFrameHost& adapter) {
    EXPECT_TRUE(ExecuteScriptWithoutUserGesture(adapter.render_frame_host(),
                                                "tryPlayback();"));
  }

  void NavigateFrameAndWait(content::RenderFrameHost* rfh, const GURL& url) {
    content::TestFrameNavigationObserver observer(rfh);
    content::NavigationController::LoadURLParams params(url);
    params.transition_type = ui::PAGE_TRANSITION_LINK;
    params.frame_tree_node_id = rfh->GetFrameTreeNodeId();
    content::WebContents::FromRenderFrameHost(rfh)
        ->GetController()
        .LoadURLWithParams(params);
    observer.Wait();
  }

  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* first_child() const {
    return web_contents()->GetAllFrames()[1];
  }

  content::RenderFrameHost* second_child() const {
    return web_contents()->GetAllFrames()[2];
  }

  static const ukm::mojom::UkmEntry* FindDocumentCreatedEntry(
      ukm::TestUkmRecorder& ukm_recorder,
      ukm::SourceId source_id) {
    using Entry = ukm::builders::DocumentCreated;
    auto entries = ukm_recorder.GetEntriesByName(Entry::kEntryName);

    for (auto* entry : entries) {
      if (entry->source_id == source_id)
        return entry;
    }

    NOTREACHED();
    return nullptr;
  }
};

IN_PROC_BROWSER_TEST_F(AutoplayMetricsBrowserTest, RecordAutoplayAttemptUkm) {
  ukm::TestAutoSetUkmRecorder test_ukm_recorder;
  using Entry = ukm::builders::Media_Autoplay_Attempt;
  using CreatedEntry = ukm::builders::DocumentCreated;

  GURL main_url(embedded_test_server()->GetURL("example.com",
                                               "/media/autoplay_iframe.html"));
  GURL foo_url(
      embedded_test_server()->GetURL("foo.com", "/media/autoplay_iframe.html"));
  GURL bar_url(
      embedded_test_server()->GetURL("bar.com", "/media/autoplay_iframe.html"));

  // Navigate main frame, try play.
  NavigateFrameAndWait(web_contents()->GetMainFrame(), main_url);
  TryAutoplay(web_contents());

  // Check that we recorded a UKM event using the main frame URL.
  {
    auto ukm_entries = test_ukm_recorder.GetEntriesByName(Entry::kEntryName);

    EXPECT_EQ(1u, ukm_entries.size());
    test_ukm_recorder.ExpectEntrySourceHasUrl(ukm_entries[0], main_url);
  }

  // Navigate sub frame, try play.
  NavigateFrameAndWait(first_child(), foo_url);
  TryAutoplay(first_child());

  // Check that we recorded a UKM event that is not keyed to any URL.
  {
    auto ukm_entries = test_ukm_recorder.GetEntriesByName(Entry::kEntryName);

    EXPECT_EQ(2u, ukm_entries.size());
    EXPECT_FALSE(
        test_ukm_recorder.GetSourceForSourceId(ukm_entries[1]->source_id));

    // Check that a DocumentCreated entry was also created that was not keyed to
    // any URL. However, we can use the navigation source ID to link this source
    // to the top frame URL.
    auto* dc_entry =
        FindDocumentCreatedEntry(test_ukm_recorder, ukm_entries[1]->source_id);
    EXPECT_EQ(ukm_entries[1]->source_id, dc_entry->source_id);
    EXPECT_FALSE(test_ukm_recorder.GetSourceForSourceId(dc_entry->source_id));
    EXPECT_EQ(main_url,
              test_ukm_recorder
                  .GetSourceForSourceId(*test_ukm_recorder.GetEntryMetric(
                      dc_entry, CreatedEntry::kNavigationSourceIdName))
                  ->url());
    EXPECT_EQ(0, *test_ukm_recorder.GetEntryMetric(
                     dc_entry, CreatedEntry::kIsMainFrameName));
  }

  // Navigate sub sub frame, try play.
  NavigateFrameAndWait(second_child(), bar_url);
  TryAutoplay(second_child());

  // Check that we recorded a UKM event that is not keyed to any url.
  {
    auto ukm_entries = test_ukm_recorder.GetEntriesByName(Entry::kEntryName);

    EXPECT_EQ(3u, ukm_entries.size());
    EXPECT_FALSE(
        test_ukm_recorder.GetSourceForSourceId(ukm_entries[2]->source_id));

    // Check that a DocumentCreated entry was also created that was not keyed to
    // any URL. However, we can use the navigation source ID to link this source
    // to the top frame URL.
    auto* dc_entry =
        FindDocumentCreatedEntry(test_ukm_recorder, ukm_entries[2]->source_id);
    EXPECT_EQ(ukm_entries[2]->source_id, dc_entry->source_id);
    EXPECT_FALSE(test_ukm_recorder.GetSourceForSourceId(dc_entry->source_id));
    EXPECT_EQ(main_url,
              test_ukm_recorder
                  .GetSourceForSourceId(*test_ukm_recorder.GetEntryMetric(
                      dc_entry, CreatedEntry::kNavigationSourceIdName))
                  ->url());
    EXPECT_EQ(0, *test_ukm_recorder.GetEntryMetric(
                     dc_entry, CreatedEntry::kIsMainFrameName));
  }

  // Navigate top frame, try play.
  NavigateFrameAndWait(web_contents()->GetMainFrame(), foo_url);
  TryAutoplay(web_contents());

  // Check that we recorded a UKM event using the main frame URL.
  {
    auto ukm_entries = test_ukm_recorder.GetEntriesByName(Entry::kEntryName);

    EXPECT_EQ(4u, ukm_entries.size());
    test_ukm_recorder.ExpectEntrySourceHasUrl(ukm_entries[3], foo_url);
  }
}

}  // namespace

}  // namespace chrome
