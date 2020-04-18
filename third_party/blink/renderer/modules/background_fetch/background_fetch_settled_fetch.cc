// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_fetch.h"

#include "third_party/blink/renderer/core/fetch/response.h"

namespace blink {

BackgroundFetchSettledFetch::BackgroundFetchSettledFetch(Request* request,
                                                         Response* response)
    : BackgroundFetchFetch(request), response_(response) {}

Response* BackgroundFetchSettledFetch::response() const {
  return response_;
}

void BackgroundFetchSettledFetch::Trace(blink::Visitor* visitor) {
  visitor->Trace(response_);
  BackgroundFetchFetch::Trace(visitor);
}

}  // namespace blink
