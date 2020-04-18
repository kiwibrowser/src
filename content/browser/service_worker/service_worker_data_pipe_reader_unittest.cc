// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_data_pipe_reader.h"

#include "base/run_loop.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_url_request_job.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/io_buffer.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/mojom/request_context_frame_type.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {

namespace {

const char kTestData[] = "Here is sample text for the blob.";

}  // namespace

class MockServiceWorkerURLRequestJob : public ServiceWorkerURLRequestJob {
 public:
  explicit MockServiceWorkerURLRequestJob(
      ServiceWorkerURLRequestJob::Delegate* delegate)
      : ServiceWorkerURLRequestJob(
            nullptr,
            nullptr,
            "",
            nullptr,
            nullptr,
            network::mojom::FetchRequestMode::kNoCORS,
            network::mojom::FetchCredentialsMode::kOmit,
            network::mojom::FetchRedirectMode::kFollow,
            std::string() /* integrity */,
            false /* keepalive */,
            RESOURCE_TYPE_MAIN_FRAME,
            REQUEST_CONTEXT_TYPE_HYPERLINK,
            network::mojom::RequestContextFrameType::kTopLevel,
            scoped_refptr<network::ResourceRequestBody>(),
            delegate),
        is_response_started_(false) {}

  void OnResponseStarted() override { is_response_started_ = true; }

  void OnReadRawDataComplete(int bytes_read) override {
    async_read_bytes_.push_back(bytes_read);
  }

  void RecordResult(ServiceWorkerMetrics::URLRequestJobResult result) override {
    results_.push_back(result);
  }

  bool is_response_started() { return is_response_started_; }
  const std::vector<int>& async_read_bytes() { return async_read_bytes_; }
  const std::vector<ServiceWorkerMetrics::URLRequestJobResult>& results() {
    return results_;
  }

 private:
  bool is_response_started_;
  std::vector<int> async_read_bytes_;
  std::vector<ServiceWorkerMetrics::URLRequestJobResult> results_;
};

