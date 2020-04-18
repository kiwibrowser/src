// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_TEST_TEST_URL_REQUEST_JOB_H_
#define HEADLESS_TEST_TEST_URL_REQUEST_JOB_H_

#include "net/url_request/url_request_job.h"

namespace net {
class StringIOBuffer;
class DrainableIOBuffer;
}

namespace headless {

class TestURLRequestJob : public net::URLRequestJob {
 public:
  TestURLRequestJob(net::URLRequest* request,
                    net::NetworkDelegate* network_delegate,
                    const std::string& body);
  ~TestURLRequestJob() override;

  // net::URLRequestJob implementation:
  void Start() override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;

 private:
  scoped_refptr<net::StringIOBuffer> body_;
  scoped_refptr<net::DrainableIOBuffer> src_buf_;

  DISALLOW_COPY_AND_ASSIGN(TestURLRequestJob);
};

}  // namespace headless

#endif  // HEADLESS_TEST_TEST_URL_REQUEST_JOB_H_
