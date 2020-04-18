// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/in_memory_protocol_handler.h"

#include "headless/public/util/in_memory_request_job.h"

namespace headless {

InMemoryProtocolHandler::InMemoryProtocolHandler() = default;
InMemoryProtocolHandler::~InMemoryProtocolHandler() = default;

net::URLRequestJob* InMemoryProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return new InMemoryRequestJob(request, network_delegate,
                                GetResponse(request->url().GetContent()));
}

void InMemoryProtocolHandler::InsertResponse(const std::string& request_path,
                                             const Response& response) {
  // GURL::GetContent excludes the scheme and : but includes the // prefix. As a
  // convenience we add it here.
  response_map_["//" + request_path] = response;
}

const InMemoryProtocolHandler::Response* InMemoryProtocolHandler::GetResponse(
    const std::string& request_path) const {
  std::map<std::string, Response>::const_iterator find_it =
      response_map_.find(request_path);
  if (find_it == response_map_.end())
    return nullptr;
  return &find_it->second;
}

}  // namespace headless
