// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PAGE_LOAD_METRICS_PAGE_LOAD_TIMING_H_
#define CHROME_COMMON_PAGE_LOAD_METRICS_PAGE_LOAD_TIMING_H_

#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/common/page_load_metrics/page_load_metrics.mojom.h"
#include "third_party/blink/public/platform/web_loading_behavior_flag.h"

namespace page_load_metrics {

// Initialize an empty PageLoadTiming with initialized empty sub-members.
mojom::PageLoadTimingPtr CreatePageLoadTiming();

bool IsEmpty(const mojom::DocumentTiming& timing);
bool IsEmpty(const mojom::PaintTiming& timing);
bool IsEmpty(const mojom::ParseTiming& timing);
bool IsEmpty(const mojom::StyleSheetTiming& timing);
bool IsEmpty(const mojom::PageLoadTiming& timing);

void InitPageLoadTimingForTest(mojom::PageLoadTiming* timing);

}  // namespace page_load_metrics

#endif  // CHROME_COMMON_PAGE_LOAD_METRICS_PAGE_LOAD_TIMING_H_
