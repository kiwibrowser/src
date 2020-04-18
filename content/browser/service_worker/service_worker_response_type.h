// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_RESPONSE_TYPE_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_RESPONSE_TYPE_H_

namespace content {

// Response handling type, used in URL {request,loader} jobs.
enum class ServiceWorkerResponseType {
  NOT_DETERMINED,
  FAIL_DUE_TO_LOST_CONTROLLER,
  FALLBACK_TO_NETWORK,
  FALLBACK_TO_RENDERER,  // Use this when falling back with CORS check
  FORWARD_TO_SERVICE_WORKER
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_RESPONSE_TYPE_H_
