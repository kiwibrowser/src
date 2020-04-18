// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_status_code.h"

#include "base/logging.h"

namespace content {

const char* ServiceWorkerStatusToString(ServiceWorkerStatusCode status) {
  switch (status) {
    case SERVICE_WORKER_OK:
      return "Operation has succeeded";
    case SERVICE_WORKER_ERROR_FAILED:
      return "Operation has failed (unknown reason)";
    case SERVICE_WORKER_ERROR_ABORT:
      return "Operation has been aborted";
    case SERVICE_WORKER_ERROR_PROCESS_NOT_FOUND:
      return "Could not find a renderer process to run a service worker";
    case SERVICE_WORKER_ERROR_NOT_FOUND:
      return "Not found";
    case SERVICE_WORKER_ERROR_EXISTS:
      return "Already exists";
    case SERVICE_WORKER_ERROR_START_WORKER_FAILED:
      return "ServiceWorker cannot be started";
    case SERVICE_WORKER_ERROR_INSTALL_WORKER_FAILED:
      return "ServiceWorker failed to install";
    case SERVICE_WORKER_ERROR_ACTIVATE_WORKER_FAILED:
      return "ServiceWorker failed to activate";
    case SERVICE_WORKER_ERROR_IPC_FAILED:
      return "IPC connection was closed or IPC error has occurred";
    case SERVICE_WORKER_ERROR_NETWORK:
      return "Operation failed by network issue";
    case SERVICE_WORKER_ERROR_SECURITY:
      return "Operation failed by security issue";
    case SERVICE_WORKER_ERROR_EVENT_WAITUNTIL_REJECTED:
      return "ServiceWorker failed to handle event (event.waitUntil "
             "Promise rejected)";
    case SERVICE_WORKER_ERROR_STATE:
      return "The ServiceWorker state was not valid";
    case SERVICE_WORKER_ERROR_TIMEOUT:
      return "The ServiceWorker timed out";
    case SERVICE_WORKER_ERROR_SCRIPT_EVALUATE_FAILED:
      return "ServiceWorker script evaluation failed";
    case SERVICE_WORKER_ERROR_DISK_CACHE:
      return "Disk cache error";
    case SERVICE_WORKER_ERROR_REDUNDANT:
      return "Redundant worker";
    case SERVICE_WORKER_ERROR_DISALLOWED:
      return "Worker disallowed";
    case SERVICE_WORKER_ERROR_MAX_VALUE:
      NOTREACHED();
  }
  NOTREACHED();
  return "";
}

}  // namespace content
