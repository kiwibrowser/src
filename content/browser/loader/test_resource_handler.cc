// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/test_resource_handler.h"

#include "base/logging.h"
#include "content/browser/loader/resource_controller.h"
#include "services/network/public/cpp/resource_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class ScopedCallDepthTracker {
 public:
  explicit ScopedCallDepthTracker(int* call_depth) : call_depth_(call_depth) {
    EXPECT_EQ(0, *call_depth_);
    (*call_depth_)++;
  }

  ~ScopedCallDepthTracker() {
    EXPECT_EQ(1, *call_depth_);
    (*call_depth_)--;
  }

 private:
  int* const call_depth_;

  DISALLOW_COPY_AND_ASSIGN(ScopedCallDepthTracker);
};

}  // namespace

TestResourceHandler::TestResourceHandler(net::URLRequestStatus* request_status,
                                         std::string* body)
    : ResourceHandler(nullptr),
      request_status_ptr_(request_status),
      body_ptr_(body),
      deferred_run_loop_(new base::RunLoop()),
      weak_ptr_factory_(this) {
  SetBufferSize(2048);
}

TestResourceHandler::TestResourceHandler()
    : TestResourceHandler(nullptr, nullptr) {}

TestResourceHandler::~TestResourceHandler() {}

void TestResourceHandler::OnRequestRedirected(
    const net::RedirectInfo& redirect_info,
    network::ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  EXPECT_FALSE(canceled_);
  EXPECT_EQ(1, on_will_start_called_);
  EXPECT_EQ(0, on_response_started_called_);
  EXPECT_EQ(0, on_response_completed_called_);
  ScopedCallDepthTracker call_depth_tracker(&call_depth_);

  ++on_request_redirected_called_;

  if (!on_request_redirected_result_) {
    canceled_ = true;
    controller->Cancel();
    return;
  }

  if (defer_on_request_redirected_) {
    defer_on_request_redirected_ = false;
    HoldController(std::move(controller));
    deferred_run_loop_->Quit();
    return;
  }

  controller->Resume();
}

void TestResourceHandler::OnResponseStarted(
    network::ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  EXPECT_FALSE(canceled_);
  EXPECT_EQ(1, on_will_start_called_);
  EXPECT_EQ(0, on_response_started_called_);
  EXPECT_EQ(0, on_response_completed_called_);
  ScopedCallDepthTracker call_depth_tracker(&call_depth_);

  ++on_response_started_called_;

  EXPECT_FALSE(resource_response_);
  resource_response_ = response;

  response_started_run_loop_.Quit();

  if (!on_response_started_result_) {
    canceled_ = true;
    controller->Cancel();
    return;
  }

  if (!on_request_redirected_result_) {
    controller->Cancel();
    return;
  }

  if (defer_on_response_started_) {
    defer_on_response_started_ = false;
    HoldController(std::move(controller));
    deferred_run_loop_->Quit();
    return;
  }

  controller->Resume();
}

void TestResourceHandler::OnWillStart(
    const GURL& url,
    std::unique_ptr<ResourceController> controller) {
  EXPECT_FALSE(canceled_);
  EXPECT_EQ(0, on_response_started_called_);
  EXPECT_EQ(0, on_will_start_called_);
  EXPECT_EQ(0, on_response_completed_called_);
  ScopedCallDepthTracker call_depth_tracker(&call_depth_);

  ++on_will_start_called_;

  start_url_ = url;

  if (!on_will_start_result_) {
    canceled_ = true;
    controller->Cancel();
    return;
  }

  if (defer_on_will_start_) {
    defer_on_will_start_ = false;
    HoldController(std::move(controller));
    deferred_run_loop_->Quit();
    return;
  }

  controller->Resume();
}

void TestResourceHandler::OnWillRead(
    scoped_refptr<net::IOBuffer>* buf,
    int* buf_size,
    std::unique_ptr<ResourceController> controller) {
  EXPECT_FALSE(canceled_);
  EXPECT_FALSE(expect_on_data_downloaded_);
  EXPECT_EQ(0, on_response_completed_called_);
  // Only create a ScopedCallDepthTracker if not called re-entrantly, as
  // OnWillRead may be called synchronously in response to a Resume(), but
  // nothing may be called synchronously in response to the OnWillRead call.
  std::unique_ptr<ScopedCallDepthTracker> call_depth_tracker;
  if (call_depth_ == 0)
    call_depth_tracker = std::make_unique<ScopedCallDepthTracker>(&call_depth_);

  ++on_will_read_called_;

  if (!on_will_read_result_) {
    canceled_ = true;
    controller->Cancel();
    return;
  }

  if (defer_on_will_read_) {
    parent_read_buffer_ = buf;
    parent_read_buffer_size_ = buf_size;
    defer_on_will_read_ = false;
    HoldController(std::move(controller));
    deferred_run_loop_->Quit();
    return;
  }

  *buf = buffer_;
  *buf_size = buffer_size_;
  controller->Resume();
}

