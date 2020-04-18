// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/mock_resource_loader.h"

#include <memory>

#include "base/memory/ref_counted.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/resource_handler.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_request_status.h"
#include "services/network/public/cpp/resource_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class MockResourceLoader::TestResourceController : public ResourceController {
 public:
  explicit TestResourceController(base::WeakPtr<MockResourceLoader> mock_loader)
      : mock_loader_(mock_loader) {}
  ~TestResourceController() override {}

  void Resume() override { mock_loader_->OnResume(); }

  void Cancel() override { CancelWithError(net::ERR_ABORTED); }

  void CancelWithError(int error_code) override {
    mock_loader_->OnCancel(error_code);
  }

  base::WeakPtr<MockResourceLoader> mock_loader_;
};

MockResourceLoader::MockResourceLoader(ResourceHandler* resource_handler)
    : resource_handler_(resource_handler), weak_factory_(this) {
  resource_handler_->SetDelegate(this);
}

MockResourceLoader::~MockResourceLoader() {}

MockResourceLoader::Status MockResourceLoader::OnWillStart(const GURL& url) {
  EXPECT_FALSE(weak_factory_.HasWeakPtrs());
  EXPECT_EQ(Status::IDLE, status_);

  status_ = Status::CALLING_HANDLER;
  resource_handler_->OnWillStart(url, std::make_unique<TestResourceController>(
                                          weak_factory_.GetWeakPtr()));
  if (status_ == Status::CALLING_HANDLER)
    status_ = Status::CALLBACK_PENDING;
  return status_;
}

MockResourceLoader::Status MockResourceLoader::OnRequestRedirected(
    const net::RedirectInfo& redirect_info,
    scoped_refptr<network::ResourceResponse> response) {
  EXPECT_FALSE(weak_factory_.HasWeakPtrs());
  EXPECT_EQ(Status::IDLE, status_);

  status_ = Status::CALLING_HANDLER;
  // Note that |this| does not hold onto |response|, to match ResourceLoader's
  // behavior. If |resource_handler_| wants to use |response| asynchronously, it
  // needs to hold onto its own pointer to it.
  resource_handler_->OnRequestRedirected(
      redirect_info, response.get(),
      std::make_unique<TestResourceController>(weak_factory_.GetWeakPtr()));
  if (status_ == Status::CALLING_HANDLER)
    status_ = Status::CALLBACK_PENDING;
  return status_;
}

MockResourceLoader::Status MockResourceLoader::OnResponseStarted(
    scoped_refptr<network::ResourceResponse> response) {
  EXPECT_FALSE(weak_factory_.HasWeakPtrs());
  EXPECT_EQ(Status::IDLE, status_);

  status_ = Status::CALLING_HANDLER;
  // Note that |this| does not hold onto |response|, to match ResourceLoader's
  // behavior. If |resource_handler_| wants to use |response| asynchronously, it
  // needs to hold onto its own pointer to it.
  resource_handler_->OnResponseStarted(
      response.get(),
      std::make_unique<TestResourceController>(weak_factory_.GetWeakPtr()));
  if (status_ == Status::CALLING_HANDLER)
    status_ = Status::CALLBACK_PENDING;
  return status_;
}

MockResourceLoader::Status MockResourceLoader::OnWillRead() {
  EXPECT_FALSE(weak_factory_.HasWeakPtrs());
  EXPECT_EQ(Status::IDLE, status_);

  status_ = Status::CALLING_HANDLER;
  waiting_on_buffer_ = true;
  resource_handler_->OnWillRead(
      &io_buffer_, &io_buffer_size_,
      std::make_unique<TestResourceController>(weak_factory_.GetWeakPtr()));
  if (status_ == Status::CALLING_HANDLER) {
    // Shouldn't update  |io_buffer_| or |io_buffer_size_| yet if Resume()
    // hasn't yet been called.
    EXPECT_FALSE(io_buffer_);
    EXPECT_EQ(0, io_buffer_size_);

    status_ = Status::CALLBACK_PENDING;
  }

  return status_;
};

MockResourceLoader::Status MockResourceLoader::OnReadCompleted(
    base::StringPiece bytes) {
  EXPECT_FALSE(weak_factory_.HasWeakPtrs());
  EXPECT_EQ(Status::IDLE, status_);
  EXPECT_LE(bytes.size(), static_cast<size_t>(io_buffer_size_));

  status_ = Status::CALLING_HANDLER;
  std::copy(bytes.begin(), bytes.end(), io_buffer_->data());
  io_buffer_ = nullptr;
  io_buffer_size_ = 0;
  resource_handler_->OnReadCompleted(
      bytes.size(),
      std::make_unique<TestResourceController>(weak_factory_.GetWeakPtr()));
  if (status_ == Status::CALLING_HANDLER)
    status_ = Status::CALLBACK_PENDING;
  return status_;
}

