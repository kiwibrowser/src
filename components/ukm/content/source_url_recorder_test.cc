// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/content/source_url_recorder.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationSimulator;

class SourceUrlRecorderWebContentsObserverTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    ukm::InitializeSourceUrlRecorderForWebContents(web_contents());
  }

  GURL GetAssociatedURLForWebContentsDocument() {
    const ukm::UkmSource* src = test_ukm_recorder_.GetSourceForSourceId(
        ukm::GetSourceIdForWebContentsDocument(web_contents()));
    return src ? src->url() : GURL();
  }

 protected:
  ukm::TestAutoSetUkmRecorder test_ukm_recorder_;
};

TEST_F(SourceUrlRecorderWebContentsObserverTest, Basic) {
  GURL url("https://www.example.com/");
  NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(), url);

  const auto& sources = test_ukm_recorder_.GetSources();
  EXPECT_EQ(1ul, sources.size());
  for (const auto& kv : sources) {
    EXPECT_EQ(url, kv.second->url());
    EXPECT_TRUE(kv.second->initial_url().is_empty());
  }
}

TEST_F(SourceUrlRecorderWebContentsObserverTest, InitialUrl) {
  GURL initial_url("https://www.a.com/");
  GURL final_url("https://www.b.com/");
  auto simulator =
      NavigationSimulator::CreateRendererInitiated(initial_url, main_rfh());
  simulator->Start();
  simulator->Redirect(final_url);
  simulator->Commit();
  const auto& sources = test_ukm_recorder_.GetSources();
  EXPECT_EQ(1ul, sources.size());
  for (const auto& kv : sources) {
    EXPECT_EQ(final_url, kv.second->url());
    EXPECT_EQ(initial_url, kv.second->initial_url());
  }

  EXPECT_EQ(final_url, GetAssociatedURLForWebContentsDocument());
}

TEST_F(SourceUrlRecorderWebContentsObserverTest, IgnoreUrlInSubframe) {
  GURL main_frame_url("https://www.example.com/");
  GURL sub_frame_url("https://www.example.com/iframe.html");
  NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                    main_frame_url);
  NavigationSimulator::NavigateAndCommitFromDocument(
      sub_frame_url,
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("subframe"));

  const auto& sources = test_ukm_recorder_.GetSources();
  EXPECT_EQ(1ul, sources.size());
  for (const auto& kv : sources) {
    EXPECT_EQ(main_frame_url, kv.second->url());
    EXPECT_TRUE(kv.second->initial_url().is_empty());
  }

  EXPECT_EQ(main_frame_url, GetAssociatedURLForWebContentsDocument());
}

TEST_F(SourceUrlRecorderWebContentsObserverTest, IgnoreSameDocumentNavigation) {
  GURL url("https://www.example.com/");
  GURL same_document_url("https://www.example.com/#samedocument");
  NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(), url);
  NavigationSimulator::CreateRendererInitiated(same_document_url, main_rfh())
      ->CommitSameDocument();

  EXPECT_EQ(same_document_url, web_contents()->GetLastCommittedURL());

  const auto& sources = test_ukm_recorder_.GetSources();
  EXPECT_EQ(1ul, sources.size());
  for (const auto& kv : sources) {
    EXPECT_EQ(url, kv.second->url());
    EXPECT_TRUE(kv.second->initial_url().is_empty());
  }

  EXPECT_EQ(url, GetAssociatedURLForWebContentsDocument());
}
