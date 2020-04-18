// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/deterministic_http_protocol_handler.h"

#include <memory>

#include "base/macros.h"
#include "headless/public/headless_browser_context.h"
#include "headless/public/util/deterministic_dispatcher.h"
#include "headless/public/util/generic_url_request_job.h"
#include "headless/public/util/http_url_fetcher.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job_factory_impl.h"

namespace headless {

class DeterministicHttpProtocolHandler::NopGenericURLRequestJobDelegate
    : public GenericURLRequestJob::Delegate {
 public:
  NopGenericURLRequestJobDelegate() = default;
  ~NopGenericURLRequestJobDelegate() override = default;

  void OnResourceLoadFailed(const Request* request, net::Error error) override {
  }

  void OnResourceLoadComplete(
      const Request* request,
      const GURL& final_url,
      scoped_refptr<net::HttpResponseHeaders> response_headers,
      const char* body,
      size_t body_size) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(NopGenericURLRequestJobDelegate);
};

DeterministicHttpProtocolHandler::DeterministicHttpProtocolHandler(
    DeterministicDispatcher* deterministic_dispatcher,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : deterministic_dispatcher_(deterministic_dispatcher),
      headless_browser_context_(nullptr),
      io_task_runner_(io_task_runner),
      nop_delegate_(new NopGenericURLRequestJobDelegate()) {}

DeterministicHttpProtocolHandler::~DeterministicHttpProtocolHandler() {
  if (url_request_context_)
    io_task_runner_->DeleteSoon(FROM_HERE, url_request_context_.release());
  if (url_request_job_factory_)
    io_task_runner_->DeleteSoon(FROM_HERE, url_request_job_factory_.release());
}

net::URLRequestJob* DeterministicHttpProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  if (!url_request_context_) {
    DCHECK(io_task_runner_->BelongsToCurrentThread());
    // Create our own URLRequestContext with an empty URLRequestJobFactoryImpl
    // which lets us use the default http(s) RequestJobs.
    url_request_context_.reset(new net::URLRequestContext());
    url_request_context_->CopyFrom(request->context());
    url_request_job_factory_.reset(new net::URLRequestJobFactoryImpl());
    url_request_context_->set_job_factory(url_request_job_factory_.get());
  }
  return new GenericURLRequestJob(
      request, network_delegate, deterministic_dispatcher_,
      std::make_unique<HttpURLFetcher>(url_request_context_.get()),
      nop_delegate_.get(), headless_browser_context_);
}

}  // namespace headless
