// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/detachable_resource_handler.h"

#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/browser/loader/mock_resource_loader.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/test_resource_handler.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/public/cpp/resource_response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {

// Full response body.
const char kResponseBody[] = "Nifty response body.";
// Two separate reads allow for testing cancellation in the middle of one read,
// and between reads.
const char kFirstBodyRead[] = "Nifty";
const char kSecondBodyRead[] = " response body.";

enum class DetachPhase {
  DETACHED_FROM_CREATION,
  ON_WILL_START,
  REQUEST_REDIRECTED,
  ON_RESPONSE_STARTED,
  FIRST_ON_WILL_READ,
  FIRST_ON_READ_COMPLETED,
  SECOND_ON_WILL_READ,
  SECOND_ON_READ_COMPLETED,
  ON_READ_EOF,
  ON_RESPONSE_COMPLETED,
  NEVER_DETACH,
};

class DetachableResourceHandlerTest
    : public testing::TestWithParam<DetachPhase> {
 public:
  DetachableResourceHandlerTest()
      : request_(context_.CreateRequest(GURL("http://foo/"),
                                        net::DEFAULT_PRIORITY,
                                        nullptr,
                                        TRAFFIC_ANNOTATION_FOR_TESTS)) {
    ResourceRequestInfo::AllocateForTesting(request_.get(),
                                            RESOURCE_TYPE_MAIN_FRAME,
                                            nullptr,       // context
                                            0,             // render_process_id
                                            0,             // render_view_id
                                            0,             // render_frame_id
                                            true,          // is_main_frame
                                            true,          // allow_download
                                            true,          // is_async
                                            PREVIEWS_OFF,  // previews_state
                                            nullptr);      // navigation_ui_data

    std::unique_ptr<TestResourceHandler> test_handler;
    if (GetParam() != DetachPhase::DETACHED_FROM_CREATION) {
      test_handler = std::make_unique<TestResourceHandler>();
      test_handler_ = test_handler->GetWeakPtr();
    }
    // TODO(mmenke):  This file currently has no timeout tests. Should it?
    detachable_handler_ = std::make_unique<DetachableResourceHandler>(
        request_.get(), base::TimeDelta::FromMinutes(30),
        std::move(test_handler));
    mock_loader_ =
        std::make_unique<MockResourceLoader>(detachable_handler_.get());
  }

  // If the DetachableResourceHandler is supposed to detach the next handler at
  // |phase|, attempts to detach the request.
  void MaybeSyncDetachAtPhase(DetachPhase phase) {
    if (GetParam() == phase) {
      detachable_handler_->Detach();
      EXPECT_FALSE(test_handler_);
    }
  }

  // Returns true if the DetachableResourceHandler should have detached the next
  // handler at or before the specified phase.  Also checks that |test_handler_|
  // is nullptr iff the request should have been detached by the specified
  // phase.
  bool WasDetachedBy(DetachPhase phase) {
    if (GetParam() <= phase) {
      EXPECT_FALSE(test_handler_);
      return true;
    }
    EXPECT_TRUE(test_handler_);
    return false;
  }

  // If the DetachableResourceHandler is supposed to detach the next handler at
  // |phase|, attempts to detach the request. Expected to be called in sync
  // tests after the specified phase has started. Performs additional sanity
  // checks based on that assumption.
  void MaybeAsyncDetachAt(DetachPhase phase) {
    if (GetParam() < phase) {
      EXPECT_FALSE(test_handler_);
      EXPECT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
      return;
    }

    EXPECT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
              mock_loader_->status());

    if (GetParam() == phase) {
      detachable_handler_->Detach();
      EXPECT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
      EXPECT_FALSE(test_handler_);
      return;
    }

    test_handler_->Resume();
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  net::TestURLRequestContext context_;
  std::unique_ptr<net::URLRequest> request_;

  base::WeakPtr<TestResourceHandler> test_handler_;

  std::unique_ptr<DetachableResourceHandler> detachable_handler_;
  std::unique_ptr<MockResourceLoader> mock_loader_;
};

