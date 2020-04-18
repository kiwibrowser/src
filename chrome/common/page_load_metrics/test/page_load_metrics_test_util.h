// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PAGE_LOAD_METRICS_TEST_PAGE_LOAD_METRICS_TEST_UTIL_H_
#define CHROME_COMMON_PAGE_LOAD_METRICS_TEST_PAGE_LOAD_METRICS_TEST_UTIL_H_

namespace page_load_metrics {
namespace mojom {
class PageLoadTiming;
}  // namespace mojom
}  // namespace page_load_metrics

// Helper that fills in any timing fields that page load metrics requires but
// that are currently missing.
void PopulateRequiredTimingFields(
    page_load_metrics::mojom::PageLoadTiming* inout_timing);

#endif  // CHROME_COMMON_PAGE_LOAD_METRICS_TEST_PAGE_LOAD_METRICS_TEST_UTIL_H_
