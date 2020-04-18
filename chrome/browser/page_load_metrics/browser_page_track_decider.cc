// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/browser_page_track_decider.h"

#include <string>

#include "chrome/browser/page_load_metrics/page_load_metrics_embedder_interface.h"
#include "content/public/browser/navigation_handle.h"
#include "net/http/http_response_headers.h"

namespace page_load_metrics {

BrowserPageTrackDecider::BrowserPageTrackDecider(
    PageLoadMetricsEmbedderInterface* embedder_interface,
    content::NavigationHandle* navigation_handle)
    : embedder_interface_(embedder_interface),
      navigation_handle_(navigation_handle) {}

BrowserPageTrackDecider::~BrowserPageTrackDecider() {}

bool BrowserPageTrackDecider::HasCommitted() {
  return navigation_handle_->HasCommitted();
}

bool BrowserPageTrackDecider::IsHttpOrHttpsUrl() {
  return navigation_handle_->GetURL().SchemeIsHTTPOrHTTPS();
}

bool BrowserPageTrackDecider::IsNewTabPageUrl() {
  return embedder_interface_->IsNewTabPageUrl(navigation_handle_->GetURL());
}

bool BrowserPageTrackDecider::IsChromeErrorPage() {
  DCHECK(HasCommitted());
  return navigation_handle_->IsErrorPage();
}

int BrowserPageTrackDecider::GetHttpStatusCode() {
  DCHECK(HasCommitted());
  const net::HttpResponseHeaders* response_headers =
      navigation_handle_->GetResponseHeaders();
  if (!response_headers)
    return -1;
  return response_headers->response_code();
}

}  // namespace page_load_metrics
