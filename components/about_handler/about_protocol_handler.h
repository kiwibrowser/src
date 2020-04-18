// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ABOUT_HANDLER_ABOUT_PROTOCOL_HANDLER_H_
#define COMPONENTS_ABOUT_HANDLER_ABOUT_PROTOCOL_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "net/url_request/url_request_job_factory.h"

namespace about_handler {

// Implements a ProtocolHandler for About jobs.
class AboutProtocolHandler : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  AboutProtocolHandler();
  ~AboutProtocolHandler() override;
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;
  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AboutProtocolHandler);
};

}  // namespace about_handler

#endif  // COMPONENTS_ABOUT_HANDLER_ABOUT_PROTOCOL_HANDLER_H_
