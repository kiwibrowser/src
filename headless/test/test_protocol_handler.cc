// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/test/test_protocol_handler.h"

#include "headless/test/test_url_request_job.h"

namespace headless {

TestProtocolHandler::TestProtocolHandler(const std::string& body)
    : body_(body) {}

net::URLRequestJob* TestProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  last_http_request_headers_ = request->extra_request_headers();
  return new TestURLRequestJob(request, network_delegate, body_);
}

}  // namespace headless
