// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_BACKGROUND_FETCH_WEB_BACKGROUND_FETCH_SETTLED_FETCH_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_BACKGROUND_FETCH_WEB_BACKGROUND_FETCH_SETTLED_FETCH_H_

#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_request.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_response.h"
#include "third_party/blink/public/platform/web_common.h"

namespace blink {

// Represents a request/response pair for a settled Background Fetch.
// Analogous to the following structure in the spec:
// http://wicg.github.io/background-fetch/#backgroundfetchsettledfetch
struct WebBackgroundFetchSettledFetch {
  WebBackgroundFetchSettledFetch() = default;
  ~WebBackgroundFetchSettledFetch() = default;

  WebServiceWorkerRequest request;
  WebServiceWorkerResponse response;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_BACKGROUND_FETCH_WEB_BACKGROUND_FETCH_SETTLED_FETCH_H_
