// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTEXT_REQUEST_HANDLER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTEXT_REQUEST_HANDLER_H_

#include <stdint.h>

#include "base/macros.h"
#include "content/browser/service_worker/service_worker_request_handler.h"

namespace content {

class ServiceWorkerVersion;

// A request handler derivative used to handle requests from
// service workers.
class CONTENT_EXPORT ServiceWorkerContextRequestHandler
    : public ServiceWorkerRequestHandler {
 public:
  // The result status for MaybeCreateJob. Used in histograms. Append-only.
  enum class CreateJobStatus {
    // Unitialized status.
    UNINITIALIZED,
    // A ServiceWorkerWriteToCacheJob was created.
    WRITE_JOB,
    // A ServiceWorkerWriteToCacheJob was created with an incumbent script to
    // compare against.
    WRITE_JOB_WITH_INCUMBENT,
    // A ServiceWorkerReadFromCacheJob was created.
    READ_JOB,
    // A ServiceWorkerReadFromCacheJob was created for a new worker that is
    // importing a script that was already imported.
    READ_JOB_FOR_DUPLICATE_SCRIPT_IMPORT,
    // A job could not be created because there is no live
    // ServiceWorkerProviderHost.
    ERROR_NO_PROVIDER,
    // A job could not be created because the ServiceWorkerVersion is in status
    // REDUNDANT.
    ERROR_REDUNDANT_VERSION,
    // A job could not be created because there is no live
    // ServiceWorkerContextCore.
    ERROR_NO_CONTEXT,
    // A job could not be created because a redirect occurred.
    ERROR_REDIRECT,
    // A job was not created because an installed worker is importing a script
    // that was not stored at installation time.
    ERROR_UNINSTALLED_SCRIPT_IMPORT,
    // A job could not be created because there are no resource ids available.
    ERROR_OUT_OF_RESOURCE_IDS,
    // Add new types here.

    NUM_TYPES
  };

  ServiceWorkerContextRequestHandler(
      base::WeakPtr<ServiceWorkerContextCore> context,
      base::WeakPtr<ServiceWorkerProviderHost> provider_host,
      base::WeakPtr<storage::BlobStorageContext> blob_storage_context,
      ResourceType resource_type);
  ~ServiceWorkerContextRequestHandler() override;

  // Called via custom URLRequestJobFactory.
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      ResourceContext* resource_context) override;

  static std::string CreateJobStatusToString(CreateJobStatus status);

 private:
  net::URLRequestJob* MaybeCreateJobImpl(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate,
                                         CreateJobStatus* out_status);

  scoped_refptr<ServiceWorkerVersion> version_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerContextRequestHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_CONTEXT_REQUEST_HANDLER_H_
