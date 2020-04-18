// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/about_handler/about_protocol_handler.h"

#include "components/about_handler/url_request_about_job.h"

namespace about_handler {

AboutProtocolHandler::AboutProtocolHandler() {
}

AboutProtocolHandler::~AboutProtocolHandler() {
}

net::URLRequestJob* AboutProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return new URLRequestAboutJob(request, network_delegate);
}

bool AboutProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace about_handler
