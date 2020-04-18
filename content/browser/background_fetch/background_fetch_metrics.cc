// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_metrics.h"

#include "base/metrics/histogram_macros.h"

namespace content {

namespace background_fetch {

void RecordSchedulerFinishedError(blink::mojom::BackgroundFetchError error) {
  UMA_HISTOGRAM_ENUMERATION("BackgroundFetch.SchedulerFinishedError", error);
}

void RecordRegistrationCreatedError(blink::mojom::BackgroundFetchError error) {
  UMA_HISTOGRAM_ENUMERATION("BackgroundFetch.RegistrationCreatedError", error);
}

void RecordRegistrationDeletedError(blink::mojom::BackgroundFetchError error) {
  UMA_HISTOGRAM_ENUMERATION("BackgroundFetch.RegistrationDeletedError", error);
}

}  // namespace background_fetch

}  // namespace content
