// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_IN_MEMORY_URL_REQUEST_JOB_H_
#define HEADLESS_PUBLIC_UTIL_IN_MEMORY_URL_REQUEST_JOB_H_

#include "base/memory/weak_ptr.h"
#include "headless/public/util/in_memory_protocol_handler.h"
#include "net/url_request/url_request_job.h"

namespace headless {
class InMemoryRequestJob : public net::URLRequestJob {
 public:
  InMemoryRequestJob(net::URLRequest* request,
                     net::NetworkDelegate* network_delegate,
                     const InMemoryProtocolHandler::Response* response);

  // net::URLRequestJob implementation:
  void Start() override;
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  bool GetMimeType(std::string* mime_type) const override;

 private:
  ~InMemoryRequestJob() override;

  void StartAsync();

  const InMemoryProtocolHandler::Response* response_;  // NOT OWNED
  size_t data_offset_;

  base::WeakPtrFactory<InMemoryRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryRequestJob);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_IN_MEMORY_URL_REQUEST_JOB_H_
