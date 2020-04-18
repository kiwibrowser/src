// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/intercepting_resource_handler.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/loader/mock_resource_loader.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/loader/test_resource_handler.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/webplugininfo.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/public/cpp/resource_response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {

class InterceptingResourceHandlerTest : public testing::Test {
 public:
  InterceptingResourceHandlerTest()
      : request_(context_.CreateRequest(GURL("http://www.google.com"),
                                        net::DEFAULT_PRIORITY,
                                        nullptr,
                                        TRAFFIC_ANNOTATION_FOR_TESTS)),
        old_handler_status_(
            net::URLRequestStatus::FromError(net::ERR_IO_PENDING)) {
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

    std::unique_ptr<TestResourceHandler> old_handler(
        new TestResourceHandler(&old_handler_status_, &old_handler_body_));
    old_handler_ = old_handler->GetWeakPtr();
    intercepting_handler_ = std::make_unique<InterceptingResourceHandler>(
        std::move(old_handler), request_.get());
    mock_loader_ =
        std::make_unique<MockResourceLoader>(intercepting_handler_.get());
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  net::TestURLRequestContext context_;
  std::unique_ptr<net::URLRequest> request_;

  net::URLRequestStatus old_handler_status_;
  std::string old_handler_body_;
  base::WeakPtr<TestResourceHandler> old_handler_;

  std::unique_ptr<InterceptingResourceHandler> intercepting_handler_;
  std::unique_ptr<MockResourceLoader> mock_loader_;
};

// Tests that the handler behaves properly when it doesn't have to use an
// alternate next handler.
TEST_F(InterceptingResourceHandlerTest, NoSwitching) {
  const std::string kData = "The data";
  const std::string kData2 = "Data 2";

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  ASSERT_NE(mock_loader_->io_buffer(), old_handler_->buffer());

  // The response is received. The handler should not change.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));

  // The read is replayed by the MimeSniffingResourceHandler. The data should
  // have been received by the old intercepting_handler.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(kData));
  EXPECT_EQ(kData, old_handler_body_);

  // Make sure another read behaves as expected.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(mock_loader_->io_buffer(), old_handler_->buffer());

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(kData2));
  EXPECT_EQ(kData + kData2, old_handler_body_);
}

// Tests that the data received is transmitted to the newly created
// ResourceHandler.
TEST_F(InterceptingResourceHandlerTest, HandlerSwitchNoPayload) {
  const std::string kData = "The data";
  const std::string kData2 = "Data 2";

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  ASSERT_NE(mock_loader_->io_buffer(), old_handler_->buffer());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status;
  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> new_handler_scoped(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  TestResourceHandler* new_test_handler = new_handler_scoped.get();
  intercepting_handler_->UseNewHandler(std::move(new_handler_scoped),
                                       std::string());

  // The response is received. The new ResourceHandler should be used to handle
  // the download.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));

  EXPECT_FALSE(old_handler_status_.is_success());
  EXPECT_EQ(net::ERR_ABORTED, old_handler_status_.error());
  EXPECT_EQ(std::string(), old_handler_body_);

  // The read is replayed by the MimeSniffingResourceHandler. The data should
  // have been received by the new handler.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(kData));
  EXPECT_EQ(kData, new_handler_body);

  // Make sure another read behaves as expected.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(mock_loader_->io_buffer(), new_test_handler->buffer());

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(kData2));
  EXPECT_EQ(kData + kData2, new_handler_body);
}

