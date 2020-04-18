// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/net/url_request_service_worker_data.h"

namespace content {

URLRequestServiceWorkerData::URLRequestServiceWorkerData() {}

URLRequestServiceWorkerData::~URLRequestServiceWorkerData() {}

// static
const void* const URLRequestServiceWorkerData::kUserDataKey =
    &URLRequestServiceWorkerData::kUserDataKey;

}  // namespace content
