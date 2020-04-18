// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_TEST_TEST_PROTOCOL_HANDLER_H_
#define HEADLESS_TEST_TEST_PROTOCOL_HANDLER_H_

#include "net/http/http_request_headers.h"
#include "net/url_request/url_request_job_factory.h"

namespace headless {

class TestProtocolHandler : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  TestProtocolHandler(const std::string& body);
  ~TestProtocolHandler() override {}

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  const net::HttpRequestHeaders& last_http_request_headers() const {
    return last_http_request_headers_;
  }

 private:
  mutable net::HttpRequestHeaders last_http_request_headers_;
  std::string body_;

  DISALLOW_COPY_AND_ASSIGN(TestProtocolHandler);
};

}  // namespace headless

#endif  // HEADLESS_TEST_TEST_PROTOCOL_HANDLER_H_
