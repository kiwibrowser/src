// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/android_stream_reader_url_request_job.h"

#include <memory>
#include <utility>

#include "android_webview/browser/input_stream.h"
#include "android_webview/browser/net/aw_url_request_job_factory.h"
#include "android_webview/browser/net/input_stream_reader.h"
#include "base/format_macros.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "net/base/request_priority.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::TestDelegate;
using net::TestJobInterceptor;
using net::TestNetworkDelegate;
using net::TestURLRequestContext;
using net::URLRequest;
using testing::DoAll;
using testing::Ge;
using testing::Gt;
using testing::InSequence;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::NotNull;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;
using testing::StrictMock;
using testing::Test;
using testing::WithArg;
using testing::WithArgs;
using testing::_;

namespace android_webview {

namespace {

// Some of the classes will DCHECK on a null InputStream (which is desirable).
// The workaround is to use this class. None of the methods need to be
// implemented as the mock InputStreamReader should never forward calls to the
// InputStream.
class NotImplInputStream : public InputStream {
 public:
  NotImplInputStream() {}
  ~NotImplInputStream() override {}
  bool BytesAvailable(int* bytes_available) const override {
    NOTIMPLEMENTED();
    return false;
  }
  bool Skip(int64_t n, int64_t* bytes_skipped) override {
    NOTIMPLEMENTED();
    return false;
  }
  bool Read(net::IOBuffer* dest, int length, int* bytes_read) override {
    NOTIMPLEMENTED();
    return false;
  }
};

// Required in order to create an instance of AndroidStreamReaderURLRequestJob.
class StreamReaderDelegate :
    public AndroidStreamReaderURLRequestJob::Delegate {
 public:
  StreamReaderDelegate() {}

  std::unique_ptr<InputStream> OpenInputStream(JNIEnv* env,
                                               const GURL& url) override {
    return std::make_unique<NotImplInputStream>();
  }

  void OnInputStreamOpenFailed(net::URLRequest* request,
                               bool* restart) override {
    *restart = false;
  }

  bool GetMimeType(JNIEnv* env,
                   net::URLRequest* request,
                   android_webview::InputStream* stream,
                   std::string* mime_type) override {
    return false;
  }

  bool GetCharset(JNIEnv* env,
                  net::URLRequest* request,
                  android_webview::InputStream* stream,
                  std::string* charset) override {
    return false;
  }

  void AppendResponseHeaders(JNIEnv* env,
                             net::HttpResponseHeaders* headers) override {
    // no-op
  }
};

class NullStreamReaderDelegate : public StreamReaderDelegate {
 public:
  NullStreamReaderDelegate() {}

  std::unique_ptr<InputStream> OpenInputStream(JNIEnv* env,
                                               const GURL& url) override {
    return nullptr;
  }
};

class HeaderAlteringStreamReaderDelegate : public NullStreamReaderDelegate {
 public:
  HeaderAlteringStreamReaderDelegate() {}

  void AppendResponseHeaders(JNIEnv* env,
                             net::HttpResponseHeaders* headers) override {
    headers->ReplaceStatusLine(kStatusLine);
    std::string headerLine(kCustomHeaderName);
    headerLine.append(": ");
    headerLine.append(kCustomHeaderValue);
    headers->AddHeader(headerLine);
  }

  static const int kResponseCode;
  static const char* kStatusLine;
  static const char* kCustomHeaderName;
  static const char* kCustomHeaderValue;
};

const int HeaderAlteringStreamReaderDelegate::kResponseCode = 401;
const char* HeaderAlteringStreamReaderDelegate::kStatusLine =
    "HTTP/1.1 401 Gone";
const char* HeaderAlteringStreamReaderDelegate::kCustomHeaderName =
    "X-Test-Header";
const char* HeaderAlteringStreamReaderDelegate::kCustomHeaderValue =
    "TestHeaderValue";

class MockInputStreamReader : public InputStreamReader {
 public:
  MockInputStreamReader() : InputStreamReader(new NotImplInputStream()) {}
  ~MockInputStreamReader() {}

  MOCK_METHOD1(Seek, int(const net::HttpByteRange& byte_range));
  MOCK_METHOD2(ReadRawData, int(net::IOBuffer* buffer, int buffer_size));
};


class TestStreamReaderJob : public AndroidStreamReaderURLRequestJob {
 public:
  TestStreamReaderJob(net::URLRequest* request,
                      net::NetworkDelegate* network_delegate,
                      std::unique_ptr<Delegate> delegate,
                      std::unique_ptr<InputStreamReader> stream_reader)
      : AndroidStreamReaderURLRequestJob(request,
                                         network_delegate,
                                         std::move(delegate)),
        stream_reader_(std::move(stream_reader)) {
  }

  ~TestStreamReaderJob() override {}

  std::unique_ptr<InputStreamReader> CreateStreamReader(
      InputStream* stream) override {
    return std::move(stream_reader_);
  }

