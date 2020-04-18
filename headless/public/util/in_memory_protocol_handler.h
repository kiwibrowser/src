// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_IN_MEMORY_PROTOCOL_HANDLER_H_
#define HEADLESS_PUBLIC_UTIL_IN_MEMORY_PROTOCOL_HANDLER_H_

#include <map>

#include "net/url_request/url_request_job_factory.h"

namespace headless {

class InMemoryProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  InMemoryProtocolHandler();
  ~InMemoryProtocolHandler() override;

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  struct Response {
    Response() {}
    Response(const std::string& data, const std::string& mime_type)
        : data(data), mime_type(mime_type) {}

    std::string data;
    std::string mime_type;
  };

  void InsertResponse(const std::string& request_path,
                      const Response& response);

 private:
  const Response* GetResponse(const std::string& request_path) const;

  std::map<std::string, Response> response_map_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryProtocolHandler);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_IN_MEMORY_PROTOCOL_HANDLER_H_
