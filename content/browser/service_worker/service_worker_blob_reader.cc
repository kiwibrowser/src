// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_blob_reader.h"

#include <utility>

#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_url_request_job_factory.h"

namespace content {

ServiceWorkerBlobReader::ServiceWorkerBlobReader(
    ServiceWorkerURLRequestJob* owner)
    : owner_(owner) {}

ServiceWorkerBlobReader::~ServiceWorkerBlobReader() {}

void ServiceWorkerBlobReader::Start(
    std::unique_ptr<storage::BlobDataHandle> blob_data_handle,
    const net::URLRequestContext* request_context) {
  blob_request_ = storage::BlobProtocolHandler::CreateBlobRequest(
      std::move(blob_data_handle), request_context, this);
  blob_request_->Start();
}

void ServiceWorkerBlobReader::OnReceivedRedirect(
    net::URLRequest* request,
    const net::RedirectInfo& redirect_info,
    bool* defer_redirect) {
  NOTREACHED();
}

void ServiceWorkerBlobReader::OnAuthRequired(
    net::URLRequest* request,
    net::AuthChallengeInfo* auth_info) {
  NOTREACHED();
}

void ServiceWorkerBlobReader::OnCertificateRequested(
    net::URLRequest* request,
    net::SSLCertRequestInfo* cert_request_info) {
  NOTREACHED();
}

void ServiceWorkerBlobReader::OnSSLCertificateError(
    net::URLRequest* request,
    const net::SSLInfo& ssl_info,
    bool fatal) {
  NOTREACHED();
}

void ServiceWorkerBlobReader::OnResponseStarted(net::URLRequest* request,
                                                int net_error) {
  // TODO(falken): This should check net_error per URLRequest::Delegate
  // contract.
  // TODO(falken): Add Content-Length, Content-Type if they were not provided in
  // the ServiceWorkerResponse.
  owner_->OnResponseStarted();
}

void ServiceWorkerBlobReader::OnReadCompleted(net::URLRequest* request,
                                              int bytes_read) {
  if (!request->status().is_success()) {
    owner_->RecordResult(ServiceWorkerMetrics::REQUEST_JOB_ERROR_BLOB_READ);
  } else if (bytes_read == 0) {
    owner_->RecordResult(ServiceWorkerMetrics::REQUEST_JOB_BLOB_RESPONSE);
  }
  net::URLRequestStatus status = request->status();
  owner_->OnReadRawDataComplete(status.is_success() ? bytes_read
                                                    : status.error());
}

int ServiceWorkerBlobReader::ReadRawData(net::IOBuffer* buf, int buf_size) {
  int bytes_read = 0;
  blob_request_->Read(buf, buf_size, &bytes_read);
  net::URLRequestStatus status = blob_request_->status();
  if (status.status() != net::URLRequestStatus::SUCCESS)
    return status.error();
  if (bytes_read == 0) {
    owner_->RecordResult(ServiceWorkerMetrics::REQUEST_JOB_BLOB_RESPONSE);
  }
  return bytes_read;
}

}  // namespace content