class ServiceWorkerDataPipeReaderTest
    : public testing::Test,
      public ServiceWorkerURLRequestJob::Delegate {
 public:
  ServiceWorkerDataPipeReaderTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    helper_ = std::make_unique<EmbeddedWorkerTestHelper>(base::FilePath());
    mock_url_request_job_ =
        std::make_unique<MockServiceWorkerURLRequestJob>(this);
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = GURL("https://example.com/");
    registration_ = new ServiceWorkerRegistration(
        options, 1L, helper_->context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(
        registration_.get(), GURL("https://example.com/service_worker.js"), 1L,
        helper_->context()->AsWeakPtr());
    std::vector<ServiceWorkerDatabase::ResourceRecord> records;
    records.push_back(
        ServiceWorkerDatabase::ResourceRecord(10, version_->script_url(), 100));
    version_->script_cache_map()->SetResources(records);
    version_->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
  }

  std::unique_ptr<ServiceWorkerDataPipeReader> CreateTargetDataPipeReader(
      blink::mojom::ServiceWorkerStreamCallbackPtr* stream_callback,
      mojo::DataPipe* data_pipe) {
    blink::mojom::ServiceWorkerStreamHandlePtr stream_handle =
        blink::mojom::ServiceWorkerStreamHandle::New();
    stream_handle->stream = std::move(data_pipe->consumer_handle);
    stream_handle->callback_request = mojo::MakeRequest(stream_callback);
    return std::make_unique<ServiceWorkerDataPipeReader>(
        mock_url_request_job_.get(), version_, std::move(stream_handle));
  }

  // Implements ServiceWorkerURLRequestJob::Delegate.
  void OnPrepareToRestart() override { NOTREACHED(); }

  ServiceWorkerVersion* GetServiceWorkerVersion(
      ServiceWorkerMetrics::URLRequestJobResult*) override {
    NOTREACHED();
    return nullptr;
  }

  bool RequestStillValid(ServiceWorkerMetrics::URLRequestJobResult*) override {
    NOTREACHED();
    return false;
  }

  void TearDown() override { helper_.reset(); }

  MockServiceWorkerURLRequestJob* mock_url_request_job() {
    return mock_url_request_job_.get();
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  std::unique_ptr<MockServiceWorkerURLRequestJob> mock_url_request_job_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
};

class ServiceWorkerDataPipeReaderTestP
    : public ServiceWorkerDataPipeReaderTest,
      public testing::WithParamInterface<
          std::tuple<bool /* should_close_connection_first */,
                     bool /* has_body */>> {
 public:
  ServiceWorkerDataPipeReaderTestP() {}
  virtual ~ServiceWorkerDataPipeReaderTestP() {}

 protected:
  bool should_close_connection_first() const { return std::get<0>(GetParam()); }
  bool has_body() const { return std::get<1>(GetParam()); }
};

TEST_P(ServiceWorkerDataPipeReaderTestP, SyncRead) {
  blink::mojom::ServiceWorkerStreamCallbackPtr stream_callback;
  mojo::DataPipe data_pipe;
  std::unique_ptr<ServiceWorkerDataPipeReader> data_pipe_reader =
      CreateTargetDataPipeReader(&stream_callback, &data_pipe);

  // Push enough data.
  if (has_body()) {
    std::string expected_response;
    expected_response.reserve((sizeof(kTestData) - 1) * 1024);
    for (int i = 0; i < 1024; ++i) {
      expected_response += kTestData;
      uint32_t written_bytes = sizeof(kTestData) - 1;
      MojoResult result = data_pipe.producer_handle->WriteData(
          kTestData, &written_bytes, MOJO_WRITE_DATA_FLAG_NONE);
      ASSERT_EQ(MOJO_RESULT_OK, result);
      EXPECT_EQ(sizeof(kTestData) - 1, written_bytes);
    }
  }
  data_pipe.producer_handle.reset();
  stream_callback->OnCompleted();
  base::RunLoop().RunUntilIdle();

  // Nothing has started.
  EXPECT_FALSE(mock_url_request_job()->is_response_started());
  EXPECT_EQ(0UL, mock_url_request_job()->async_read_bytes().size());
  EXPECT_EQ(0UL, mock_url_request_job()->results().size());

  // Start to read.
  data_pipe_reader->Start();
  EXPECT_TRUE(mock_url_request_job()->is_response_started());
  const int buffer_size = sizeof(kTestData);
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(buffer_size);
  buffer->data()[buffer_size - 1] = '\0';

  // Read successfully.
  if (has_body()) {
    std::string retrieved_response;
    retrieved_response.reserve(buffer_size * 1024);
    for (int i = 0; i < 1024; ++i) {
      EXPECT_EQ(buffer_size - 1,
                data_pipe_reader->ReadRawData(buffer.get(), buffer_size - 1));
      EXPECT_STREQ(kTestData, buffer->data());
      retrieved_response += buffer->data();
    }
  }

  // Finish successfully.
  EXPECT_EQ(net::OK,
            data_pipe_reader->ReadRawData(buffer.get(), buffer_size - 1));
  EXPECT_EQ(0UL, mock_url_request_job()->async_read_bytes().size());
  ASSERT_EQ(1UL, mock_url_request_job()->results().size());
  EXPECT_EQ(ServiceWorkerMetrics::REQUEST_JOB_STREAM_RESPONSE,
            mock_url_request_job()->results()[0]);
}

TEST_P(ServiceWorkerDataPipeReaderTestP, SyncAbort) {
  blink::mojom::ServiceWorkerStreamCallbackPtr stream_callback;
  mojo::DataPipe data_pipe;
  std::unique_ptr<ServiceWorkerDataPipeReader> data_pipe_reader =
      CreateTargetDataPipeReader(&stream_callback, &data_pipe);

  // Push enough data.
  if (has_body()) {
    std::string expected_response;
    expected_response.reserve((sizeof(kTestData) - 1) * 1024);
    for (int i = 0; i < 1024; ++i) {
      expected_response += kTestData;
      uint32_t written_bytes = sizeof(kTestData) - 1;
      MojoResult result = data_pipe.producer_handle->WriteData(
          kTestData, &written_bytes, MOJO_WRITE_DATA_FLAG_NONE);
      ASSERT_EQ(MOJO_RESULT_OK, result);
      EXPECT_EQ(sizeof(kTestData) - 1, written_bytes);
    }
  }
  data_pipe.producer_handle.reset();
  stream_callback->OnAborted();
  base::RunLoop().RunUntilIdle();

  // Nothing has started.
  EXPECT_FALSE(mock_url_request_job()->is_response_started());
  EXPECT_EQ(0UL, mock_url_request_job()->async_read_bytes().size());
  EXPECT_EQ(0UL, mock_url_request_job()->results().size());

  // Start to read.
  data_pipe_reader->Start();
  EXPECT_TRUE(mock_url_request_job()->is_response_started());
  const int buffer_size = sizeof(kTestData);
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(buffer_size);
  buffer->data()[buffer_size - 1] = '\0';

  // Read successfully.
  if (has_body()) {
    std::string retrieved_response;
    retrieved_response.reserve(buffer_size * 1024);
    for (int i = 0; i < 1024; ++i) {
      EXPECT_EQ(buffer_size - 1,
                data_pipe_reader->ReadRawData(buffer.get(), buffer_size - 1));
      EXPECT_STREQ(kTestData, buffer->data());
      retrieved_response += buffer->data();
    }
  }

  // Abort after all data has been read.
  EXPECT_EQ(net::ERR_CONNECTION_RESET,
            data_pipe_reader->ReadRawData(buffer.get(), buffer_size - 1));
  EXPECT_EQ(0UL, mock_url_request_job()->async_read_bytes().size());
  ASSERT_EQ(1UL, mock_url_request_job()->results().size());
  EXPECT_EQ(ServiceWorkerMetrics::REQUEST_JOB_ERROR_STREAM_ABORTED,
            mock_url_request_job()->results()[0]);
}

TEST_P(ServiceWorkerDataPipeReaderTestP, AsyncRead) {
  blink::mojom::ServiceWorkerStreamCallbackPtr stream_callback;
  mojo::DataPipe data_pipe;
  std::unique_ptr<ServiceWorkerDataPipeReader> data_pipe_reader =
      CreateTargetDataPipeReader(&stream_callback, &data_pipe);

  // Nothing has started.
  EXPECT_FALSE(mock_url_request_job()->is_response_started());
  EXPECT_EQ(0UL, mock_url_request_job()->async_read_bytes().size());
  EXPECT_EQ(0UL, mock_url_request_job()->results().size());

  // Start to read.
  data_pipe_reader->Start();
  EXPECT_TRUE(mock_url_request_job()->is_response_started());
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(sizeof(kTestData));
  buffer->data()[sizeof(kTestData) - 1] = '\0';
  std::string expected_response;
  std::string retrieved_response;
  expected_response.reserve((sizeof(kTestData) - 1) * 1024);
  retrieved_response.reserve((sizeof(kTestData) - 1) * 1024);

  if (has_body()) {
    for (int i = 0; i < 1024; ++i) {
      // Data is not coming. It should be pending state.
      EXPECT_EQ(net::ERR_IO_PENDING, data_pipe_reader->ReadRawData(
                                         buffer.get(), sizeof(kTestData) - 1));

      // Push a portion of data.
      uint32_t written_bytes = sizeof(kTestData) - 1;
      MojoResult result = data_pipe.producer_handle->WriteData(
          kTestData, &written_bytes, MOJO_WRITE_DATA_FLAG_NONE);
      ASSERT_EQ(MOJO_RESULT_OK, result);
      EXPECT_EQ(sizeof(kTestData) - 1, written_bytes);
      expected_response += kTestData;
      base::RunLoop().RunUntilIdle();

      // Read the pushed data correctly.
      ASSERT_EQ(static_cast<size_t>(i + 1),
                mock_url_request_job()->async_read_bytes().size());
      EXPECT_EQ(static_cast<int>(sizeof(kTestData) - 1),
                mock_url_request_job()->async_read_bytes()[i]);
      EXPECT_STREQ(kTestData, buffer->data());
    }
  }

  // Data is not coming. It should be pending state.
  EXPECT_EQ(net::ERR_IO_PENDING,
            data_pipe_reader->ReadRawData(buffer.get(), sizeof(kTestData) - 1));

  // Finish successfully when connection is closed AND OnCompleted is delivered.
  size_t num_read = mock_url_request_job()->async_read_bytes().size();
  if (should_close_connection_first()) {
    data_pipe.producer_handle.reset();
  } else {
    stream_callback->OnCompleted();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(num_read, mock_url_request_job()->async_read_bytes().size());
  EXPECT_EQ(0UL, mock_url_request_job()->results().size());

  if (should_close_connection_first()) {
    stream_callback->OnCompleted();
  } else {
    data_pipe.producer_handle.reset();
  }
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(num_read + 1, mock_url_request_job()->async_read_bytes().size());
  EXPECT_EQ(net::OK, mock_url_request_job()->async_read_bytes().back());
  ASSERT_EQ(1UL, mock_url_request_job()->results().size());
  EXPECT_EQ(ServiceWorkerMetrics::REQUEST_JOB_STREAM_RESPONSE,
            mock_url_request_job()->results()[0]);
}

TEST_P(ServiceWorkerDataPipeReaderTestP, AsyncAbort) {
  blink::mojom::ServiceWorkerStreamCallbackPtr stream_callback;
  mojo::DataPipe data_pipe;
  std::unique_ptr<ServiceWorkerDataPipeReader> data_pipe_reader =
      CreateTargetDataPipeReader(&stream_callback, &data_pipe);

  // Nothing has started.
  EXPECT_FALSE(mock_url_request_job()->is_response_started());
  EXPECT_EQ(0UL, mock_url_request_job()->async_read_bytes().size());
  EXPECT_EQ(0UL, mock_url_request_job()->results().size());

  // Start to read.
  data_pipe_reader->Start();
  EXPECT_TRUE(mock_url_request_job()->is_response_started());
  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(sizeof(kTestData));
  buffer->data()[sizeof(kTestData) - 1] = '\0';
  std::string expected_response;
  std::string retrieved_response;
  expected_response.reserve((sizeof(kTestData) - 1) * 1024);
  retrieved_response.reserve((sizeof(kTestData) - 1) * 1024);

  if (has_body()) {
    for (int i = 0; i < 1024; ++i) {
      // Data is not coming. It should be pending state.
      EXPECT_EQ(net::ERR_IO_PENDING, data_pipe_reader->ReadRawData(
                                         buffer.get(), sizeof(kTestData) - 1));

      // Push a portion of data.
      uint32_t written_bytes = sizeof(kTestData) - 1;
      MojoResult result = data_pipe.producer_handle->WriteData(
          kTestData, &written_bytes, MOJO_WRITE_DATA_FLAG_NONE);
      ASSERT_EQ(MOJO_RESULT_OK, result);
      EXPECT_EQ(sizeof(kTestData) - 1, written_bytes);
      expected_response += kTestData;
      base::RunLoop().RunUntilIdle();

      // Read the pushed data correctly.
      ASSERT_EQ(static_cast<size_t>(i + 1),
                mock_url_request_job()->async_read_bytes().size());
      EXPECT_EQ(static_cast<int>(sizeof(kTestData) - 1),
                mock_url_request_job()->async_read_bytes()[i]);
      EXPECT_STREQ(kTestData, buffer->data());
    }
  }

  // Data is not coming. It should be pending state.
  EXPECT_EQ(net::ERR_IO_PENDING,
            data_pipe_reader->ReadRawData(buffer.get(), sizeof(kTestData) - 1));

  // Abort when connection is closed AND OnAborted is delivered.
  size_t num_read = mock_url_request_job()->async_read_bytes().size();
  if (should_close_connection_first()) {
    data_pipe.producer_handle.reset();
  } else {
    stream_callback->OnAborted();
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(num_read, mock_url_request_job()->async_read_bytes().size());
  EXPECT_EQ(0UL, mock_url_request_job()->results().size());

  if (should_close_connection_first()) {
    stream_callback->OnAborted();
  } else {
    data_pipe.producer_handle.reset();
  }
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(num_read + 1, mock_url_request_job()->async_read_bytes().size());
  EXPECT_EQ(net::ERR_CONNECTION_RESET,
            mock_url_request_job()->async_read_bytes().back());
  ASSERT_EQ(1UL, mock_url_request_job()->results().size());
  EXPECT_EQ(ServiceWorkerMetrics::REQUEST_JOB_ERROR_STREAM_ABORTED,
            mock_url_request_job()->results()[0]);
}

INSTANTIATE_TEST_CASE_P(ServiceWorkerDataPipeReaderTest,
                        ServiceWorkerDataPipeReaderTestP,
                        testing::Combine(testing::Bool(), testing::Bool()));

}  // namespace content