// Tests that the data received is transmitted to the newly created
// ResourceHandler and the specified payload to the old ResourceHandler.
TEST_F(InterceptingResourceHandlerTest, HandlerSwitchWithPayload) {
  const std::string kData = "The data";
  const std::string kData2 = "Data 2";
  const std::string kPayload = "The payload";

  // When sending a payload to the old ResourceHandler, the
  // InterceptingResourceHandler doesn't send a final EOF read.
  // TODO(mmenke):  Should it?  Or can we just get rid of that 0-byte read
  // entirely?
  old_handler_->set_expect_eof_read(false);
  scoped_refptr<net::IOBuffer> old_buffer = old_handler_->buffer();

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  ASSERT_NE(mock_loader_->io_buffer(), old_buffer.get());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status;
  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> new_handler_scoped(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  TestResourceHandler* new_test_handler = new_handler_scoped.get();
  intercepting_handler_->UseNewHandler(std::move(new_handler_scoped), kPayload);

  // The old handler should not have received the payload yet.
  ASSERT_EQ(std::string(), old_handler_body_);

  // The response is received. The new ResourceHandler should be used to handle
  // the download.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));

  // The old handler should have received the payload.
  EXPECT_EQ(kPayload, old_handler_body_);

  EXPECT_TRUE(old_handler_status_.is_success());
  EXPECT_EQ(net::OK, old_handler_status_.error());

  // The read is replayed by the MimeSniffingResourceHandler. The data should
  // have been received by the new handler.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(kData));
  EXPECT_EQ(kData, new_handler_body);

  // Make sure another read behaves as expected.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(mock_loader_->io_buffer(), new_test_handler->buffer());

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(kData2));
  EXPECT_EQ(kData + kData2, new_handler_body);
}

// Tests that the handler behaves properly if the old handler fails will read.
TEST_F(InterceptingResourceHandlerTest, OldHandlerFailsWillRead) {
  old_handler_->set_on_will_read_result(false);

  // Simulate the MimeSniffingResourceHandler buffering the data. The old
  // handler should tell the caller to fail.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::CANCELED, mock_loader_->OnWillRead());
  EXPECT_EQ(net::ERR_ABORTED, mock_loader_->error_code());
}

// Tests that the handler behaves properly if the new handler fails in
// OnWillStart.
TEST_F(InterceptingResourceHandlerTest, NewHandlerFailsOnWillStart) {
  scoped_refptr<net::IOBuffer> old_buffer = old_handler_->buffer();

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  ASSERT_NE(mock_loader_->io_buffer(), old_buffer.get());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status;
  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> new_handler(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  new_handler->set_on_will_start_result(false);
  intercepting_handler_->UseNewHandler(std::move(new_handler), std::string());

  // The response is received. The new ResourceHandler should tell us to fail.
  ASSERT_EQ(MockResourceLoader::Status::CANCELED,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));
  EXPECT_EQ(net::ERR_ABORTED, mock_loader_->error_code());
}

// Tests that the handler behaves properly if the new handler fails response
// started.
TEST_F(InterceptingResourceHandlerTest, NewHandlerFailsResponseStarted) {
  scoped_refptr<net::IOBuffer> old_buffer = old_handler_->buffer();

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  ASSERT_NE(mock_loader_->io_buffer(), old_buffer.get());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status;
  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> new_handler(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  new_handler->set_on_response_started_result(false);
  intercepting_handler_->UseNewHandler(std::move(new_handler), std::string());

  // The response is received. The new ResourceHandler should tell us to fail.
  ASSERT_EQ(MockResourceLoader::Status::CANCELED,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));
  EXPECT_EQ(net::ERR_ABORTED, mock_loader_->error_code());
}

