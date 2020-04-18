// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PREVIEWS_UKM_OBSERVER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PREVIEWS_UKM_OBSERVER_H_

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"

namespace content {
class NavigationHandle;
}

namespace previews {

// Observer responsible for appending previews information to the PLM UKM
// report.
class PreviewsUKMObserver : public page_load_metrics::PageLoadMetricsObserver {
 public:
  PreviewsUKMObserver();
  ~PreviewsUKMObserver() override;

  // page_load_metrics::PageLoadMetricsObserver:
  ObservePolicy OnStart(content::NavigationHandle* navigation_handle,
                        const GURL& currently_committed_url,
                        bool started_in_foreground) override;
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;
  ObservePolicy FlushMetricsOnAppEnterBackground(
      const page_load_metrics::mojom::PageLoadTiming& timing,
      const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnComplete(const page_load_metrics::mojom::PageLoadTiming& timing,
                  const page_load_metrics::PageLoadExtraInfo& info) override;
  void OnLoadedResource(const page_load_metrics::ExtraRequestCompleteInfo&
                            extra_request_complete_info) override;
  void OnEventOccurred(const void* const event_key) override;

 protected:
  // Returns true if data saver feature is enabled in Chrome. Virtualized for
  // testing.
  virtual bool IsDataSaverEnabled(
      content::NavigationHandle* navigation_handle) const;

 private:
  void RecordPreviewsTypes(const page_load_metrics::PageLoadExtraInfo& info);

  bool server_lofi_seen_ = false;
  bool client_lofi_seen_ = false;
  bool lite_page_seen_ = false;
  bool noscript_seen_ = false;
  bool opt_out_occurred_ = false;
  bool origin_opt_out_occurred_ = false;
  bool save_data_enabled_ = false;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(PreviewsUKMObserver);
};

}  // namespace previews

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_OBSERVERS_PREVIEWS_UKM_OBSERVER_H_