MockResourceLoader::Status MockResourceLoader::OnResponseCompleted(
    const net::URLRequestStatus& status) {
  EXPECT_FALSE(weak_factory_.HasWeakPtrs());
  // This should only happen while the ResourceLoader is idle or the request was
  // canceled.
  EXPECT_TRUE(status_ == Status::IDLE ||
              (!status.is_success() && status_ == Status::CANCELED &&
               error_code_ == status.error()));

  io_buffer_ = nullptr;
  io_buffer_size_ = 0;
  status_ = Status::CALLING_HANDLER;
  resource_handler_->OnResponseCompleted(
      status,
      std::make_unique<TestResourceController>(weak_factory_.GetWeakPtr()));
  if (status_ == Status::CALLING_HANDLER)
    status_ = Status::CALLBACK_PENDING;
  EXPECT_NE(Status::CANCELED, status_);
  return status_;
}

MockResourceLoader::Status
MockResourceLoader::OnResponseCompletedFromExternalOutOfBandCancel(
    const net::URLRequestStatus& url_request_status) {
  // This can happen at any point, except from a recursive call from
  // ResourceHandler.
  EXPECT_NE(Status::CALLING_HANDLER, status_);

  waiting_on_buffer_ = false;
  io_buffer_ = nullptr;
  io_buffer_size_ = 0;
  status_ = Status::CALLING_HANDLER;

  resource_handler_->OnResponseCompleted(
      url_request_status,
      std::make_unique<TestResourceController>(weak_factory_.GetWeakPtr()));
  if (status_ == Status::CALLING_HANDLER)
    status_ = Status::CALLBACK_PENDING;
  EXPECT_NE(Status::CANCELED, status_);
  return status_;
}

void MockResourceLoader::WaitUntilIdleOrCanceled() {
  if (status_ == Status::IDLE || status_ == Status::CANCELED)
    return;
  EXPECT_FALSE(canceled_or_idle_run_loop_);
  canceled_or_idle_run_loop_.reset(new base::RunLoop());
  canceled_or_idle_run_loop_->Run();
  canceled_or_idle_run_loop_.reset();
  EXPECT_TRUE(status_ == Status::IDLE || status_ == Status::CANCELED);
}

void MockResourceLoader::OutOfBandCancel(int error_code, bool tell_renderer) {
  // Shouldn't be called in-band.
  EXPECT_NE(Status::CALLING_HANDLER, status_);

  status_ = Status::CANCELED;
  canceled_out_of_band_ = true;

  // If OnWillRead was deferred, no longer waiting on a buffer.
  waiting_on_buffer_ = false;

  // To mimic real behavior, keep old error, in the case of double-cancel.
  if (error_code_ == net::OK)
    error_code_ = error_code;
}

void MockResourceLoader::OnCancel(int error_code) {
  // It's currently allowed to be canceled in-band after being cancelled
  // out-of-band, so do nothing, unless the status is no longer CANCELED, which
  // which case, OnResponseCompleted has already been called, and cancels aren't
  // expected then.
  // TODO(mmenke):  Make CancelOutOfBand synchronously destroy the
  // ResourceLoader.
  if (canceled_out_of_band_ && status_ == Status::CANCELED)
    return;

  // Shouldn't update |io_buffer_| or |io_buffer_size_| on cancel.
  if (waiting_on_buffer_) {
    EXPECT_FALSE(io_buffer_);
    EXPECT_EQ(0, io_buffer_size_);
    waiting_on_buffer_ = false;
  }

  EXPECT_LT(error_code, 0);
  EXPECT_TRUE(status_ == Status::CALLBACK_PENDING ||
              status_ == Status::CALLING_HANDLER);

  status_ = Status::CANCELED;
  error_code_ = error_code;
  if (canceled_or_idle_run_loop_)
    canceled_or_idle_run_loop_->Quit();
}

void MockResourceLoader::OnResume() {
  if (waiting_on_buffer_) {
    EXPECT_TRUE(io_buffer_);
    EXPECT_LT(0, io_buffer_size_);

    waiting_on_buffer_ = false;
  }

  // Shouldn't update |io_buffer_| or |io_buffer_size_| on cancel.
  EXPECT_TRUE(status_ == Status::CALLBACK_PENDING ||
              status_ == Status::CALLING_HANDLER);

  status_ = Status::IDLE;
  if (canceled_or_idle_run_loop_)
    canceled_or_idle_run_loop_->Quit();
}

}  // namespace content
