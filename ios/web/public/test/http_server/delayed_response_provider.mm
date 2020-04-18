// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/http_server/delayed_response_provider.h"

#import <Foundation/Foundation.h>

#include "base/bind.h"
#import "base/mac/foundation_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/test/embedded_test_server/http_response.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

// Delays |delay| seconds before sending a response to the client.
class DelayedHttpResponse : public net::test_server::BasicHttpResponse {
 public:
  explicit DelayedHttpResponse(double delay) : delay_(delay) {}

  void SendResponse(
      const net::test_server::SendBytesCallback& send,
      const net::test_server::SendCompleteCallback& done) override {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(send, ToResponseString(), done),
        base::TimeDelta::FromSecondsD(delay_));
  }

 private:
  const double delay_;

  DISALLOW_COPY_AND_ASSIGN(DelayedHttpResponse);
};

DelayedResponseProvider::DelayedResponseProvider(
    std::unique_ptr<web::ResponseProvider> delayed_provider,
    double delay)
    : web::ResponseProvider(),
      delayed_provider_(std::move(delayed_provider)),
      delay_(delay) {}

DelayedResponseProvider::~DelayedResponseProvider() {}

bool DelayedResponseProvider::CanHandleRequest(const Request& request) {
  return delayed_provider_->CanHandleRequest(request);
}

std::unique_ptr<net::test_server::HttpResponse>
DelayedResponseProvider::GetEmbeddedTestServerResponse(const Request& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      std::make_unique<DelayedHttpResponse>(delay_));
  http_response->set_content_type("text/html");
  http_response->set_content("Slow Page");
  return std::move(http_response);
}
}  // namespace web
