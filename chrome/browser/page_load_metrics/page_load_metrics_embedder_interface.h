// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_EMBEDDER_INTERFACE_H_
#define CHROME_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_EMBEDDER_INTERFACE_H_

#include <memory>

class GURL;

namespace base {
class Timer;
}  // namespace base

namespace page_load_metrics {

class PageLoadTracker;

// This class serves as a functional interface to various chrome// features.
// Impl version is defined in chrome/browser/page_load_metrics.
class PageLoadMetricsEmbedderInterface {
 public:
  virtual ~PageLoadMetricsEmbedderInterface() {}
  virtual bool IsNewTabPageUrl(const GURL& url) = 0;
  virtual void RegisterObservers(PageLoadTracker* metrics) = 0;
  virtual std::unique_ptr<base::Timer> CreateTimer() = 0;
};

}  // namespace page_load_metrics

#endif  // CHROME_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_EMBEDDER_INTERFACE_H_
