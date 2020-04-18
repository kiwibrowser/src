// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_url_job_wrapper.h"

#include "content/browser/service_worker/service_worker_navigation_loader.h"
#include "content/browser/service_worker/service_worker_url_request_job.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/content_switches.h"

namespace content {

ServiceWorkerURLJobWrapper::ServiceWorkerURLJobWrapper(
    base::WeakPtr<ServiceWorkerURLRequestJob> url_request_job)
    : url_request_job_(std::move(url_request_job)) {}

ServiceWorkerURLJobWrapper::ServiceWorkerURLJobWrapper(
    std::unique_ptr<ServiceWorkerNavigationLoader> url_loader_job)
    : url_loader_job_(std::move(url_loader_job)) {}

ServiceWorkerURLJobWrapper::~ServiceWorkerURLJobWrapper() {
  if (url_loader_job_) {
    // Detach the URLLoader from this wrapper (and therefore
    // from the request handler). This may delete the
    // |url_loader_job_| or may make it self-owned if it is
    // bound to a Mojo endpoint.
    url_loader_job_.release()->DetachedFromRequest();
  }
}

void ServiceWorkerURLJobWrapper::FallbackToNetwork() {
  if (url_loader_job_) {
    url_loader_job_->FallbackToNetwork();
  } else {
    url_request_job_->FallbackToNetwork();
  }
}

void ServiceWorkerURLJobWrapper::FallbackToNetworkOrRenderer() {
  if (url_loader_job_) {
    url_loader_job_->FallbackToNetworkOrRenderer();
  } else {
    url_request_job_->FallbackToNetworkOrRenderer();
  }
}

void ServiceWorkerURLJobWrapper::ForwardToServiceWorker() {
  if (url_loader_job_) {
    url_loader_job_->ForwardToServiceWorker();
  } else {
    url_request_job_->ForwardToServiceWorker();
  }
}

bool ServiceWorkerURLJobWrapper::ShouldFallbackToNetwork() {
  if (url_loader_job_) {
    return url_loader_job_->ShouldFallbackToNetwork();
  } else {
    return url_request_job_->ShouldFallbackToNetwork();
  }
}

void ServiceWorkerURLJobWrapper::FailDueToLostController() {
  if (url_loader_job_) {
    url_loader_job_->FailDueToLostController();
  } else {
    url_request_job_->FailDueToLostController();
  }
}

bool ServiceWorkerURLJobWrapper::WasCanceled() const {
  if (url_loader_job_) {
    return url_loader_job_->WasCanceled();
  } else {
    return !url_request_job_;
  }
}

}  // namespace content
