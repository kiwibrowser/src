// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_DETERMINISTIC_HTTP_PROTOCOL_HANDLER_H_
#define HEADLESS_PUBLIC_UTIL_DETERMINISTIC_HTTP_PROTOCOL_HANDLER_H_

#include <memory>

#include "base/single_thread_task_runner.h"
#include "headless/public/headless_export.h"
#include "net/url_request/url_request_job_factory.h"

namespace net {
class URLRequestContext;
class URLRequestJobFactory;
}  // namespace

namespace headless {
class DeterministicDispatcher;
class HeadlessBrowserContext;

// A deterministic protocol handler.  Requests made to this protocol handler
// will return in order of creation, regardless of what order the network
// returns them in.  This helps remove one large source of network related
// non determinism at the cost of slower page loads.
class HEADLESS_EXPORT DeterministicHttpProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  // Note |deterministic_dispatcher| is expected to be shared across a number of
  // protocol handlers, e.g. for http & https protocols.
  DeterministicHttpProtocolHandler(
      DeterministicDispatcher* deterministic_dispatcher,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);
  ~DeterministicHttpProtocolHandler() override;

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  void SetHeadlessBrowserContext(
      HeadlessBrowserContext* headless_browser_context) {
    headless_browser_context_ = headless_browser_context;
  }

 private:
  class NopGenericURLRequestJobDelegate;

  DeterministicDispatcher* deterministic_dispatcher_;  // NOT OWNED.
  HeadlessBrowserContext* headless_browser_context_;   // NOT OWNED.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  std::unique_ptr<NopGenericURLRequestJobDelegate> nop_delegate_;

  // |url_request_context_| and |url_request_job_factory_| are lazily created on
  // the IO thread. The URLRequestContext is setup to bypass any user-specified
  // protocol handlers including this one. This is necessary to actually fetch
  // http resources.
  mutable std::unique_ptr<net::URLRequestContext> url_request_context_;
  mutable std::unique_ptr<net::URLRequestJobFactory> url_request_job_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeterministicHttpProtocolHandler);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_DETERMINISTIC_HTTP_PROTOCOL_HANDLER_H_
