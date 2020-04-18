// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_STATUS_CODE_H_
#define CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_STATUS_CODE_H_

#include "content/common/content_export.h"

namespace content {

// Generic service worker operation statuses.
// This enum is used in UMA histograms. Append-only.
enum ServiceWorkerStatusCode {
  // Operation succeeded.
  SERVICE_WORKER_OK = 0,

  // Generic operation error (more specific error code should be used in
  // general).
  SERVICE_WORKER_ERROR_FAILED = 1,

  // Operation was aborted (e.g. due to context or child process shutdown).
  SERVICE_WORKER_ERROR_ABORT = 2,

  // Starting a new service worker script context failed.
  SERVICE_WORKER_ERROR_START_WORKER_FAILED = 3,

  // Could not find a renderer process to run a service worker.
  SERVICE_WORKER_ERROR_PROCESS_NOT_FOUND = 4,

  // Generic error code to indicate the specified item is not found.
  SERVICE_WORKER_ERROR_NOT_FOUND = 5,

  // Generic error code to indicate the specified item already exists.
  SERVICE_WORKER_ERROR_EXISTS = 6,

  // Install event handling failed.
  SERVICE_WORKER_ERROR_INSTALL_WORKER_FAILED = 7,

  // Activate event handling failed.
  SERVICE_WORKER_ERROR_ACTIVATE_WORKER_FAILED = 8,

  // Sending an IPC to the worker failed (often due to child process is
  // terminated).
  SERVICE_WORKER_ERROR_IPC_FAILED = 9,

  // Operation is failed by network issue.
  SERVICE_WORKER_ERROR_NETWORK = 10,

  // Operation is failed by security issue.
  SERVICE_WORKER_ERROR_SECURITY = 11,

  // Event handling failed (event.waitUntil Promise rejected).
  SERVICE_WORKER_ERROR_EVENT_WAITUNTIL_REJECTED = 12,

  // An error triggered by invalid worker state.
  SERVICE_WORKER_ERROR_STATE = 13,

  // The Service Worker took too long to finish a task.
  SERVICE_WORKER_ERROR_TIMEOUT = 14,

  // An error occurred during initial script evaluation.
  SERVICE_WORKER_ERROR_SCRIPT_EVALUATE_FAILED = 15,

  // Generic error to indicate failure to read/write the disk cache.
  SERVICE_WORKER_ERROR_DISK_CACHE = 16,

  // The worker is in REDUNDANT state.
  SERVICE_WORKER_ERROR_REDUNDANT = 17,

  // The worker was disallowed (by ContentClient: e.g., due to
  // browser settings).
  SERVICE_WORKER_ERROR_DISALLOWED = 18,

  // Obsolete.
  // SERVICE_WORKER_ERROR_DISABLED_WORKER = 19,

  // Add new status codes here.

  SERVICE_WORKER_ERROR_MAX_VALUE = 20
};

CONTENT_EXPORT const char* ServiceWorkerStatusToString(
    ServiceWorkerStatusCode code);

}  // namespace content

#endif  // CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_STATUS_CODE_H_