// Tests where ResourceHandler completes synchronously. Handler is detached
// just before the phase indicated by the DetachPhase parameter.
TEST_P(DetachableResourceHandlerTest, Sync) {
  MaybeSyncDetachAtPhase(DetachPhase::ON_WILL_START);
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  if (!WasDetachedBy(DetachPhase::ON_WILL_START)) {
    EXPECT_EQ(1, test_handler_->on_will_start_called());
    EXPECT_EQ(0, test_handler_->on_request_redirected_called());
  }

  MaybeSyncDetachAtPhase(DetachPhase::REQUEST_REDIRECTED);
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnRequestRedirected(
                net::RedirectInfo(),
                base::MakeRefCounted<network::ResourceResponse>()));
  if (!WasDetachedBy(DetachPhase::REQUEST_REDIRECTED)) {
    EXPECT_EQ(1, test_handler_->on_request_redirected_called());
    EXPECT_EQ(0, test_handler_->on_response_started_called());
  }

  MaybeSyncDetachAtPhase(DetachPhase::ON_RESPONSE_STARTED);
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));
  if (!WasDetachedBy(DetachPhase::ON_RESPONSE_STARTED)) {
    EXPECT_EQ(1, test_handler_->on_request_redirected_called());
    EXPECT_EQ(1, test_handler_->on_response_started_called());
    EXPECT_EQ(0, test_handler_->on_will_read_called());
  }

  MaybeSyncDetachAtPhase(DetachPhase::FIRST_ON_WILL_READ);
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  if (!WasDetachedBy(DetachPhase::FIRST_ON_WILL_READ)) {
    EXPECT_EQ(1, test_handler_->on_will_read_called());
    EXPECT_EQ(0, test_handler_->on_read_completed_called());
  }

  MaybeSyncDetachAtPhase(DetachPhase::FIRST_ON_READ_COMPLETED);
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(kFirstBodyRead));
  if (!WasDetachedBy(DetachPhase::FIRST_ON_READ_COMPLETED)) {
    EXPECT_EQ(1, test_handler_->on_read_completed_called());
    EXPECT_EQ(kFirstBodyRead, test_handler_->body());
  }

  MaybeSyncDetachAtPhase(DetachPhase::SECOND_ON_WILL_READ);
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  if (!WasDetachedBy(DetachPhase::SECOND_ON_WILL_READ)) {
    EXPECT_EQ(2, test_handler_->on_will_read_called());
    EXPECT_EQ(1, test_handler_->on_read_completed_called());
  }

  MaybeSyncDetachAtPhase(DetachPhase::SECOND_ON_READ_COMPLETED);
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(kSecondBodyRead));
  if (!WasDetachedBy(DetachPhase::SECOND_ON_READ_COMPLETED)) {
    EXPECT_EQ(2, test_handler_->on_will_read_called());
    EXPECT_EQ(2, test_handler_->on_read_completed_called());
    EXPECT_EQ(kResponseBody, test_handler_->body());
  }

  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  if (!WasDetachedBy(DetachPhase::SECOND_ON_READ_COMPLETED)) {
    EXPECT_EQ(3, test_handler_->on_will_read_called());
    EXPECT_EQ(2, test_handler_->on_read_completed_called());
    EXPECT_EQ(0, test_handler_->on_response_completed_called());
  }

  MaybeSyncDetachAtPhase(DetachPhase::ON_READ_EOF);
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(""));
  if (!WasDetachedBy(DetachPhase::ON_READ_EOF)) {
    EXPECT_EQ(3, test_handler_->on_read_completed_called());
    EXPECT_EQ(1, test_handler_->on_read_eof_called());
    EXPECT_EQ(0, test_handler_->on_response_completed_called());
  }

  MaybeSyncDetachAtPhase(DetachPhase::ON_RESPONSE_COMPLETED);
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseCompleted(
                net::URLRequestStatus::FromError(net::OK)));
  if (!WasDetachedBy(DetachPhase::ON_RESPONSE_COMPLETED)) {
    EXPECT_EQ(1, test_handler_->on_response_completed_called());
    EXPECT_EQ(kResponseBody, test_handler_->body());
  }
}

