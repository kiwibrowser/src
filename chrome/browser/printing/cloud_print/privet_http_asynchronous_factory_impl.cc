// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_http_asynchronous_factory_impl.h"

#include <memory>

#include "chrome/browser/local_discovery/endpoint_resolver.h"
#include "chrome/browser/printing/cloud_print/privet_http_impl.h"

namespace cloud_print {

PrivetHTTPAsynchronousFactoryImpl::PrivetHTTPAsynchronousFactoryImpl(
    net::URLRequestContextGetter* request_context)
    : request_context_(request_context) {
}

PrivetHTTPAsynchronousFactoryImpl::~PrivetHTTPAsynchronousFactoryImpl() {
}

std::unique_ptr<PrivetHTTPResolution>
PrivetHTTPAsynchronousFactoryImpl::CreatePrivetHTTP(
    const std::string& service_name) {
  return std::unique_ptr<PrivetHTTPResolution>(
      new ResolutionImpl(service_name, request_context_.get()));
}

PrivetHTTPAsynchronousFactoryImpl::ResolutionImpl::ResolutionImpl(
    const std::string& service_name,
    net::URLRequestContextGetter* request_context)
    : name_(service_name),
      request_context_(request_context),
      endpoint_resolver_(new local_discovery::EndpointResolver()) {}

PrivetHTTPAsynchronousFactoryImpl::ResolutionImpl::~ResolutionImpl() {
}

const std::string&
PrivetHTTPAsynchronousFactoryImpl::ResolutionImpl::GetName() {
  return name_;
}

void PrivetHTTPAsynchronousFactoryImpl::ResolutionImpl::Start(
    const ResultCallback& callback) {
  endpoint_resolver_->Start(name_,
                            base::Bind(&ResolutionImpl::ResolveComplete,
                                       base::Unretained(this), callback));
}

void PrivetHTTPAsynchronousFactoryImpl::ResolutionImpl::Start(
    const net::HostPortPair& address,
    const ResultCallback& callback) {
  endpoint_resolver_->Start(address,
                            base::Bind(&ResolutionImpl::ResolveComplete,
                                       base::Unretained(this), callback));
}

void PrivetHTTPAsynchronousFactoryImpl::ResolutionImpl::ResolveComplete(
    const ResultCallback& callback,
    const net::IPEndPoint& endpoint) {
  if (endpoint.address().empty())
    return callback.Run(std::unique_ptr<PrivetHTTPClient>());

  net::HostPortPair new_address = net::HostPortPair::FromIPEndPoint(endpoint);
  callback.Run(std::unique_ptr<PrivetHTTPClient>(
      new PrivetHTTPClientImpl(name_, new_address, request_context_.get())));
}

}  // namespace cloud_print
