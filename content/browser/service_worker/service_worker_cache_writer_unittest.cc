// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_cache_writer.h"

#include <stddef.h>

#include <list>
#include <string>

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

// A test implementation of ServiceWorkerResponseReader.
//
// This class exposes the ability to expect reads (see ExpectRead*() below).
// Each call to ReadInfo() or ReadData() consumes another expected read, in the
// order those reads were expected, so:
//    reader->ExpectReadInfoOk(5, false);
//    reader->ExpectReadDataOk("abcdef", false);
//    reader->ExpectReadDataOk("ghijkl", false);
// Expects these calls, in this order:
//    reader->ReadInfo(...);  // reader writes 5 into
//                            // |info_buf->response_data_size|
//    reader->ReadData(...);  // reader writes "abcdef" into |buf|
//    reader->ReadData(...);  // reader writes "ghijkl" into |buf|
// If an unexpected call happens, this class DCHECKs.
// If an expected read is marked "async", it will not complete immediately, but
// must be completed by the test using CompletePendingRead().
// These is a convenience method AllExpectedReadsDone() which returns whether
// there are any expected reads that have not yet happened.
class MockServiceWorkerResponseReader : public ServiceWorkerResponseReader {
 public:
  MockServiceWorkerResponseReader()
      : ServiceWorkerResponseReader(
            0,
            base::WeakPtr<AppCacheDiskCacheInterface>()) {}
  ~MockServiceWorkerResponseReader() override {}

  // ServiceWorkerResponseReader overrides
  void ReadInfo(HttpResponseInfoIOBuffer* info_buf,
                OnceCompletionCallback callback) override;
  void ReadData(net::IOBuffer* buf,
                int buf_len,
                OnceCompletionCallback callback) override;

  // Test helpers. ExpectReadInfo() and ExpectReadData() give precise control
  // over both the data to be written and the result to return.
  // ExpectReadInfoOk() and ExpectReadDataOk() are convenience functions for
  // expecting successful reads, which always have their length as their result.

  // Expect a call to ReadInfo() on this reader. For these functions, |len| will
  // be used as |response_data_size|, not as the length of this particular read.
  void ExpectReadInfo(size_t len, bool async, int result);
  void ExpectReadInfoOk(size_t len, bool async);

  // Expect a call to ReadData() on this reader. For these functions, |len| is
  // the length of the data to be written back; in ExpectReadDataOk(), |len| is
  // implicitly the length of |data|.
  void ExpectReadData(const char* data, size_t len, bool async, int result);
  void ExpectReadDataOk(const std::string& data, bool async);

  // Complete a pending async read. It is an error to call this function without
  // a pending async read (ie, a previous call to ReadInfo() or ReadData()
  // having not run its callback yet).
  void CompletePendingRead();

  // Returns whether all expected reads have occurred.
  bool AllExpectedReadsDone() { return expected_reads_.size() == 0; }

 private:
  struct ExpectedRead {
    ExpectedRead(size_t len, bool async, int result)
        : data(nullptr), len(len), info(true), async(async), result(result) {}
    ExpectedRead(const char* data, size_t len, bool async, int result)
        : data(data), len(len), info(false), async(async), result(result) {}
    const char* data;
    size_t len;
    bool info;
    bool async;
    int result;
  };

  base::queue<ExpectedRead> expected_reads_;
  scoped_refptr<net::IOBuffer> pending_buffer_;
  size_t pending_buffer_len_;
  scoped_refptr<HttpResponseInfoIOBuffer> pending_info_;
  OnceCompletionCallback pending_callback_;

  DISALLOW_COPY_AND_ASSIGN(MockServiceWorkerResponseReader);
};

void MockServiceWorkerResponseReader::ReadInfo(
    HttpResponseInfoIOBuffer* info_buf,
    OnceCompletionCallback callback) {
  DCHECK(!expected_reads_.empty());
  ExpectedRead expected = expected_reads_.front();
  EXPECT_TRUE(expected.info);
  if (expected.async) {
    pending_info_ = info_buf;
    pending_callback_ = std::move(callback);
  } else {
    expected_reads_.pop();
    info_buf->response_data_size = expected.len;
    std::move(callback).Run(expected.result);
  }
}

