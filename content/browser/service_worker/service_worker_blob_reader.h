// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_BLOB_READER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_BLOB_READER_H_

#include <memory>

#include "content/browser/service_worker/service_worker_url_request_job.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_request.h"

namespace content {

// Reads a blob response for ServiceWorkerURLRequestJob.
// Owned by ServiceWorkerURLRequestJob.
class ServiceWorkerBlobReader : public net::URLRequest::Delegate {
 public:
  explicit ServiceWorkerBlobReader(ServiceWorkerURLRequestJob* owner);
  ~ServiceWorkerBlobReader() override;

  // Starts reading the blob. owner_->OnResponseStarted will be called when the
  // response starts.
  void Start(std::unique_ptr<storage::BlobDataHandle> blob_data_handle,
             const net::URLRequestContext* request_context);

  scoped_refptr<net::IOBufferWithSize> response_metadata() {
    return blob_request_->response_info().metadata;
  }

  // Same as URLRequestJob::ReadRawData. If ERR_IO_PENDING is returned,
  // owner_->OnReadRawDataComplete will be called when the read completes.
  int ReadRawData(net::IOBuffer* buf, int buf_size);

  // net::URLRequest::Delegate overrides
  void OnReceivedRedirect(net::URLRequest* request,
                          const net::RedirectInfo& redirect_info,
                          bool* defer_redirect) override;
  void OnAuthRequired(net::URLRequest* request,
                      net::AuthChallengeInfo* auth_info) override;
  void OnCertificateRequested(
      net::URLRequest* request,
      net::SSLCertRequestInfo* cert_request_info) override;
  void OnSSLCertificateError(net::URLRequest* request,
                             const net::SSLInfo& ssl_info,
                             bool fatal) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;

 private:
  ServiceWorkerURLRequestJob* owner_;
  std::unique_ptr<net::URLRequest> blob_request_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerBlobReader);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_BLOB_READER_H_
