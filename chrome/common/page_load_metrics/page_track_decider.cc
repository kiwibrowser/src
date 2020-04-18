// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/page_load_metrics/page_track_decider.h"

namespace page_load_metrics {

PageTrackDecider::PageTrackDecider() {}
PageTrackDecider::~PageTrackDecider() {}

bool PageTrackDecider::ShouldTrack() {
  // Ignore non-HTTP schemes (e.g. chrome://).
  if (!IsHttpOrHttpsUrl())
    return false;

  // Ignore NTP loads.
  if (IsNewTabPageUrl())
    return false;

  if (HasCommitted()) {
    // Ignore Chrome error pages (e.g. No Internet connection).
    if (IsChromeErrorPage())
      return false;

    // Ignore network error pages (e.g. 4xx, 5xx).
    int http_status_code = GetHttpStatusCode();
    if (http_status_code > 0 &&
        (http_status_code < 200 || http_status_code >= 400))
      return false;
  }

  return true;
}

}  // namespace page_load_metrics
