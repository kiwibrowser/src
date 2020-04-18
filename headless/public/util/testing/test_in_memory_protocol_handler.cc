// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/testing/test_in_memory_protocol_handler.h"

#include <memory>

#include "headless/public/util/expedited_dispatcher.h"
#include "headless/public/util/generic_url_request_job.h"
#include "headless/public/util/url_fetcher.h"

namespace headless {

class TestInMemoryProtocolHandler::MockURLFetcher : public URLFetcher {
 public:
  explicit MockURLFetcher(TestInMemoryProtocolHandler* protocol_handler)
      : protocol_handler_(protocol_handler) {}
  ~MockURLFetcher() override = default;

  // URLFetcher implementation:
  void StartFetch(const Request* request,
                  ResultListener* result_listener) override {
    GURL url = request->GetURL();
    const std::string& method = request->GetMethod();
    if (method == "POST" || method == "PUT") {
      request->GetPostData();
    } else if (method == "GET") {
      // Do nothing.
    } else {
      DCHECK(false) << "Method " << method << " is not supported. Probably.";
    }

    std::string devtools_frame_id = request->GetDevToolsFrameId();
    // Note |devtools_frame_id| can sometimes be empty if called during context
    // shutdown. This isn't a big deal because code should avoid performing net
    // operations during shutdown.
    protocol_handler_->methods_requested_.push_back(method);
    protocol_handler_->RegisterUrl(url.spec(), devtools_frame_id);

    if (protocol_handler_->request_deferrer()) {
      protocol_handler_->request_deferrer()->OnRequest(
          url, base::Bind(&MockURLFetcher::FinishFetch, base::Unretained(this),
                          result_listener, url));
    } else {
      FinishFetch(result_listener, url);
    }
  }

  void FinishFetch(ResultListener* result_listener, GURL url) {
    const TestInMemoryProtocolHandler::Response* response =
        protocol_handler_->GetResponse(url.spec());
    if (response) {
      net::LoadTimingInfo load_timing_info;
      load_timing_info.receive_headers_end = base::TimeTicks::Now();
      result_listener->OnFetchCompleteExtractHeaders(
          url, response->data.c_str(), response->data.size(),
          response->metadata, load_timing_info, 0);
    } else {
      result_listener->OnFetchStartError(net::ERR_FILE_NOT_FOUND);
    }
  }

 private:
  TestInMemoryProtocolHandler* protocol_handler_;  // NOT OWNED.

  DISALLOW_COPY_AND_ASSIGN(MockURLFetcher);
};

class TestInMemoryProtocolHandler::TestDelegate
    : public GenericURLRequestJob::Delegate {
 public:
  TestDelegate() = default;
  ~TestDelegate() override = default;

  // GenericURLRequestJob::Delegate implementation:
  void OnResourceLoadFailed(const Request* request, net::Error error) override {
  }

  void OnResourceLoadComplete(
      const Request* request,
      const GURL& final_url,
      scoped_refptr<net::HttpResponseHeaders> response_headers,
      const char* body,
      size_t body_size) override {}

 private:
  scoped_refptr<base::SingleThreadTaskRunner> ui_thread_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(TestDelegate);
};

TestInMemoryProtocolHandler::TestInMemoryProtocolHandler(
    scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner,
    RequestDeferrer* request_deferrer)
    : test_delegate_(new TestDelegate()),
      dispatcher_(new ExpeditedDispatcher(io_thread_task_runner)),
      headless_browser_context_(nullptr),
      request_deferrer_(request_deferrer),
      io_thread_task_runner_(io_thread_task_runner) {}

TestInMemoryProtocolHandler::~TestInMemoryProtocolHandler() = default;

void TestInMemoryProtocolHandler::SetHeadlessBrowserContext(
    HeadlessBrowserContext* headless_browser_context) {
  headless_browser_context_ = headless_browser_context;
}

void TestInMemoryProtocolHandler::InsertResponse(const std::string& url,
                                                 const Response& response) {
  response_map_[url].reset(new Response(response));
}

void TestInMemoryProtocolHandler::SetResponseMetadata(
    const std::string& url,
    scoped_refptr<net::IOBufferWithSize> metadata) {
  response_map_[url]->metadata = metadata;
}

const TestInMemoryProtocolHandler::Response*
TestInMemoryProtocolHandler::GetResponse(const std::string& url) const {
  const auto find_it = response_map_.find(url);
  if (find_it == response_map_.end())
    return nullptr;
  return find_it->second.get();
}

net::URLRequestJob* TestInMemoryProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return new GenericURLRequestJob(
      request, network_delegate, dispatcher_.get(),
      std::make_unique<MockURLFetcher>(
          const_cast<TestInMemoryProtocolHandler*>(this)),
      test_delegate_.get(), headless_browser_context_);
}

}  // namespace headless
