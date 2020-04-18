// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_TESTING_FAKE_MANAGED_DISPATCH_URL_REQUEST_JOB_H_
#define HEADLESS_PUBLIC_UTIL_TESTING_FAKE_MANAGED_DISPATCH_URL_REQUEST_JOB_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "headless/public/util/managed_dispatch_url_request_job.h"
#include "net/base/net_errors.h"

namespace headless {

class URLRequestDispatcher;

class FakeManagedDispatchURLRequestJob : public ManagedDispatchURLRequestJob {
 public:
  FakeManagedDispatchURLRequestJob(URLRequestDispatcher* url_request_dispatcher,
                                   int id,
                                   std::vector<std::string>* notifications)
      : ManagedDispatchURLRequestJob(nullptr, nullptr, url_request_dispatcher),
        id_(id),
        notifications_(notifications) {}

  ~FakeManagedDispatchURLRequestJob() override {}

  using ManagedDispatchURLRequestJob::DispatchHeadersComplete;
  using ManagedDispatchURLRequestJob::DispatchStartError;

  void Kill() override;

  void Start() override {}

  void OnHeadersComplete() override;

  void OnStartError(net::Error error) override;

 private:
  int id_;
  std::vector<std::string>* notifications_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(FakeManagedDispatchURLRequestJob);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_TESTING_FAKE_MANAGED_DISPATCH_URL_REQUEST_JOB_H_