void MockServiceWorkerResponseReader::ReadData(
    net::IOBuffer* buf,
    int buf_len,
    OnceCompletionCallback callback) {
  DCHECK(!expected_reads_.empty());
  ExpectedRead expected = expected_reads_.front();
  EXPECT_FALSE(expected.info);
  if (expected.async) {
    pending_callback_ = std::move(callback);
    pending_buffer_ = buf;
    pending_buffer_len_ = static_cast<size_t>(buf_len);
  } else {
    expected_reads_.pop();
    if (expected.len > 0) {
      size_t to_read = std::min(static_cast<size_t>(buf_len), expected.len);
      memcpy(buf->data(), expected.data, to_read);
    }
    std::move(callback).Run(expected.result);
  }
}

void MockServiceWorkerResponseReader::ExpectReadInfo(size_t len,
                                                     bool async,
                                                     int result) {
  expected_reads_.push(ExpectedRead(len, async, result));
}

void MockServiceWorkerResponseReader::ExpectReadInfoOk(size_t len, bool async) {
  expected_reads_.push(ExpectedRead(len, async, len));
}

void MockServiceWorkerResponseReader::ExpectReadData(const char* data,
                                                     size_t len,
                                                     bool async,
                                                     int result) {
  expected_reads_.push(ExpectedRead(data, len, async, result));
}

void MockServiceWorkerResponseReader::ExpectReadDataOk(const std::string& data,
                                                       bool async) {
  expected_reads_.push(
      ExpectedRead(data.data(), data.size(), async, data.size()));
}

void MockServiceWorkerResponseReader::CompletePendingRead() {
  DCHECK(!expected_reads_.empty());
  ExpectedRead expected = expected_reads_.front();
  expected_reads_.pop();
  EXPECT_TRUE(expected.async);
  if (expected.info) {
    pending_info_->response_data_size = expected.len;
  } else {
    size_t to_read = std::min(pending_buffer_len_, expected.len);
    if (to_read > 0)
      memcpy(pending_buffer_->data(), expected.data, to_read);
  }
  pending_info_ = nullptr;
  pending_buffer_ = nullptr;
  OnceCompletionCallback callback = std::move(pending_callback_);
  pending_callback_.Reset();
  std::move(callback).Run(expected.result);
}

// A test implementation of ServiceWorkerResponseWriter.
//
// This class exposes the ability to expect writes (see ExpectWrite*Ok() below).
// Each write to this class via WriteInfo() or WriteData() consumes another
// expected write, in the order they were added, so:
//   writer->ExpectWriteInfoOk(5, false);
//   writer->ExpectWriteDataOk(6, false);
//   writer->ExpectWriteDataOk(6, false);
// Expects these calls, in this order:
//   writer->WriteInfo(...);  // checks that |buf->response_data_size| == 5
//   writer->WriteData(...);  // checks that 6 bytes are being written
//   writer->WriteData(...);  // checks that another 6 bytes are being written
// If this class receives an unexpected call to WriteInfo() or WriteData(), it
// DCHECKs.
// Expected writes marked async do not complete synchronously, but rather return
// without running their callback and need to be completed with
// CompletePendingWrite().
// A convenience method AllExpectedWritesDone() is exposed so tests can ensure
// that all expected writes have been consumed by matching calls to WriteInfo()
// or WriteData().
class MockServiceWorkerResponseWriter : public ServiceWorkerResponseWriter {
 public:
  MockServiceWorkerResponseWriter()
      : ServiceWorkerResponseWriter(
            0,
            base::WeakPtr<AppCacheDiskCacheInterface>()),
        info_written_(0),
        data_written_(0) {}
  ~MockServiceWorkerResponseWriter() override {}

