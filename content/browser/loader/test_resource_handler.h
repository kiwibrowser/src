// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_TEST_RESOURCE_HANDLER_H_
#define CONTENT_BROWSER_LOADER_TEST_RESOURCE_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "content/browser/loader/resource_handler.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace net {
class URLRequestStatus;
}

namespace network {
struct ResourceResponse;
}

namespace content {

class ResourceController;
class ResourceHandler;

// A test version of a ResourceHandler. It returns a configurable buffer in
// response to OnWillStart. It records what ResourceHandler methods are called,
// and verifies that they are called in the correct order. It can optionally
// defer or fail the request at any stage, and record the response body and
// final status it sees. Redirects currently not supported.
class TestResourceHandler : public ResourceHandler {
 public:
  // If non-null, |request_status| will be updated when the response is complete
  // with the final status of the request received by the handler and |body|
  // will be updated on each OnReadCompleted call.
  TestResourceHandler(net::URLRequestStatus* request_status, std::string* body);
  TestResourceHandler();
  ~TestResourceHandler() override;

  // ResourceHandler implementation:
  void OnRequestRedirected(
      const net::RedirectInfo& redirect_info,
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnResponseStarted(
      network::ResourceResponse* response,
      std::unique_ptr<ResourceController> controller) override;
  void OnWillStart(const GURL& url,
                   std::unique_ptr<ResourceController> controller) override;
  void OnWillRead(scoped_refptr<net::IOBuffer>* buf,
                  int* buf_size,
                  std::unique_ptr<ResourceController> controller) override;
  void OnReadCompleted(int bytes_read,
                       std::unique_ptr<ResourceController> controller) override;
  void OnResponseCompleted(
      const net::URLRequestStatus& status,
      std::unique_ptr<ResourceController> controller) override;
  void OnDataDownloaded(int bytes_downloaded) override;

  void Resume();
  void CancelWithError(net::Error error_code);

  // Sets the size of the read buffer returned by OnWillRead. Releases reference
  // to previous read buffer. Default size is 2048 bytes.
  void SetBufferSize(int buffer_size);

  scoped_refptr<net::IOBuffer> buffer() const { return buffer_; }

  // Sets the result returned by each method. All default to returning true.
  void set_on_will_start_result(bool on_will_start_result) {
    on_will_start_result_ = on_will_start_result;
  }
  void set_on_request_redirected_result(bool on_request_redirected_result) {
    on_request_redirected_result_ = on_request_redirected_result;
  }
  void set_on_response_started_result(bool on_response_started_result) {
    on_response_started_result_ = on_response_started_result;
  }
  void set_on_will_read_result(bool on_will_read_result) {
    on_will_read_result_ = on_will_read_result;
  }
  void set_on_read_completed_result(bool on_read_completed_result) {
    on_read_completed_result_ = on_read_completed_result;
  }
  void set_on_read_eof_result(bool on_read_eof_result) {
    on_read_eof_result_ = on_read_eof_result;
  }

  // Cause |defer| to be set to true when the specified method is invoked. The
  // test itself is responsible for resuming the request after deferral.

  void set_defer_on_will_start(bool defer_on_will_start) {
    defer_on_will_start_ = defer_on_will_start;
  }
  void set_defer_on_request_redirected(bool defer_on_request_redirected) {
    defer_on_request_redirected_ = defer_on_request_redirected;
  }
  void set_defer_on_response_started(bool defer_on_response_started) {
    defer_on_response_started_ = defer_on_response_started;
  }
  // Only the next OnWillRead call will set |defer| to true.
  void set_defer_on_will_read(bool defer_on_will_read) {
    defer_on_will_read_ = defer_on_will_read;
  }
  // Only the next OnReadCompleted call will set |defer| to true.
  void set_defer_on_read_completed(bool defer_on_read_completed) {
    defer_on_read_completed_ = defer_on_read_completed;
  }
  // The final-byte read will set |defer| to true.
  void set_defer_on_read_eof(bool defer_on_read_eof) {
    defer_on_read_eof_ = defer_on_read_eof;
  }
  void set_defer_on_response_completed(bool defer_on_response_completed) {
    defer_on_response_completed_ = defer_on_response_completed;
  }

  // Set if OnDataDownloaded calls are expected instead of
  // OnWillRead/OnReadCompleted.
  void set_expect_on_data_downloaded(bool expect_on_data_downloaded) {
    expect_on_data_downloaded_ = expect_on_data_downloaded;
  }

  // Sets whether to expect a final 0-byte read on success. Defaults to true.
  void set_expect_eof_read(bool expect_eof_read) {
    expect_eof_read_ = expect_eof_read;
  }

  // Return the number of times the corresponding method was invoked.

  int on_will_start_called() const { return on_will_start_called_; }
  int on_request_redirected_called() const {
    return on_request_redirected_called_;
  }
  int on_response_started_called() const { return on_response_started_called_; }
  int on_will_read_called() const { return on_will_read_called_; }
  int on_read_completed_called() const { return on_read_completed_called_; }
  int on_read_eof_called() const { return on_read_eof_called_; }
  int on_response_completed_called() const {
    return on_response_completed_called_;
  }

  // URL passed to OnResponseStarted, if it was called.
  const GURL& start_url() const { return start_url_; }

  network::ResourceResponse* resource_response() {
    return resource_response_.get();
  };

  int total_bytes_downloaded() const { return total_bytes_downloaded_; }

  const std::string& body() const { return body_; }
  net::URLRequestStatus final_status() const { return final_status_; }

  // Returns the current number of |this|'s methods on the callstack.
  int call_depth() const { return call_depth_; }

  // Spins the message loop until the request is deferred.  Using this is
  // optional, but if used, must use it exclusively to wait for the request. If
  // the request was deferred and then resumed/canceled without calling this
  // method, behavior is undefined.
  void WaitUntilDeferred();

  void WaitUntilResponseStarted();
  void WaitUntilResponseComplete();

  // Returns a weak pointer to |this|.  Allows testing object lifetime.
  base::WeakPtr<TestResourceHandler> GetWeakPtr();

 private:
  // TODO(mmenke):  Remove these, in favor of final_status_ and body_.
  net::URLRequestStatus* request_status_ptr_;
  std::string* body_ptr_;

  scoped_refptr<net::IOBuffer> buffer_;
  size_t buffer_size_;

  bool on_will_start_result_ = true;
  bool on_request_redirected_result_ = true;
  bool on_response_started_result_ = true;
  bool on_will_read_result_ = true;
  bool on_read_completed_result_ = true;
  bool on_read_eof_result_ = true;

  bool defer_on_will_start_ = false;
  bool defer_on_request_redirected_ = false;
  bool defer_on_response_started_ = false;
  bool defer_on_will_read_ = false;
  bool defer_on_read_completed_ = false;
  bool defer_on_read_eof_ = false;
  bool defer_on_response_completed_ = false;

  bool expect_on_data_downloaded_ = false;

  bool expect_eof_read_ = true;

  int on_will_start_called_ = 0;
  int on_request_redirected_called_ = 0;
  int on_response_started_called_ = 0;
  int on_will_read_called_ = 0;
  int on_read_completed_called_ = 0;
  int on_read_eof_called_ = 0;
  int on_response_completed_called_ = 0;

  GURL start_url_;
  scoped_refptr<network::ResourceResponse> resource_response_;
  int total_bytes_downloaded_ = 0;
  std::string body_;
  net::URLRequestStatus final_status_ =
      net::URLRequestStatus::FromError(net::ERR_UNEXPECTED);
  bool canceled_ = false;

  // Pointers to the parent's read buffer and size. Only non-NULL during
  // OnWillRead call, until cancellation or resumption.
  scoped_refptr<net::IOBuffer>* parent_read_buffer_ = nullptr;
  int* parent_read_buffer_size_ = nullptr;

  // Tracks recursive calls, which aren't allowed.
  int call_depth_ = 0;

  std::unique_ptr<base::RunLoop> deferred_run_loop_;

  base::RunLoop response_started_run_loop_;

  base::RunLoop response_complete_run_loop_;

  base::WeakPtrFactory<TestResourceHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestResourceHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_TEST_RESOURCE_HANDLER_H_
