// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_TESTING_TEST_IN_MEMORY_PROTOCOL_HANDLER_H_
#define HEADLESS_PUBLIC_UTIL_TESTING_TEST_IN_MEMORY_PROTOCOL_HANDLER_H_

#include <map>
#include <vector>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_request_job_factory.h"

namespace headless {
class ExpeditedDispatcher;
class HeadlessBrowserContext;

class TestInMemoryProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  class RequestDeferrer {
   public:
    virtual ~RequestDeferrer() {}

    // Notifies that the target page has made a request for the |url|. The
    // request will not be completed until |complete_request| is run.
    // NOTE this will be called on the IO thread.
    virtual void OnRequest(const GURL& url, base::Closure complete_request) = 0;
  };

  // Note |request_deferrer| is optional.  If the test doesn't need to control
  // when resources are fulfilled then pass in nullptr.
  TestInMemoryProtocolHandler(
      scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner,
      RequestDeferrer* request_deferrer);

  ~TestInMemoryProtocolHandler() override;

  void SetHeadlessBrowserContext(
      HeadlessBrowserContext* headless_browser_context);

  struct Response {
    Response() {}
    Response(const std::string& data) : data(data) {}
    Response(const std::string& body, const std::string& mime_type)
        : data("HTTP/1.1 200 OK\r\nContent-Type: " + mime_type + "\r\n\r\n" +
               body) {}

    std::string data;
    scoped_refptr<net::IOBufferWithSize> metadata;
  };

  void InsertResponse(const std::string& url, const Response& response);
  void SetResponseMetadata(const std::string& url,
                           scoped_refptr<net::IOBufferWithSize> metadata);

  // net::URLRequestJobFactory::ProtocolHandler implementation::
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  const std::map<std::string, std::string>& url_to_devtools_frame_id() const {
    return url_to_devtools_frame_id_;
  }

  const std::vector<std::string>& urls_requested() const {
    return urls_requested_;
  }

  const std::vector<std::string>& methods_requested() const {
    return methods_requested_;
  }

 private:
  const Response* GetResponse(const std::string& url) const;

  void RegisterUrl(const std::string& url, std::string& devtools_frame_id) {
    urls_requested_.push_back(url);
    url_to_devtools_frame_id_[url] = devtools_frame_id;
  }

  RequestDeferrer* request_deferrer() const { return request_deferrer_; }

  class TestDelegate;
  class MockURLFetcher;
  friend class TestDelegate;
  friend class MockURLFetcher;

  std::unique_ptr<TestDelegate> test_delegate_;
  std::unique_ptr<ExpeditedDispatcher> dispatcher_;
  std::map<std::string, std::unique_ptr<Response>> response_map_;
  HeadlessBrowserContext* headless_browser_context_;
  std::map<std::string, std::string> url_to_devtools_frame_id_;
  std::vector<std::string> urls_requested_;
  std::vector<std::string> methods_requested_;
  RequestDeferrer* request_deferrer_;  // NOT OWNED.
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(TestInMemoryProtocolHandler);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_TESTING_TEST_IN_MEMORY_PROTOCOL_HANDLER_H_