// Tests that the handler behaves properly if the new handler fails will read.
TEST_F(InterceptingResourceHandlerTest, NewHandlerFailsWillRead) {
  const char kData[] = "The data";

  scoped_refptr<net::IOBuffer> old_buffer = old_handler_->buffer();

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  ASSERT_NE(mock_loader_->io_buffer(), old_buffer.get());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status;
  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> new_handler(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  new_handler->set_on_will_read_result(false);
  intercepting_handler_->UseNewHandler(std::move(new_handler), std::string());

  // The response is received. The new handler should not have been asked to
  // read yet.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));
  EXPECT_EQ(net::URLRequestStatus::CANCELED, old_handler_status_.status());
  EXPECT_EQ(net::ERR_ABORTED, old_handler_status_.error());

  // The read is replayed by the MimeSniffingResourceHandler. The new
  // handler should tell the caller to fail.

  ASSERT_EQ(MockResourceLoader::Status::CANCELED,
            mock_loader_->OnReadCompleted(kData));
  EXPECT_EQ(net::ERR_ABORTED, mock_loader_->error_code());
}

// Tests that the handler behaves properly if the new handler fails read
// completed.
TEST_F(InterceptingResourceHandlerTest, NewHandlerFailsReadCompleted) {
  const char kData[] = "The data";

  scoped_refptr<net::IOBuffer> old_buffer = old_handler_->buffer();

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  ASSERT_NE(mock_loader_->io_buffer(), old_buffer.get());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status;
  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> new_handler(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  new_handler->set_on_read_completed_result(false);
  intercepting_handler_->UseNewHandler(std::move(new_handler), std::string());

  // The response is received.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));
  EXPECT_EQ(net::URLRequestStatus::CANCELED, old_handler_status_.status());
  EXPECT_EQ(net::ERR_ABORTED, old_handler_status_.error());

  // The read is replayed by the MimeSniffingResourceHandler. The new handler
  // should tell the caller to fail.
  ASSERT_EQ(MockResourceLoader::Status::CANCELED,
            mock_loader_->OnReadCompleted(kData));
  EXPECT_EQ(net::ERR_ABORTED, mock_loader_->error_code());
}

// The old handler sets |defer| to true in OnReadCompleted and
// OnResponseCompleted. The new handler sets |defer| to true in
// OnResponseStarted and OnReadCompleted.
TEST_F(InterceptingResourceHandlerTest, DeferredOperations) {
  const char kData[] = "The data";
  const char kPayload[] = "The long long long long long payload";
  // This should be less than half the size of the payload, so it needs at least
  // 3 reads to receive.
  const int kOldHandlerBufferSize = arraysize(kPayload) / 3;

  // When sending a payload to the old ResourceHandler, the
  // InterceptingResourceHandler doesn't send a final EOF read.
  // TODO(mmenke):  Should it?  Or can we just get rid of that 0-byte read
  // entirely?
  old_handler_->set_expect_eof_read(false);
  old_handler_->SetBufferSize(kOldHandlerBufferSize);
  old_handler_->set_defer_on_will_read(true);
  old_handler_->set_defer_on_read_completed(true);
  scoped_refptr<net::IOBuffer> old_buffer = old_handler_->buffer();

  // Simulate the MimeSniffingResourceHandler buffering the data.

  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  // The old handler defers the read.
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnWillRead());
  old_handler_->WaitUntilDeferred();

  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, old_handler_status_.status());
  EXPECT_EQ(1, old_handler_->on_will_read_called());
  EXPECT_EQ(0, old_handler_->on_read_completed_called());
  EXPECT_EQ(0, old_handler_->on_response_completed_called());

  // Defer the next OnWillRead, too. This is needed to test the case where
  // OnWillRead completes asynchronously when passing the payload to the old
  // handler.
  old_handler_->set_defer_on_will_read(true);

  // The old handle resumes the request.
  old_handler_->Resume();

  // Resume() call may do work asynchronously. Wait until that's done.
  mock_loader_->WaitUntilIdleOrCanceled();
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());

  ASSERT_NE(mock_loader_->io_buffer(), old_buffer.get());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status = {net::URLRequestStatus::IO_PENDING,
                                              0};

  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> scoped_new_handler(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  base::WeakPtr<TestResourceHandler> new_handler =
      scoped_new_handler->GetWeakPtr();
  scoped_new_handler->SetBufferSize(1);
  scoped_new_handler->set_defer_on_will_start(true);
  scoped_new_handler->set_defer_on_response_started(true);
  scoped_new_handler->set_defer_on_will_read(true);
  scoped_new_handler->set_defer_on_read_completed(true);
  scoped_new_handler->set_defer_on_response_completed(true);
  intercepting_handler_->UseNewHandler(std::move(scoped_new_handler), kPayload);

  // The response is received, and then deferred by the old handler's
  // OnReadCompleted method.
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));
  old_handler_->WaitUntilDeferred();

  EXPECT_EQ(1, old_handler_->on_read_completed_called());
  EXPECT_EQ(0, old_handler_->on_response_completed_called());
  EXPECT_EQ(0, new_handler->on_response_started_called());

  // The old handler has received the first N bytes of the payload synchronously
  // where N is the size of the buffer exposed via OnWillRead.
  EXPECT_EQ(std::string(kPayload, 0, kOldHandlerBufferSize), old_handler_body_);
  EXPECT_EQ(std::string(), new_handler_body);
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, old_handler_status_.status());
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, new_handler_status.status());

  // Run until the old handler's OnWillRead method defers the request while
  // replaying the payload.
  old_handler_->Resume();
  old_handler_->WaitUntilDeferred();
  EXPECT_EQ(2, old_handler_->on_will_read_called());
  EXPECT_EQ(1, old_handler_->on_read_completed_called());
  EXPECT_EQ(0, old_handler_->on_response_completed_called());
  EXPECT_EQ(std::string(kPayload, 0, kOldHandlerBufferSize), old_handler_body_);
  EXPECT_EQ(std::string(), new_handler_body);
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, old_handler_status_.status());
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, new_handler_status.status());

  // Run until the new handler's OnWillStart method defers the request.
  old_handler_->Resume();
  new_handler->WaitUntilDeferred();

  EXPECT_EQ(1, new_handler->on_will_start_called());
  EXPECT_EQ(0, new_handler->on_response_started_called());
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->status());
  EXPECT_EQ(kPayload, old_handler_body_);
  EXPECT_EQ(net::URLRequestStatus::SUCCESS, old_handler_status_.status());
  EXPECT_FALSE(old_handler_);
  EXPECT_EQ(std::string(), new_handler_body);
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, new_handler_status.status());

  // Run until the new handler's OnResponseStarted method defers the request.
  new_handler->Resume();
  // Resume() call may do work asynchronously. Wait until that's done.
  new_handler->WaitUntilDeferred();

  EXPECT_EQ(1, new_handler->on_response_started_called());
  EXPECT_EQ(0, new_handler->on_will_read_called());
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->status());
  EXPECT_EQ(std::string(), new_handler_body);
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, new_handler_status.status());

  // Resuming should finally call back into the ResourceController.
  new_handler->Resume();
  mock_loader_->WaitUntilIdleOrCanceled();
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());

  // Data is read, the new handler defers OnWillRead.
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnReadCompleted(kData));
  new_handler->WaitUntilDeferred();
  EXPECT_EQ(1, new_handler->on_will_read_called());
  EXPECT_EQ(0, new_handler->on_read_completed_called());

  // The new ResourceHandler resumes, and then defers again in OnReadCompleted.
  new_handler->Resume();
  new_handler->WaitUntilDeferred();
  EXPECT_EQ(1, new_handler->on_will_read_called());
  EXPECT_EQ(1, new_handler->on_read_completed_called());
  EXPECT_EQ(0, new_handler->on_response_completed_called());

  EXPECT_EQ("T", new_handler_body);

  // New handler resumes again, everything continues synchronously until all
  // written data is consumed.
  new_handler->Resume();
  mock_loader_->WaitUntilIdleOrCanceled();
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->status());
  EXPECT_EQ(kData, new_handler_body);
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, new_handler_status.status());
  EXPECT_EQ(0, new_handler->on_read_eof_called());
  EXPECT_EQ(0, new_handler->on_response_completed_called());

  // Final EOF byte is read.
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnReadCompleted(""));
  EXPECT_EQ(1, new_handler->on_read_eof_called());
  EXPECT_EQ(0, new_handler->on_response_completed_called());

  ASSERT_EQ(
      MockResourceLoader::Status::CALLBACK_PENDING,
      mock_loader_->OnResponseCompleted({net::URLRequestStatus::SUCCESS, 0}));
  EXPECT_EQ(net::URLRequestStatus::SUCCESS, new_handler_status.status());
  EXPECT_EQ(1, new_handler->on_response_completed_called());
}