void TestResourceHandler::OnReadCompleted(
    int bytes_read,
    std::unique_ptr<ResourceController> controller) {
  EXPECT_FALSE(canceled_);
  EXPECT_FALSE(expect_on_data_downloaded_);
  EXPECT_EQ(1, on_will_start_called_);
  EXPECT_EQ(1, on_response_started_called_);
  EXPECT_EQ(0, on_response_completed_called_);
  EXPECT_EQ(0, on_read_eof_called_);
  ScopedCallDepthTracker call_depth_tracker(&call_depth_);

  ++on_read_completed_called_;
  EXPECT_EQ(on_read_completed_called_, on_will_read_called_);
  if (bytes_read == 0)
    ++on_read_eof_called_;

  EXPECT_LE(static_cast<size_t>(bytes_read), buffer_size_);
  if (body_ptr_)
    body_ptr_->append(buffer_->data(), bytes_read);
  body_.append(buffer_->data(), bytes_read);

  if (!on_read_completed_result_ || (!on_read_eof_result_ && bytes_read == 0)) {
    canceled_ = true;
    controller->Cancel();
    return;
  }

  if (defer_on_read_completed_ || (bytes_read == 0 && defer_on_read_eof_)) {
    defer_on_read_completed_ = false;
    HoldController(std::move(controller));
    deferred_run_loop_->Quit();
    return;
  }

  controller->Resume();
}

void TestResourceHandler::OnResponseCompleted(
    const net::URLRequestStatus& status,
    std::unique_ptr<ResourceController> controller) {
  ScopedCallDepthTracker call_depth_tracker(&call_depth_);

  // These may be non-NULL if there was an out-of-band cancel.
  parent_read_buffer_ = nullptr;
  parent_read_buffer_size_ = nullptr;

  EXPECT_EQ(0, on_response_completed_called_);
  if (status.is_success() && !expect_on_data_downloaded_ && expect_eof_read_)
    EXPECT_EQ(1, on_read_eof_called_);

  ++on_response_completed_called_;

  if (request_status_ptr_)
    *request_status_ptr_ = status;
  final_status_ = status;

  // Consider response completed.  Even if deferring, the TestResourceHandler
  // won't be called again.
  response_complete_run_loop_.Quit();

  if (defer_on_response_completed_) {
    defer_on_response_completed_ = false;
    HoldController(std::move(controller));
    deferred_run_loop_->Quit();
    return;
  }

  controller->Resume();
}

void TestResourceHandler::OnDataDownloaded(int bytes_downloaded) {
  EXPECT_TRUE(expect_on_data_downloaded_);
  EXPECT_EQ(1, on_will_start_called_);
  EXPECT_EQ(1, on_response_started_called_);
  EXPECT_EQ(0, on_response_completed_called_);

  total_bytes_downloaded_ += bytes_downloaded;
}

void TestResourceHandler::Resume() {
  ScopedCallDepthTracker call_depth_tracker(&call_depth_);

  if (parent_read_buffer_) {
    *parent_read_buffer_ = buffer_;
    *parent_read_buffer_size_ = buffer_size_;
    parent_read_buffer_ = nullptr;
    parent_read_buffer_size_ = nullptr;
    memset(buffer_->data(), '\0', buffer_size_);
  }

  ResourceHandler::Resume();
}

void TestResourceHandler::CancelWithError(net::Error net_error) {
  ScopedCallDepthTracker call_depth_tracker(&call_depth_);
  canceled_ = true;

  // Don't want to populate these after a cancel.
  parent_read_buffer_ = nullptr;
  parent_read_buffer_size_ = nullptr;

  ResourceHandler::CancelWithError(net_error);
}

void TestResourceHandler::SetBufferSize(int buffer_size) {
  buffer_ = new net::IOBuffer(buffer_size);
  buffer_size_ = buffer_size;
  memset(buffer_->data(), '\0', buffer_size);
}

void TestResourceHandler::WaitUntilDeferred() {
  deferred_run_loop_->Run();
  deferred_run_loop_.reset(new base::RunLoop());
}

void TestResourceHandler::WaitUntilResponseStarted() {
  response_started_run_loop_.Run();
}

void TestResourceHandler::WaitUntilResponseComplete() {
  response_complete_run_loop_.Run();
}

base::WeakPtr<TestResourceHandler> TestResourceHandler::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace content
