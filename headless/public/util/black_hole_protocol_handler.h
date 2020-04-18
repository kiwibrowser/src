// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_BLACK_HOLE_PROTOCOL_HANDLER_H_
#define HEADLESS_PUBLIC_UTIL_BLACK_HOLE_PROTOCOL_HANDLER_H_

#include <map>

#include "net/url_request/url_request_job_factory.h"

namespace headless {

// A protocol handler that fails any request sent to it.
class BlackHoleProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  BlackHoleProtocolHandler();
  ~BlackHoleProtocolHandler() override;

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BlackHoleProtocolHandler);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_BLACK_HOLE_PROTOCOL_HANDLER_H_
