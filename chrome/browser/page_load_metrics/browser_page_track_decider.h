// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_BROWSER_PAGE_TRACK_DECIDER_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_BROWSER_PAGE_TRACK_DECIDER_H_

#include "base/macros.h"
#include "chrome/common/page_load_metrics/page_track_decider.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace page_load_metrics {

class PageLoadMetricsEmbedderInterface;

class BrowserPageTrackDecider : public PageTrackDecider {
 public:
  // embedder_interface and navigation_handle are not owned by
  // BrowserPageTrackDecider, and must outlive the
  // BrowserPageTrackDecider.
  BrowserPageTrackDecider(PageLoadMetricsEmbedderInterface* embedder_interface,
                          content::NavigationHandle* navigation_handle);
  ~BrowserPageTrackDecider() override;

  bool HasCommitted() override;
  bool IsHttpOrHttpsUrl() override;
  bool IsNewTabPageUrl() override;
  bool IsChromeErrorPage() override;
  int GetHttpStatusCode() override;

 private:
  PageLoadMetricsEmbedderInterface* const embedder_interface_;
  content::NavigationHandle* const navigation_handle_;

  DISALLOW_COPY_AND_ASSIGN(BrowserPageTrackDecider);
};

}  // namespace page_load_metrics

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_BROWSER_PAGE_TRACK_DECIDER_H_
