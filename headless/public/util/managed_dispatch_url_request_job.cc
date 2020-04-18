// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/managed_dispatch_url_request_job.h"

#include "headless/public/util/url_request_dispatcher.h"

namespace headless {

ManagedDispatchURLRequestJob::ManagedDispatchURLRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    URLRequestDispatcher* url_request_dispatcher)
    : net::URLRequestJob(request, network_delegate),
      url_request_dispatcher_(url_request_dispatcher),
      weak_ptr_factory_(this) {
  url_request_dispatcher_->JobCreated(this);
}

ManagedDispatchURLRequestJob::~ManagedDispatchURLRequestJob() {
  // We can be fairly certain the renderer has finished by the time the job gets
  // deleted.
  url_request_dispatcher_->JobDeleted(this);
}

void ManagedDispatchURLRequestJob::Kill() {
  url_request_dispatcher_->JobKilled(this);
  URLRequestJob::Kill();
}

void ManagedDispatchURLRequestJob::DispatchStartError(net::Error error) {
  url_request_dispatcher_->JobFailed(this, error);
}

void ManagedDispatchURLRequestJob::DispatchHeadersComplete() {
  url_request_dispatcher_->DataReady(this);
}

void ManagedDispatchURLRequestJob::OnHeadersComplete() {
  NotifyHeadersComplete();
}

void ManagedDispatchURLRequestJob::OnStartError(net::Error error) {
  NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED, error));
}

}  // namespace headless
