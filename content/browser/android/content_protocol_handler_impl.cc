// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/content_protocol_handler_impl.h"

#include <memory>

#include "base/task_runner.h"
#include "content/browser/android/url_request_content_job.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_error_job.h"

namespace content {

// static
std::unique_ptr<ContentProtocolHandler> ContentProtocolHandler::Create(
    const scoped_refptr<base::TaskRunner>& content_task_runner) {
  return std::make_unique<ContentProtocolHandlerImpl>(content_task_runner);
}

ContentProtocolHandlerImpl::ContentProtocolHandlerImpl(
    const scoped_refptr<base::TaskRunner>& content_task_runner)
    : content_task_runner_(content_task_runner) {}

ContentProtocolHandlerImpl::~ContentProtocolHandlerImpl() {}

net::URLRequestJob* ContentProtocolHandlerImpl::MaybeCreateJob(
    net::URLRequest* request, net::NetworkDelegate* network_delegate) const {
  if (!network_delegate) {
    return new net::URLRequestErrorJob(
        request, network_delegate, net::ERR_ACCESS_DENIED);
  }
  return new URLRequestContentJob(
      request, network_delegate, base::FilePath(request->url().spec()),
      content_task_runner_);
}

bool ContentProtocolHandlerImpl::IsSafeRedirectTarget(
    const GURL& location) const {
  return false;
}

}  // namespace content