 protected:
  std::unique_ptr<InputStreamReader> stream_reader_;
};

}  // namespace


class AndroidStreamReaderURLRequestJobTest : public Test {
 public:
  AndroidStreamReaderURLRequestJobTest() {}

 protected:
  void SetUp() override {
    context_.set_job_factory(&factory_);
    context_.set_network_delegate(&network_delegate_);
    req_ = context_.CreateRequest(GURL("content://foo"),
                                  net::DEFAULT_PRIORITY,
                                  &url_request_delegate_);
    req_->set_method("GET");
  }

  void SetRange(net::URLRequest* req, int first_byte, int last_byte) {
    net::HttpRequestHeaders headers;
    headers.SetHeader(net::HttpRequestHeaders::kRange,
                      net::HttpByteRange::Bounded(
                          first_byte, last_byte).GetHeaderValue());
    req->SetExtraRequestHeaders(headers);
  }

  void SetUpTestJob(std::unique_ptr<InputStreamReader> stream_reader) {
    SetUpTestJob(std::move(stream_reader),
                 std::make_unique<StreamReaderDelegate>());
  }

  void SetUpTestJob(std::unique_ptr<InputStreamReader> stream_reader,
                    std::unique_ptr<AndroidStreamReaderURLRequestJob::Delegate>
                        stream_reader_delegate) {
    std::unique_ptr<TestStreamReaderJob> test_stream_reader_job(
        new TestStreamReaderJob(req_.get(), &network_delegate_,
                                std::move(stream_reader_delegate),
                                std::move(stream_reader)));
    // The Interceptor is owned by the |factory_|.
    std::unique_ptr<TestJobInterceptor> protocol_handler(
        new TestJobInterceptor);
    protocol_handler->set_main_intercept_job(std::move(test_stream_reader_job));
    bool set_protocol =
        factory_.SetProtocolHandler("content", std::move(protocol_handler));
    DCHECK(set_protocol);
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_ = {
      base::test::ScopedTaskEnvironment::MainThreadType::IO};
  TestURLRequestContext context_;
  android_webview::AwURLRequestJobFactory factory_;
  TestDelegate url_request_delegate_;
  TestNetworkDelegate network_delegate_;
  std::unique_ptr<URLRequest> req_;
};

TEST_F(AndroidStreamReaderURLRequestJobTest, ReadEmptyStream) {
  std::unique_ptr<StrictMock<MockInputStreamReader>> stream_reader(
      new StrictMock<MockInputStreamReader>());
  {
    InSequence s;
    EXPECT_CALL(*stream_reader, Seek(_))
        .WillOnce(Return(0));
    EXPECT_CALL(*stream_reader, ReadRawData(NotNull(), Gt(0)))
        .WillOnce(Return(0));
  }

  SetUpTestJob(std::move(stream_reader));

  req_->Start();

  // The TestDelegate will quit the message loop on request completion.
  base::RunLoop().Run();

  EXPECT_FALSE(url_request_delegate_.request_failed());
  EXPECT_EQ(1, network_delegate_.completed_requests());
  EXPECT_EQ(0, network_delegate_.error_count());
  EXPECT_EQ(200, req_->GetResponseCode());
}

TEST_F(AndroidStreamReaderURLRequestJobTest, ReadWithNullStream) {
  SetUpTestJob(nullptr, std::make_unique<NullStreamReaderDelegate>());
  req_->Start();

  // The TestDelegate will quit the message loop on request completion.
  base::RunLoop().Run();

  // The request_failed() method is named confusingly but all it checks is
  // whether the request got as far as calling NotifyHeadersComplete.
  EXPECT_FALSE(url_request_delegate_.request_failed());
  EXPECT_EQ(1, network_delegate_.completed_requests());
  // A null input stream shouldn't result in an error. See crbug.com/180950.
  EXPECT_EQ(0, network_delegate_.error_count());
  EXPECT_EQ(404, req_->GetResponseCode());
}

TEST_F(AndroidStreamReaderURLRequestJobTest, ModifyHeadersAndStatus) {
  SetUpTestJob(nullptr, std::make_unique<HeaderAlteringStreamReaderDelegate>());
  req_->Start();

  // The TestDelegate will quit the message loop on request completion.
  base::RunLoop().Run();

  // The request_failed() method is named confusingly but all it checks is
  // whether the request got as far as calling NotifyHeadersComplete.
  EXPECT_FALSE(url_request_delegate_.request_failed());
  EXPECT_EQ(1, network_delegate_.completed_requests());
  // A null input stream shouldn't result in an error. See crbug.com/180950.
  EXPECT_EQ(0, network_delegate_.error_count());
  EXPECT_EQ(HeaderAlteringStreamReaderDelegate::kResponseCode,
            req_->GetResponseCode());
  EXPECT_EQ(HeaderAlteringStreamReaderDelegate::kStatusLine,
            req_->response_headers()->GetStatusLine());
  EXPECT_TRUE(req_->response_headers()->HasHeader(
      HeaderAlteringStreamReaderDelegate::kCustomHeaderName));
  std::string header_value;
  EXPECT_TRUE(req_->response_headers()->EnumerateHeader(
      NULL, HeaderAlteringStreamReaderDelegate::kCustomHeaderName,
      &header_value));
  EXPECT_EQ(HeaderAlteringStreamReaderDelegate::kCustomHeaderValue,
            header_value);
}

TEST_F(AndroidStreamReaderURLRequestJobTest, ReadPartOfStream) {
  const int bytes_available = 128;
  const int offset = 32;
  const int bytes_to_read = bytes_available - offset;
  std::unique_ptr<StrictMock<MockInputStreamReader>> stream_reader(
      new StrictMock<MockInputStreamReader>());
  {
    InSequence s;
    EXPECT_CALL(*stream_reader, Seek(_))
        .WillOnce(Return(bytes_available));
    EXPECT_CALL(*stream_reader, ReadRawData(NotNull(), Ge(bytes_to_read)))
        .WillOnce(Return(bytes_to_read/2));
    EXPECT_CALL(*stream_reader, ReadRawData(NotNull(), Ge(bytes_to_read)))
        .WillOnce(Return(bytes_to_read/2));
    EXPECT_CALL(*stream_reader, ReadRawData(NotNull(), Ge(bytes_to_read)))
        .WillOnce(Return(0));
  }

  SetUpTestJob(std::move(stream_reader));

  SetRange(req_.get(), offset, bytes_available);
  req_->Start();

  base::RunLoop().Run();

  EXPECT_FALSE(url_request_delegate_.request_failed());
  EXPECT_EQ(bytes_to_read, url_request_delegate_.bytes_received());
  EXPECT_EQ(1, network_delegate_.completed_requests());
  EXPECT_EQ(0, network_delegate_.error_count());
}

TEST_F(AndroidStreamReaderURLRequestJobTest,
       ReadStreamWithMoreAvailableThanActual) {
  const int bytes_available_reported = 190;
  const int bytes_available = 128;
  const int offset = 0;
  const int bytes_to_read = bytes_available - offset;
  std::unique_ptr<StrictMock<MockInputStreamReader>> stream_reader(
      new StrictMock<MockInputStreamReader>());
  {
    InSequence s;
    EXPECT_CALL(*stream_reader, Seek(_))
        .WillOnce(Return(bytes_available_reported));
    EXPECT_CALL(*stream_reader, ReadRawData(NotNull(), Ge(bytes_to_read)))
        .WillOnce(Return(bytes_available));
    EXPECT_CALL(*stream_reader, ReadRawData(NotNull(), Ge(bytes_to_read)))
        .WillOnce(Return(0));
  }

  SetUpTestJob(std::move(stream_reader));

  SetRange(req_.get(), offset, bytes_available_reported);
  req_->Start();

  base::RunLoop().Run();

  EXPECT_FALSE(url_request_delegate_.request_failed());
  EXPECT_EQ(bytes_to_read, url_request_delegate_.bytes_received());
  EXPECT_EQ(1, network_delegate_.completed_requests());
  EXPECT_EQ(0, network_delegate_.error_count());
}

TEST_F(AndroidStreamReaderURLRequestJobTest, DeleteJobMidWaySeek) {
  const int offset = 20;
  const int bytes_available = 128;
  base::RunLoop loop;
  std::unique_ptr<StrictMock<MockInputStreamReader>> stream_reader(
      new StrictMock<MockInputStreamReader>());
  EXPECT_CALL(*stream_reader, Seek(_))
      .WillOnce(DoAll(InvokeWithoutArgs(&loop, &base::RunLoop::Quit),
                      Return(bytes_available)));
  ON_CALL(*stream_reader, ReadRawData(_, _))
      .WillByDefault(Return(0));

  SetUpTestJob(std::move(stream_reader));

  SetRange(req_.get(), offset, bytes_available);
  req_->Start();

  loop.Run();

  EXPECT_EQ(0, network_delegate_.completed_requests());
  req_->Cancel();
  EXPECT_EQ(1, network_delegate_.completed_requests());
}

TEST_F(AndroidStreamReaderURLRequestJobTest, DeleteJobMidWayRead) {
  const int offset = 20;
  const int bytes_available = 128;
  base::RunLoop loop;
  std::unique_ptr<StrictMock<MockInputStreamReader>> stream_reader(
      new StrictMock<MockInputStreamReader>());
  net::CompletionCallback read_completion_callback;
  EXPECT_CALL(*stream_reader, Seek(_))
      .WillOnce(Return(bytes_available));
  EXPECT_CALL(*stream_reader, ReadRawData(_, _))
      .WillOnce(DoAll(InvokeWithoutArgs(&loop, &base::RunLoop::Quit),
                      Return(bytes_available)));

  SetUpTestJob(std::move(stream_reader));

  SetRange(req_.get(), offset, bytes_available);
  req_->Start();

  loop.Run();

  EXPECT_EQ(0, network_delegate_.completed_requests());
  req_->Cancel();
  EXPECT_EQ(1, network_delegate_.completed_requests());
}

}  // namespace android_webview
