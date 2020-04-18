// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/test/test_url_request_job.h"

#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"

namespace headless {

TestURLRequestJob::TestURLRequestJob(net::URLRequest* request,
                                     net::NetworkDelegate* network_delegate,
                                     const std::string& body)
    : net::URLRequestJob(request, network_delegate),
      body_(new net::StringIOBuffer(body)),
      src_buf_(new net::DrainableIOBuffer(body_.get(), body_->size())) {}

TestURLRequestJob::~TestURLRequestJob() = default;

void TestURLRequestJob::Start() {
  NotifyHeadersComplete();
}

void TestURLRequestJob::GetResponseInfo(net::HttpResponseInfo* info) {
  info->headers =
      new net::HttpResponseHeaders("Content-Type: text/html\r\n\r\n");
}

int TestURLRequestJob::ReadRawData(net::IOBuffer* buf, int buf_size) {
  scoped_refptr<net::DrainableIOBuffer> dest_buf(
      new net::DrainableIOBuffer(buf, buf_size));
  while (src_buf_->BytesRemaining() > 0 && dest_buf->BytesRemaining() > 0) {
    *dest_buf->data() = *src_buf_->data();
    src_buf_->DidConsume(1);
    dest_buf->DidConsume(1);
  }
  return dest_buf->BytesConsumed();
}

}  // namespace headless