  // ServiceWorkerResponseWriter overrides
  void WriteInfo(HttpResponseInfoIOBuffer* info_buf,
                 OnceCompletionCallback callback) override;
  void WriteData(net::IOBuffer* buf,
                 int buf_len,
                 OnceCompletionCallback callback) override;

  // Enqueue expected writes.
  void ExpectWriteInfoOk(size_t len, bool async);
  void ExpectWriteInfo(size_t len, bool async, int result);
  void ExpectWriteDataOk(size_t len, bool async);
  void ExpectWriteData(size_t len, bool async, int result);

  // Complete a pending asynchronous write. This method DCHECKs unless there is
  // a pending write (a write for which WriteInfo() or WriteData() has been
  // called but the callback has not yet been run).
  void CompletePendingWrite();

  // Returns whether all expected reads have been consumed.
  bool AllExpectedWritesDone() { return expected_writes_.size() == 0; }

 private:
  struct ExpectedWrite {
    ExpectedWrite(bool is_info, size_t length, bool async, int result)
        : is_info(is_info), length(length), async(async), result(result) {}
    bool is_info;
    size_t length;
    bool async;
    int result;
  };

  base::queue<ExpectedWrite> expected_writes_;

  size_t info_written_;
  size_t data_written_;

  OnceCompletionCallback pending_callback_;

  DISALLOW_COPY_AND_ASSIGN(MockServiceWorkerResponseWriter);
};

void MockServiceWorkerResponseWriter::WriteInfo(
    HttpResponseInfoIOBuffer* info_buf,
    OnceCompletionCallback callback) {
  DCHECK(!expected_writes_.empty());
  ExpectedWrite write = expected_writes_.front();
  EXPECT_TRUE(write.is_info);
  if (write.result > 0) {
    EXPECT_EQ(write.length, static_cast<size_t>(info_buf->response_data_size));
    info_written_ += info_buf->response_data_size;
  }
  if (!write.async) {
    expected_writes_.pop();
    std::move(callback).Run(write.result);
  } else {
    pending_callback_ = std::move(callback);
  }
}

void MockServiceWorkerResponseWriter::WriteData(
    net::IOBuffer* buf,
    int buf_len,
    OnceCompletionCallback callback) {
  DCHECK(!expected_writes_.empty());
  ExpectedWrite write = expected_writes_.front();
  EXPECT_FALSE(write.is_info);
  if (write.result > 0) {
    EXPECT_EQ(write.length, static_cast<size_t>(buf_len));
    data_written_ += buf_len;
  }
  if (!write.async) {
    expected_writes_.pop();
    std::move(callback).Run(write.result);
  } else {
    pending_callback_ = std::move(callback);
  }
}

void MockServiceWorkerResponseWriter::ExpectWriteInfoOk(size_t length,
                                                        bool async) {
  ExpectWriteInfo(length, async, length);
}

void MockServiceWorkerResponseWriter::ExpectWriteDataOk(size_t length,
                                                        bool async) {
  ExpectWriteData(length, async, length);
}

void MockServiceWorkerResponseWriter::ExpectWriteInfo(size_t length,
                                                      bool async,
                                                      int result) {
  DCHECK_NE(net::ERR_IO_PENDING, result);
  ExpectedWrite expected(true, length, async, result);
  expected_writes_.push(expected);
}

void MockServiceWorkerResponseWriter::ExpectWriteData(size_t length,
                                                      bool async,
                                                      int result) {
  DCHECK_NE(net::ERR_IO_PENDING, result);
  ExpectedWrite expected(false, length, async, result);
  expected_writes_.push(expected);
}

void MockServiceWorkerResponseWriter::CompletePendingWrite() {
  DCHECK(!expected_writes_.empty());
  ExpectedWrite write = expected_writes_.front();
  DCHECK(write.async);
  expected_writes_.pop();
  std::move(pending_callback_).Run(write.result);
}

