// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webrunner/browser/webrunner_url_request_context_getter.h"

#include <utility>

#include "base/single_thread_task_runner.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"

namespace webrunner {

WebRunnerURLRequestContextGetter::WebRunnerURLRequestContextGetter(
    scoped_refptr<base::SingleThreadTaskRunner> network_task_runner,
    net::NetLog* net_log,
    content::ProtocolHandlerMap protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors)
    : network_task_runner_(std::move(network_task_runner)),
      net_log_(net_log),
      protocol_handlers_(std::move(protocol_handlers)),
      request_interceptors_(std::move(request_interceptors)) {}

WebRunnerURLRequestContextGetter::~WebRunnerURLRequestContextGetter() = default;

net::URLRequestContext*
WebRunnerURLRequestContextGetter::GetURLRequestContext() {
  if (!url_request_context_) {
    net::URLRequestContextBuilder builder;
    builder.set_net_log(net_log_);

    for (auto& protocol_handler : protocol_handlers_) {
      builder.SetProtocolHandler(protocol_handler.first,
                                 std::move(protocol_handler.second));
    }
    protocol_handlers_.clear();

    builder.SetInterceptors(std::move(request_interceptors_));

    // TODO(sergeyu): Configure CookieStore, cache and proxy resolver.
    url_request_context_ = builder.Build();
  }
  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
WebRunnerURLRequestContextGetter::GetNetworkTaskRunner() const {
  return network_task_runner_;
}

}  // namespace webrunner
