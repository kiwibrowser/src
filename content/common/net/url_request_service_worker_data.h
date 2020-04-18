// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_NET_URL_REQUEST_SERVICE_WORKER_DATA_H_
#define CONTENT_COMMON_NET_URL_REQUEST_SERVICE_WORKER_DATA_H_

#include "base/supports_user_data.h"

namespace content {

// Used to annotate all URLRequests for which the request originated in the
// Service Worker and for initial Service Worker script loads.
// Summarized this includes requests due to:
//  - fetching Service Worker script itself for installation,
//  - importing other scripts from within a Service Worker script,
//  - calling fetch() from within a Service Worker script.
class URLRequestServiceWorkerData : public base::SupportsUserData::Data {
 public:
  URLRequestServiceWorkerData();
  ~URLRequestServiceWorkerData() override;

  static const void* const kUserDataKey;
};

}  // namespace content

#endif  // CONTENT_COMMON_NET_URL_REQUEST_SERVICE_WORKER_DATA_H_