// Test cancellation where there is only the old handler in an
// InterceptingResourceHandler.
TEST_F(InterceptingResourceHandlerTest, CancelOldHandler) {
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnResponseCompletedFromExternalOutOfBandCancel(
                {net::URLRequestStatus::CANCELED, net::ERR_FAILED}));
  EXPECT_EQ(net::URLRequestStatus::CANCELED, old_handler_status_.status());
  EXPECT_EQ(net::ERR_FAILED, old_handler_status_.error());
}

// Test cancellation where there is only the new handler in an
// InterceptingResourceHandler.
TEST_F(InterceptingResourceHandlerTest, CancelNewHandler) {
  const std::string kPayload = "The payload";

  // When sending a payload to the old ResourceHandler, the
  // InterceptingResourceHandler doesn't send a final EOF read.
  // TODO(mmenke):  Should it?  Or can we just get rid of that 0-byte read
  // entirely?
  old_handler_->set_expect_eof_read(false);

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status = {net::URLRequestStatus::IO_PENDING,
                                              0};

  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> new_handler(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  new_handler->SetBufferSize(1);
  new_handler->set_defer_on_response_started(true);
  new_handler->set_defer_on_response_completed(true);
  intercepting_handler_->UseNewHandler(std::move(new_handler), kPayload);

  // The response is received.
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));

  EXPECT_EQ(net::URLRequestStatus::SUCCESS, old_handler_status_.status());
  EXPECT_FALSE(old_handler_);
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, new_handler_status.status());

  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseCompletedFromExternalOutOfBandCancel(
                {net::URLRequestStatus::CANCELED, net::ERR_FAILED}));
  EXPECT_EQ(net::URLRequestStatus::CANCELED, new_handler_status.status());
  EXPECT_EQ(net::ERR_FAILED, new_handler_status.error());
}

