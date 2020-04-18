// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_LOAD_METRICS_OBSERVER_TESTER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_LOAD_METRICS_OBSERVER_TESTER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/common/page_load_metrics/test/weak_mock_timer.h"

namespace blink {
class WebInputEvent;
}  // namespace blink

namespace content {
struct GlobalRequestID;
class WebContents;
}  // namespace content

namespace mojom {
class PageLoadMetadata;
class PageLoadTiming;
}  // namespace mojom

namespace page_load_metrics {

class MetricsWebContentsObserver;
class PageLoadTracker;

// This class creates a MetricsWebContentsObserver and provides methods for
// interacting with it. This class is designed to be used in unit tests for
// PageLoadMetricsObservers.
//
// To use it, simply instantiate it with a given WebContents, along with a
// callback used to register observers to a given PageLoadTracker.
class PageLoadMetricsObserverTester : public test::WeakMockTimerProvider {
 public:
  using RegisterObserversCallback =
      base::RepeatingCallback<void(PageLoadTracker*)>;
  PageLoadMetricsObserverTester(content::WebContents* web_contents,
                                const RegisterObserversCallback& callback);
  ~PageLoadMetricsObserverTester() override;

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

  MetricsWebContentsObserver* observer() const;

  // Gets the PageLoadExtraInfo for the committed_load_ in observer_.
  const PageLoadExtraInfo GetPageLoadExtraInfoForCommittedLoad();

  void RegisterObservers(PageLoadTracker* tracker);

 private:
  content::WebContents* web_contents() const { return web_contents_; }

  RegisterObserversCallback register_callback_;
  content::WebContents* web_contents_;
  MetricsWebContentsObserver* observer_;

  DISALLOW_COPY_AND_ASSIGN(PageLoadMetricsObserverTester);
};

}  // namespace page_load_metrics

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PAGE_LOAD_METRICS_OBSERVER_TESTER_H_