class ServiceWorkerCacheWriterTest : public ::testing::Test {
 public:
  ServiceWorkerCacheWriterTest() {}
  ~ServiceWorkerCacheWriterTest() override {}

  MockServiceWorkerResponseReader* ExpectReader() {
    std::unique_ptr<MockServiceWorkerResponseReader> reader(
        new MockServiceWorkerResponseReader);
    MockServiceWorkerResponseReader* borrowed_reader = reader.get();
    readers_.push_back(std::move(reader));
    return borrowed_reader;
  }

  MockServiceWorkerResponseWriter* ExpectWriter() {
    std::unique_ptr<MockServiceWorkerResponseWriter> writer(
        new MockServiceWorkerResponseWriter);
    MockServiceWorkerResponseWriter* borrowed_writer = writer.get();
    writers_.push_back(std::move(writer));
    return borrowed_writer;
  }

  // This should be called after ExpectReader() and ExpectWriter().
  void Initialize() {
    std::unique_ptr<ServiceWorkerResponseReader> compare_reader(CreateReader());
    std::unique_ptr<ServiceWorkerResponseReader> copy_reader(CreateReader());
    std::unique_ptr<ServiceWorkerResponseWriter> writer(CreateWriter());
    cache_writer_.reset(new ServiceWorkerCacheWriter(
        std::move(compare_reader), std::move(copy_reader), std::move(writer)));
  }

 protected:
  std::list<std::unique_ptr<MockServiceWorkerResponseReader>> readers_;
  std::list<std::unique_ptr<MockServiceWorkerResponseWriter>> writers_;
  std::unique_ptr<ServiceWorkerCacheWriter> cache_writer_;
  bool write_complete_ = false;
  net::Error last_error_;

  std::unique_ptr<ServiceWorkerResponseReader> CreateReader() {
    if (readers_.empty())
      return base::WrapUnique<ServiceWorkerResponseReader>(nullptr);
    std::unique_ptr<ServiceWorkerResponseReader> reader(
        std::move(readers_.front()));
    readers_.pop_front();
    return reader;
  }

  std::unique_ptr<ServiceWorkerResponseWriter> CreateWriter() {
    if (writers_.empty())
      return base::WrapUnique<ServiceWorkerResponseWriter>(nullptr);
    std::unique_ptr<ServiceWorkerResponseWriter> writer(
        std::move(writers_.front()));
    writers_.pop_front();
    return writer;
  }

  ServiceWorkerCacheWriter::OnWriteCompleteCallback CreateWriteCallback() {
    return base::BindOnce(&ServiceWorkerCacheWriterTest::OnWriteComplete,
                          base::Unretained(this));
  }

  void OnWriteComplete(net::Error error) {
    write_complete_ = true;
    last_error_ = error;
  }

  net::Error WriteHeaders(size_t len) {
    scoped_refptr<HttpResponseInfoIOBuffer> buf(new HttpResponseInfoIOBuffer);
    buf->response_data_size = len;
    return cache_writer_->MaybeWriteHeaders(buf.get(), CreateWriteCallback());
  }

  net::Error WriteData(const std::string& data) {
    scoped_refptr<net::IOBuffer> buf = new net::StringIOBuffer(data);
    return cache_writer_->MaybeWriteData(buf.get(), data.size(),
                                         CreateWriteCallback());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerCacheWriterTest);
};

// Passthrough tests:
// In these tests, the ServiceWorkerCacheWriter under test has no existing
// reader, since no calls to ExpectReader() have been made; this means that
// there is no existing cached response and the incoming data is written back to
// the cache directly.