// Test cancellation where there are both the old and the new handlers in an
// InterceptingResourceHandler.
TEST_F(InterceptingResourceHandlerTest, CancelBothHandlers) {
  const std::string kPayload = "The payload";

  old_handler_->set_defer_on_read_completed(true);

  // Simulate the MimeSniffingResourceHandler buffering the data.
  ASSERT_EQ(MockResourceLoader::Status::IDLE,
            mock_loader_->OnWillStart(request_->url()));
  ASSERT_EQ(MockResourceLoader::Status::IDLE, mock_loader_->OnWillRead());

  // Simulate the MimeSniffingResourceHandler asking the
  // InterceptingResourceHandler to switch to a new handler.
  net::URLRequestStatus new_handler_status = {net::URLRequestStatus::IO_PENDING,
                                              0};

  std::string new_handler_body;
  std::unique_ptr<TestResourceHandler> new_handler(
      new TestResourceHandler(&new_handler_status, &new_handler_body));
  new_handler->SetBufferSize(1);
  new_handler->set_defer_on_response_completed(true);
  intercepting_handler_->UseNewHandler(std::move(new_handler), kPayload);

  // The response is received.
  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseStarted(
                base::MakeRefCounted<network::ResourceResponse>()));

  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, old_handler_status_.status());
  EXPECT_EQ(net::URLRequestStatus::IO_PENDING, new_handler_status.status());

  ASSERT_EQ(MockResourceLoader::Status::CALLBACK_PENDING,
            mock_loader_->OnResponseCompletedFromExternalOutOfBandCancel(
                {net::URLRequestStatus::CANCELED, net::ERR_FAILED}));
  EXPECT_EQ(net::URLRequestStatus::CANCELED, old_handler_status_.status());
  EXPECT_EQ(net::ERR_FAILED, old_handler_status_.error());
  EXPECT_EQ(net::URLRequestStatus::CANCELED, new_handler_status.status());
  EXPECT_EQ(net::ERR_FAILED, new_handler_status.error());
}

}  // namespace

}  // namespace content
