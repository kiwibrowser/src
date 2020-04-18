// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_LOAD_METRICS_OBSERVER_TEST_HARNESS_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_LOAD_METRICS_OBSERVER_TEST_HARNESS_H_

#include <memory>

#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/page_load_metrics/observers/page_load_metrics_observer_tester.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/ukm/test_ukm_recorder.h"
#include "ui/base/page_transition_types.h"

namespace base {
class GURL;
}  // namespace base

namespace blink {
class WebInputEvent;
}  // namespace blink

namespace content {
struct GlobalRequestID;
}  // namespace content

namespace mojom {
class PageLoadMetadata;
class PageLoadTiming;
}  // namespace mojom

namespace page_load_metrics {

class MetricsWebContentsObserver;
class PageLoadTracker;

// This class can be used to drive tests of PageLoadMetricsObservers. To hook up
// an observer, override RegisterObservers and call tracker->AddObserver. This
// will attach the observer to all main frame navigations.
//
// This class is mostly just a wrapper around a PageLoadMetricsObserverTester.
// Prefer to use the tester directly if you need to compose the testing logic
// with another test harness.
class PageLoadMetricsObserverTestHarness
    : public ChromeRenderViewHostTestHarness {
 public:
  // Sample URL for resource loads.
  static const char kResourceUrl[];

  PageLoadMetricsObserverTestHarness();
  ~PageLoadMetricsObserverTestHarness() override;

  void SetUp() override;

  virtual void RegisterObservers(PageLoadTracker* tracker) {}

  // Simulates starting a navigation to the given gurl, without committing the
  // navigation.
  // Note: The navigation is left in a pending state and cannot be successfully
  // completed.
  void StartNavigation(const GURL& gurl);

  // Simulates committing a navigation to the given URL with the given
  // PageTransition.
  void NavigateWithPageTransitionAndCommit(const GURL& url,
                                           ui::PageTransition transition);

  // Navigates to a URL that is not tracked by page_load_metrics. Useful for
  // forcing the OnComplete method of a PageLoadMetricsObserver to run.
  void NavigateToUntrackedUrl();

  // Call this to simulate sending a PageLoadTiming IPC from the render process
  // to the browser process. These will update the timing information for the
  // most recently committed navigation.
  void SimulateTimingUpdate(const mojom::PageLoadTiming& timing);
  void SimulateTimingAndMetadataUpdate(const mojom::PageLoadTiming& timing,
                                       const mojom::PageLoadMetadata& metadata);
  void SimulateFeaturesUpdate(const mojom::PageLoadFeatures& new_features);
  void SimulatePageLoadTimingUpdate(
      const mojom::PageLoadTiming& timing,
      const mojom::PageLoadMetadata& metadata,
      const mojom::PageLoadFeatures& new_features);

  // Simulates a loaded resource. Main frame resources must specify a
  // GlobalRequestID, using the SimulateLoadedResource() method that takes a
  // |request_id| parameter.
  void SimulateLoadedResource(const ExtraRequestCompleteInfo& info);

  // Simulates a loaded resource, with the given GlobalRequestID.
  void SimulateLoadedResource(const ExtraRequestCompleteInfo& info,
                              const content::GlobalRequestID& request_id);

  // Simulates a user input.
  void SimulateInputEvent(const blink::WebInputEvent& event);

  // Simulates the app being backgrounded.
  void SimulateAppEnterBackground();

  // Simulate playing a media element.
  void SimulateMediaPlayed();

  const base::HistogramTester& histogram_tester() const;

  MetricsWebContentsObserver* observer() const;

  // Gets the PageLoadExtraInfo for the committed_load_ in observer_.
  const PageLoadExtraInfo GetPageLoadExtraInfoForCommittedLoad();

  const ukm::TestAutoSetUkmRecorder& test_ukm_recorder() const {
    return test_ukm_recorder_;
  }

 private:
  base::HistogramTester histogram_tester_;
  ukm::TestAutoSetUkmRecorder test_ukm_recorder_;
  std::unique_ptr<PageLoadMetricsObserverTester> tester_;

  DISALLOW_COPY_AND_ASSIGN(PageLoadMetricsObserverTestHarness);
};

}  // namespace page_load_metrics

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_LOAD_METRICS_OBSERVER_TEST_HARNESS_H_