TEST_F(ServiceWorkerCacheWriterTest, PassthroughHeadersSync) {
  const size_t kHeaderSize = 16;
  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  writer->ExpectWriteInfoOk(kHeaderSize, false);
  Initialize();

  net::Error error = WriteHeaders(kHeaderSize);
  EXPECT_EQ(net::OK, error);
  EXPECT_FALSE(write_complete_);
  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

TEST_F(ServiceWorkerCacheWriterTest, PassthroughHeadersAsync) {
  size_t kHeaderSize = 16;
  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  writer->ExpectWriteInfoOk(kHeaderSize, true);
  Initialize();

  net::Error error = WriteHeaders(kHeaderSize);
  EXPECT_EQ(net::ERR_IO_PENDING, error);
  EXPECT_FALSE(write_complete_);
  writer->CompletePendingWrite();
  EXPECT_TRUE(write_complete_);
  EXPECT_EQ(net::OK, last_error_);
  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

TEST_F(ServiceWorkerCacheWriterTest, PassthroughDataSync) {
  const std::string data1 = "abcdef";
  const std::string data2 = "ghijklmno";
  size_t response_size = data1.size() + data2.size();

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  writer->ExpectWriteInfoOk(response_size, false);
  writer->ExpectWriteDataOk(data1.size(), false);
  writer->ExpectWriteDataOk(data2.size(), false);
  Initialize();

  net::Error error = WriteHeaders(response_size);
  EXPECT_EQ(net::OK, error);

  error = WriteData(data1);
  EXPECT_EQ(net::OK, error);

  error = WriteData(data2);
  EXPECT_EQ(net::OK, error);
  EXPECT_TRUE(writer->AllExpectedWritesDone());
}

TEST_F(ServiceWorkerCacheWriterTest, PassthroughDataAsync) {
  const std::string data1 = "abcdef";
  const std::string data2 = "ghijklmno";
  size_t response_size = data1.size() + data2.size();

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  writer->ExpectWriteInfoOk(response_size, false);
  writer->ExpectWriteDataOk(data1.size(), true);
  writer->ExpectWriteDataOk(data2.size(), true);
  Initialize();

  net::Error error = WriteHeaders(response_size);
  EXPECT_EQ(net::OK, error);

  error = WriteData(data1);
  EXPECT_EQ(net::ERR_IO_PENDING, error);
  writer->CompletePendingWrite();
  EXPECT_TRUE(write_complete_);

  write_complete_ = false;
  error = WriteData(data2);
  EXPECT_EQ(net::ERR_IO_PENDING, error);
  writer->CompletePendingWrite();
  EXPECT_TRUE(write_complete_);
  EXPECT_EQ(net::OK, last_error_);
  EXPECT_TRUE(writer->AllExpectedWritesDone());
}

TEST_F(ServiceWorkerCacheWriterTest, PassthroughHeadersFailSync) {
  const size_t kHeaderSize = 16;
  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  writer->ExpectWriteInfo(kHeaderSize, false, net::ERR_FAILED);
  Initialize();

  net::Error error = WriteHeaders(kHeaderSize);
  EXPECT_EQ(net::ERR_FAILED, error);
  EXPECT_FALSE(write_complete_);
  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

TEST_F(ServiceWorkerCacheWriterTest, PassthroughHeadersFailAsync) {
  size_t kHeaderSize = 16;
  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  writer->ExpectWriteInfo(kHeaderSize, true, net::ERR_FAILED);
  Initialize();

  net::Error error = WriteHeaders(kHeaderSize);
  EXPECT_EQ(net::ERR_IO_PENDING, error);
  EXPECT_FALSE(write_complete_);
  writer->CompletePendingWrite();
  EXPECT_TRUE(write_complete_);
  EXPECT_EQ(net::ERR_FAILED, last_error_);
  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

TEST_F(ServiceWorkerCacheWriterTest, PassthroughDataFailSync) {
  const std::string data = "abcdef";

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  writer->ExpectWriteInfoOk(data.size(), false);
  writer->ExpectWriteData(data.size(), false, net::ERR_FAILED);
  Initialize();

  EXPECT_EQ(net::OK, WriteHeaders(data.size()));
  EXPECT_EQ(net::ERR_FAILED, WriteData(data));
  EXPECT_TRUE(writer->AllExpectedWritesDone());
}

TEST_F(ServiceWorkerCacheWriterTest, PassthroughDataFailAsync) {
  const std::string data = "abcdef";

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  writer->ExpectWriteInfoOk(data.size(), false);
  writer->ExpectWriteData(data.size(), true, net::ERR_FAILED);
  Initialize();

  EXPECT_EQ(net::OK, WriteHeaders(data.size()));

  EXPECT_EQ(net::ERR_IO_PENDING, WriteData(data));
  writer->CompletePendingWrite();
  EXPECT_EQ(net::ERR_FAILED, last_error_);
  EXPECT_TRUE(write_complete_);
  EXPECT_TRUE(writer->AllExpectedWritesDone());
}

// Comparison tests:
// For the Compare* tests below, the ServiceWorkerCacheWriter under test has a
// reader for an existing cached response, so it will compare the response being
// written to it against the existing cached response.

TEST_F(ServiceWorkerCacheWriterTest, CompareHeadersSync) {
  size_t response_size = 3;
  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  MockServiceWorkerResponseReader* reader = ExpectReader();

  reader->ExpectReadInfoOk(response_size, false);
  Initialize();

  net::Error error = WriteHeaders(response_size);
  EXPECT_EQ(net::OK, error);
  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_TRUE(reader->AllExpectedReadsDone());
}

TEST_F(ServiceWorkerCacheWriterTest, CompareDataOkSync) {
  const std::string data1 = "abcdef";
  size_t response_size = data1.size();

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  MockServiceWorkerResponseReader* reader = ExpectReader();

  reader->ExpectReadInfoOk(response_size, false);
  reader->ExpectReadDataOk(data1, false);
  Initialize();

  net::Error error = WriteHeaders(response_size);
  EXPECT_EQ(net::OK, error);

  error = WriteData(data1);
  EXPECT_EQ(net::OK, error);

  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_TRUE(reader->AllExpectedReadsDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

TEST_F(ServiceWorkerCacheWriterTest, CompareHeadersFailSync) {
  size_t response_size = 3;
  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  MockServiceWorkerResponseReader* reader = ExpectReader();

  reader->ExpectReadInfo(response_size, false, net::ERR_FAILED);
  Initialize();

  EXPECT_EQ(net::ERR_FAILED, WriteHeaders(response_size));
  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_TRUE(reader->AllExpectedReadsDone());
}

TEST_F(ServiceWorkerCacheWriterTest, CompareDataFailSync) {
  const std::string data1 = "abcdef";
  size_t response_size = data1.size();

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  MockServiceWorkerResponseReader* reader = ExpectReader();

  reader->ExpectReadInfoOk(response_size, false);
  reader->ExpectReadData(data1.c_str(), data1.length(), false, net::ERR_FAILED);
  Initialize();

  net::Error error = WriteHeaders(response_size);
  EXPECT_EQ(net::OK, error);

  EXPECT_EQ(net::ERR_FAILED, WriteData(data1));

  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_TRUE(reader->AllExpectedReadsDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

TEST_F(ServiceWorkerCacheWriterTest, CompareShortCacheReads) {
  const size_t kHeaderSize = 16;
  const std::string& data1 = "abcdef";
  const std::string& cache_data2 = "ghi";
  const std::string& cache_data3 = "j";
  const std::string& cache_data4 = "kl";
  const std::string& net_data2 = "ghijkl";
  const std::string& data5 = "mnopqrst";

  MockServiceWorkerResponseReader* reader = ExpectReader();
  reader->ExpectReadInfo(kHeaderSize, false, kHeaderSize);
  reader->ExpectReadDataOk(data1, false);
  reader->ExpectReadDataOk(cache_data2, false);
  reader->ExpectReadDataOk(cache_data3, false);
  reader->ExpectReadDataOk(cache_data4, false);
  reader->ExpectReadDataOk(data5, false);
  Initialize();

  net::Error error = WriteHeaders(kHeaderSize);
  EXPECT_EQ(net::OK, error);
  error = WriteData(data1);
  EXPECT_EQ(net::OK, error);
  error = WriteData(net_data2);
  EXPECT_EQ(net::OK, error);
  error = WriteData(data5);
  EXPECT_EQ(net::OK, error);
  EXPECT_TRUE(reader->AllExpectedReadsDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

TEST_F(ServiceWorkerCacheWriterTest, CompareDataOkAsync) {
  const std::string data1 = "abcdef";
  size_t response_size = data1.size();

  MockServiceWorkerResponseReader* reader = ExpectReader();

  reader->ExpectReadInfoOk(response_size, true);
  reader->ExpectReadDataOk(data1, true);
  Initialize();

  net::Error error = WriteHeaders(response_size);
  EXPECT_EQ(net::ERR_IO_PENDING, error);
  reader->CompletePendingRead();

  error = WriteData(data1);
  EXPECT_EQ(net::ERR_IO_PENDING, error);
  reader->CompletePendingRead();

  EXPECT_TRUE(reader->AllExpectedReadsDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

TEST_F(ServiceWorkerCacheWriterTest, CompareDataManyOkAsync) {
  const std::string expected_data[] = {
      "abcdef", "ghijkl", "mnopqr", "stuvwxyz",
  };
  size_t response_size = 0;
  for (size_t i = 0; i < arraysize(expected_data); ++i)
    response_size += expected_data[i].size();

  MockServiceWorkerResponseReader* reader = ExpectReader();

  reader->ExpectReadInfoOk(response_size, true);
  for (size_t i = 0; i < arraysize(expected_data); ++i) {
    reader->ExpectReadDataOk(expected_data[i], true);
  }
  Initialize();

  net::Error error = WriteHeaders(response_size);
  EXPECT_EQ(net::ERR_IO_PENDING, error);
  reader->CompletePendingRead();

  for (size_t i = 0; i < arraysize(expected_data); ++i) {
    error = WriteData(expected_data[i]);
    EXPECT_EQ(net::ERR_IO_PENDING, error);
    reader->CompletePendingRead();
    EXPECT_EQ(net::OK, last_error_);
  }

  EXPECT_TRUE(reader->AllExpectedReadsDone());
  EXPECT_EQ(0U, cache_writer_->bytes_written());
}

// This test writes headers and three data blocks data1, data2, data3; data2
// differs in the cached version. The writer should be asked to rewrite the
// headers and body with the new value, and the copy reader should be asked to
// read the header and data1.
TEST_F(ServiceWorkerCacheWriterTest, CompareFailedCopySync) {
  std::string data1 = "abcdef";
  std::string cache_data2 = "ghijkl";
  std::string net_data2 = "mnopqr";
  std::string data3 = "stuvwxyz";
  size_t cache_response_size = data1.size() + cache_data2.size() + data3.size();
  size_t net_response_size = data1.size() + net_data2.size() + data3.size();

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  MockServiceWorkerResponseReader* compare_reader = ExpectReader();
  MockServiceWorkerResponseReader* copy_reader = ExpectReader();

  compare_reader->ExpectReadInfoOk(cache_response_size, false);
  compare_reader->ExpectReadDataOk(data1, false);
  compare_reader->ExpectReadDataOk(cache_data2, false);

  copy_reader->ExpectReadInfoOk(cache_response_size, false);
  copy_reader->ExpectReadDataOk(data1, false);

  writer->ExpectWriteInfoOk(net_response_size, false);
  writer->ExpectWriteDataOk(data1.size(), false);
  writer->ExpectWriteDataOk(net_data2.size(), false);
  writer->ExpectWriteDataOk(data3.size(), false);

  Initialize();

  net::Error error = WriteHeaders(net_response_size);
  EXPECT_EQ(net::OK, error);
  error = WriteData(data1);
  EXPECT_EQ(net::OK, error);
  error = WriteData(net_data2);
  EXPECT_EQ(net::OK, error);
  error = WriteData(data3);
  EXPECT_EQ(net::OK, error);

  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_TRUE(compare_reader->AllExpectedReadsDone());
  EXPECT_TRUE(copy_reader->AllExpectedReadsDone());
}

// Tests behavior when the cached data is shorter than the network data.
TEST_F(ServiceWorkerCacheWriterTest, CompareFailedCopyShort) {
  std::string data1 = "abcdef";
  std::string cache_data2 = "mnop";
  std::string net_data2 = "mnopqr";
  std::string data3 = "stuvwxyz";
  size_t cache_response_size = data1.size() + cache_data2.size() + data3.size();
  size_t net_response_size = data1.size() + net_data2.size() + data3.size();

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  MockServiceWorkerResponseReader* compare_reader = ExpectReader();
  MockServiceWorkerResponseReader* copy_reader = ExpectReader();

  compare_reader->ExpectReadInfoOk(cache_response_size, false);
  compare_reader->ExpectReadDataOk(data1, false);
  compare_reader->ExpectReadDataOk(cache_data2, false);
  compare_reader->ExpectReadDataOk("", false);  // EOF read

  copy_reader->ExpectReadInfoOk(cache_response_size, false);
  copy_reader->ExpectReadDataOk(data1, false);

  writer->ExpectWriteInfoOk(net_response_size, false);
  writer->ExpectWriteDataOk(data1.size(), false);
  writer->ExpectWriteDataOk(net_data2.size(), false);
  writer->ExpectWriteDataOk(data3.size(), false);

  Initialize();

  net::Error error = WriteHeaders(net_response_size);
  EXPECT_EQ(net::OK, error);
  error = WriteData(data1);
  EXPECT_EQ(net::OK, error);
  error = WriteData(net_data2);
  EXPECT_EQ(net::OK, error);
  error = WriteData(data3);
  EXPECT_EQ(net::OK, error);

  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_TRUE(compare_reader->AllExpectedReadsDone());
  EXPECT_TRUE(copy_reader->AllExpectedReadsDone());
}

// Tests behavior when the cached data is longer than the network data.
TEST_F(ServiceWorkerCacheWriterTest, CompareFailedCopyLong) {
  std::string data1 = "abcdef";
  std::string cache_data2 = "mnop";
  std::string net_data2 = "mnop";
  std::string cache_data3 = "qr";
  size_t cached_size = data1.size() + cache_data2.size() + cache_data3.size();
  size_t net_size = data1.size() + net_data2.size();

  MockServiceWorkerResponseWriter* writer = ExpectWriter();
  MockServiceWorkerResponseReader* compare_reader = ExpectReader();
  MockServiceWorkerResponseReader* copy_reader = ExpectReader();

  compare_reader->ExpectReadInfoOk(cached_size, false);
  compare_reader->ExpectReadDataOk(data1, false);
  compare_reader->ExpectReadDataOk(cache_data2, false);

  // The comparison should fail at the end of |cache_data2|, when the cache
  // writer realizes the two responses are different sizes, and then the network
  // data should be written back starting with |net_data2|.
  copy_reader->ExpectReadInfoOk(cached_size, false);
  copy_reader->ExpectReadDataOk(data1, false);
  copy_reader->ExpectReadDataOk(net_data2, false);

  writer->ExpectWriteInfoOk(net_size, false);
  writer->ExpectWriteDataOk(data1.size(), false);
  writer->ExpectWriteDataOk(net_data2.size(), false);

  Initialize();

  net::Error error = WriteHeaders(net_size);
  EXPECT_EQ(net::OK, error);
  error = WriteData(data1);
  EXPECT_EQ(net::OK, error);
  error = WriteData(net_data2);
  EXPECT_EQ(net::OK, error);
  error = WriteData("");
  EXPECT_EQ(net::OK, error);

  EXPECT_TRUE(writer->AllExpectedWritesDone());
  EXPECT_TRUE(compare_reader->AllExpectedReadsDone());
  EXPECT_TRUE(copy_reader->AllExpectedReadsDone());
}

}  // namespace
}  // namespace content
