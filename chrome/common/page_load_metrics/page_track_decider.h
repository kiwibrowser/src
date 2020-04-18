// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PAGE_LOAD_METRICS_PAGE_TRACK_DECIDER_H_
#define CHROME_COMMON_PAGE_LOAD_METRICS_PAGE_TRACK_DECIDER_H_

#include "base/macros.h"

namespace page_load_metrics {

// PageTrackDecider is an interface used to determine whether metrics should be
// tracked for a given page. This class is used by the core page load metrics
// tracking infrastructure in both the browser and render processes so policy
// decisions about which page loads to track are consistent across processes.
class PageTrackDecider {
 public:
  PageTrackDecider();
  virtual ~PageTrackDecider();

  // Whether metrics should be tracked for the current page. This method invokes
  // the pure virtual methods below to decide whether metrics should be tracked.
  bool ShouldTrack();

  // Whether the navigation for the current page has committed.
  virtual bool HasCommitted() = 0;

  // Whether the current page's URL is HTTP or HTTPS. Note that the page does
  // not need to have committed, nor does the URL or hostname of the URL need to
  // point at a valid web page or host, for this method to return true.
  virtual bool IsHttpOrHttpsUrl() = 0;

  // Whether the current page's URL is for a Chrome new tab page. Note that in
  // some cases the new tab page is served over the network via https.
  virtual bool IsNewTabPageUrl() = 0;

  // The methods below can only be called if HasCommitted() returns true.

  // Whether the current page is a Chrome-generated error page. Example error
  // pages include pages displayed after a network error that did not result in
  // receiving an HTTP/HTTPS response, such as the 'There is no Internet
  // connection' page, which is displayed when the user attempts to navigate
  // without an Internet connection. This method returns false for non-Chrome
  // generated error pages, such as pages with 4xx/5xx response codes. To
  // check whether the current page resulted in a 4xx/5xx error, use
  // IsHTTPErrorPage() instead.
  virtual bool IsChromeErrorPage() = 0;

  // Returns the HTTP status code for the current page, or -1 if no status code
  // is available.
  virtual int GetHttpStatusCode() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(PageTrackDecider);
};

}  // namespace page_load_metrics

#endif  // CHROME_COMMON_PAGE_LOAD_METRICS_PAGE_TRACK_DECIDER_H_
