// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_SLOW_DOWNLOAD_HTTP_RESPONSE_H_
#define CONTENT_PUBLIC_TEST_SLOW_DOWNLOAD_HTTP_RESPONSE_H_

#include <set>
#include <string>

#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace content {

/*
 * Download response that won't complete until |kFinishDownloadUrl| request is
 * received.
 */
class SlowDownloadHttpResponse : public net::test_server::HttpResponse {
 public:
  // Test URLs.
  static const char kSlowDownloadHostName[];
  static const char kUnknownSizeUrl[];
  static const char kKnownSizeUrl[];
  static const char kFinishDownloadUrl[];
  static const int kFirstDownloadSize;
  static const int kSecondDownloadSize;

  static std::unique_ptr<net::test_server::HttpResponse>
  HandleSlowDownloadRequest(const net::test_server::HttpRequest& request);

  SlowDownloadHttpResponse(const std::string& url);
  ~SlowDownloadHttpResponse() override;

  // net::test_server::HttpResponse implementations.
  void SendResponse(
      const net::test_server::SendBytesCallback& send,
      const net::test_server::SendCompleteCallback& done) override;

 private:
  std::string url_;

  DISALLOW_COPY_AND_ASSIGN(SlowDownloadHttpResponse);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_SLOW_DOWNLOAD_HTTP_RESPONSE_H_