// Tests where ResourceHandler completes asynchronously. Handler is detached
// during the phase indicated by the DetachPhase parameter. Async cases where
// the handler is detached between phases are similar enough to the sync tests
// that they wouldn't provide meaningfully better test coverage.
//
// Before the handler is detached, all calls complete asynchronously.
// Afterwards, they all complete synchronously.
TEST_P(DetachableResourceHandlerTest, Async) {
  if (GetParam() != DetachPhase::DETACHED_FROM_CREATION) {
    test_handler_->set_defer_on_will_start(true);
    test_handler_->set_defer_on_request_redirected(true);
    test_handler_->set_defer_on_response_started(true);
    test_handler_->set_defer_on_will_read(true);
    test_handler_->set_defer_on_read_completed(true);
    test_handler_->set_defer_on_read_eof(true);
    // Note:  Can't set |defer_on_response_completed|, since the
    // DetachableResourceHandler DCHECKs when the next handler tries to defer
    // the ERR_ABORTED message it sends downstream.
  }

  mock_loader_->OnWillStart(request_->url());
  if (test_handler_) {
    EXPECT_EQ(1, test_handler_->on_will_start_called());
    EXPECT_EQ(0, test_handler_->on_request_redirected_called());
  }
  MaybeAsyncDetachAt(DetachPhase::ON_WILL_START);

  mock_loader_->OnRequestRedirected(
      net::RedirectInfo(), base::MakeRefCounted<network::ResourceResponse>());
  if (test_handler_) {
    EXPECT_EQ(1, test_handler_->on_request_redirected_called());
    EXPECT_EQ(0, test_handler_->on_response_started_called());
  }
  MaybeAsyncDetachAt(DetachPhase::REQUEST_REDIRECTED);

  mock_loader_->OnResponseStarted(
      base::MakeRefCounted<network::ResourceResponse>());
  if (test_handler_) {
    EXPECT_EQ(1, test_handler_->on_request_redirected_called());
    EXPECT_EQ(1, test_handler_->on_response_started_called());
    EXPECT_EQ(0, test_handler_->on_will_read_called());
  }
  MaybeAsyncDetachAt(DetachPhase::ON_RESPONSE_STARTED);

  mock_loader_->OnWillRead();
  if (test_handler_) {
    EXPECT_EQ(1, test_handler_->on_will_read_called());
    EXPECT_EQ(0, test_handler_->on_read_completed_called());
  }
  MaybeAsyncDetachAt(DetachPhase::FIRST_ON_WILL_READ);

  mock_loader_->OnReadCompleted(kFirstBodyRead);
  if (test_handler_) {
    EXPECT_EQ(1, test_handler_->on_read_completed_called());
    EXPECT_EQ(kFirstBodyRead, test_handler_->body());
  }
  MaybeAsyncDetachAt(DetachPhase::FIRST_ON_READ_COMPLETED);

  if (test_handler_)
    test_handler_->set_defer_on_will_read(true);
  mock_loader_->OnWillRead();
  if (test_handler_) {
    EXPECT_EQ(2, test_handler_->on_will_read_called());
    EXPECT_EQ(1, test_handler_->on_read_completed_called());
  }
  MaybeAsyncDetachAt(DetachPhase::SECOND_ON_WILL_READ);

  if (test_handler_)
    test_handler_->set_defer_on_read_completed(true);
  mock_loader_->OnReadCompleted(kSecondBodyRead);
  if (test_handler_) {
    EXPECT_EQ(2, test_handler_->on_will_read_called());
    EXPECT_EQ(2, test_handler_->on_read_completed_called());
    EXPECT_EQ(kResponseBody, test_handler_->body());
  }
  MaybeAsyncDetachAt(DetachPhase::SECOND_ON_READ_COMPLETED);

  // Test doesn't check detaching on the third OnWillRead call.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  if (GetParam() > DetachPhase::SECOND_ON_READ_COMPLETED) {
    EXPECT_EQ(3, test_handler_->on_will_read_called());
    EXPECT_EQ(2, test_handler_->on_read_completed_called());
    EXPECT_EQ(0, test_handler_->on_response_completed_called());
  } else {
    EXPECT_FALSE(test_handler_);
  }

  if (test_handler_)
    test_handler_->set_defer_on_read_completed(true);
  mock_loader_->OnReadCompleted("");
  if (test_handler_) {
    EXPECT_EQ(3, test_handler_->on_read_completed_called());
    EXPECT_EQ(1, test_handler_->on_read_eof_called());
    EXPECT_EQ(0, test_handler_->on_response_completed_called());
  }
  MaybeAsyncDetachAt(DetachPhase::ON_READ_EOF);

  if (test_handler_)
    test_handler_->set_defer_on_response_completed(true);
  mock_loader_->OnResponseCompleted(net::URLRequestStatus::FromError(net::OK));
  if (test_handler_) {
    EXPECT_EQ(1, test_handler_->on_response_completed_called());
    EXPECT_EQ(kResponseBody, test_handler_->body());
  }
  MaybeAsyncDetachAt(DetachPhase::ON_RESPONSE_COMPLETED);
}

INSTANTIATE_TEST_CASE_P(/* No prefix needed*/,
                        DetachableResourceHandlerTest,
                        testing::Values(DetachPhase::DETACHED_FROM_CREATION,
                                        DetachPhase::ON_WILL_START,
                                        DetachPhase::REQUEST_REDIRECTED,
                                        DetachPhase::ON_RESPONSE_STARTED,
                                        DetachPhase::FIRST_ON_WILL_READ,
                                        DetachPhase::FIRST_ON_READ_COMPLETED,
                                        DetachPhase::SECOND_ON_WILL_READ,
                                        DetachPhase::SECOND_ON_READ_COMPLETED,
                                        DetachPhase::ON_READ_EOF,
                                        DetachPhase::ON_RESPONSE_COMPLETED,
                                        DetachPhase::NEVER_DETACH));

}  // namespace

}  // namespace content
