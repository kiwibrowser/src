// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_test_harness.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/ukm/content/source_url_recorder.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/web_contents_tester.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace page_load_metrics {

PageLoadMetricsObserverTestHarness::PageLoadMetricsObserverTestHarness()
    : ChromeRenderViewHostTestHarness() {}

PageLoadMetricsObserverTestHarness::~PageLoadMetricsObserverTestHarness() {}

void PageLoadMetricsObserverTestHarness::SetUp() {
  ChromeRenderViewHostTestHarness::SetUp();
  SetContents(CreateTestWebContents());
  NavigateAndCommit(GURL("http://www.google.com"));
  // Page load metrics depends on UKM source URLs being recorded, so make sure
  // the SourceUrlRecorderWebContentsObserver is instantiated.
  ukm::InitializeSourceUrlRecorderForWebContents(web_contents());
  tester_ = std::make_unique<PageLoadMetricsObserverTester>(
      web_contents(),
      base::BindRepeating(
          &PageLoadMetricsObserverTestHarness::RegisterObservers,
          base::Unretained(this)));
  web_contents()->WasShown();
}

void PageLoadMetricsObserverTestHarness::StartNavigation(const GURL& gurl) {
  std::unique_ptr<content::NavigationSimulator> navigation =
      content::NavigationSimulator::CreateBrowserInitiated(gurl,
                                                           web_contents());
  navigation->Start();
}

void PageLoadMetricsObserverTestHarness::SimulateTimingUpdate(
    const mojom::PageLoadTiming& timing) {
  tester_->SimulateTimingAndMetadataUpdate(timing, mojom::PageLoadMetadata());
}

void PageLoadMetricsObserverTestHarness::SimulateTimingAndMetadataUpdate(
    const mojom::PageLoadTiming& timing,
    const mojom::PageLoadMetadata& metadata) {
  tester_->SimulateTimingAndMetadataUpdate(timing, metadata);
}

void PageLoadMetricsObserverTestHarness::SimulateFeaturesUpdate(
    const mojom::PageLoadFeatures& new_features) {
  tester_->SimulateFeaturesUpdate(new_features);
}

void PageLoadMetricsObserverTestHarness::SimulateLoadedResource(
    const ExtraRequestCompleteInfo& info) {
  tester_->SimulateLoadedResource(info, content::GlobalRequestID());
}

void PageLoadMetricsObserverTestHarness::SimulateLoadedResource(
    const ExtraRequestCompleteInfo& info,
    const content::GlobalRequestID& request_id) {
  tester_->SimulateLoadedResource(info, request_id);
}

void PageLoadMetricsObserverTestHarness::SimulateInputEvent(
    const blink::WebInputEvent& event) {
  tester_->SimulateInputEvent(event);
}

void PageLoadMetricsObserverTestHarness::SimulateAppEnterBackground() {
  tester_->SimulateAppEnterBackground();
}

void PageLoadMetricsObserverTestHarness::SimulateMediaPlayed() {
  tester_->SimulateMediaPlayed();
}

const base::HistogramTester&
PageLoadMetricsObserverTestHarness::histogram_tester() const {
  return histogram_tester_;
}

MetricsWebContentsObserver* PageLoadMetricsObserverTestHarness::observer()
    const {
  return tester_->observer();
}

const PageLoadExtraInfo
PageLoadMetricsObserverTestHarness::GetPageLoadExtraInfoForCommittedLoad() {
  return tester_->GetPageLoadExtraInfoForCommittedLoad();
}

void PageLoadMetricsObserverTestHarness::NavigateWithPageTransitionAndCommit(
    const GURL& url,
    ui::PageTransition transition) {
  auto simulator =
      content::NavigationSimulator::CreateRendererInitiated(url, main_rfh());
  simulator->SetTransition(transition);
  simulator->Commit();
}

void PageLoadMetricsObserverTestHarness::NavigateToUntrackedUrl() {
  NavigateAndCommit(GURL(url::kAboutBlankURL));
}

const char PageLoadMetricsObserverTestHarness::kResourceUrl[] =
    "https://www.example.com/resource";

}  // namespace page_load_metrics
