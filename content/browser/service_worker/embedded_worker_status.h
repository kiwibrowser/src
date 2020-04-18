// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_STATUS_H_
#define CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_STATUS_H_

namespace content {

enum class EmbeddedWorkerStatus {
  STOPPED,
  STARTING,
  RUNNING,
  STOPPING,
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_EMBEDDED_WORKER_STATUS_H_
