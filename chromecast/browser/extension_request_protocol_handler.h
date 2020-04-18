// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_EXTENION_REQUEST_PROTOCOL_HANDLER_H_
#define CHROMECAST_BROWSER_EXTENION_REQUEST_PROTOCOL_HANDLER_H_

#include "net/url_request/url_request_job_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {
class InfoMap;
}

namespace chromecast {

class ExtensionRequestProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  explicit ExtensionRequestProtocolHandler(
      content::BrowserContext* browser_context);
  ~ExtensionRequestProtocolHandler() override;

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  content::BrowserContext* const browser_context_;
  mutable const extensions::InfoMap* info_map_ = nullptr;
  mutable std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
      default_handler_;
};

}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_EXTENION_REQUEST_PROTOCOL_HANDLER_H_
