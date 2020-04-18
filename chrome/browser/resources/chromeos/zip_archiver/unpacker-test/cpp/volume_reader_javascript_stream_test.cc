// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "volume_reader_javascript_stream.h"

#include <limits>
#include <string>

#include "native_client_sdk/src/libraries/ppapi_simple/ps_main.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "testing/gtest/gtest.h"

// Fake JavaScriptRequestor that responds to
// VolumeReaderJavaScriptStream::Read.
class FakeJavaScriptRequestor : public JavaScriptRequestorInterface {
 public:
  explicit FakeJavaScriptRequestor(pp::InstanceHandle instance_handle)
      : volume_reader_(NULL),
        array_buffer_(50),
        worker_(instance_handle),
        callback_factory_(this),
        force_failure_(false) {
    void* data = array_buffer_.Map();
    memset(data, 1, array_buffer_.ByteLength());
    array_buffer_.Unmap();
  }

  virtual ~FakeJavaScriptRequestor() { worker_.Join(); }

  bool Init() { return worker_.Start(); }

  void RequestFileChunk(const std::string& request_id,
                        int64_t offset,
                        int64_t bytes_to_read) {
    worker_.message_loop().PostWork(callback_factory_.NewCallback(
        &FakeJavaScriptRequestor::RequestFileChunkCallback, offset,
        bytes_to_read));
  }

  void RequestPassphrase(const std::string& request_id) {
    worker_.message_loop().PostWork(callback_factory_.NewCallback(
        &FakeJavaScriptRequestor::RequestPassphraseCallback));
  }

  void SetVolumeReader(VolumeReaderJavaScriptStream* volume_reader) {
    volume_reader_ = volume_reader;
  }

  void set_force_failure(bool force_failure) { force_failure_ = force_failure; }

  pp::VarArrayBuffer array_buffer() const { return array_buffer_; }

 private:
  void RequestFileChunkCallback(int32_t /*result*/,
                                int64_t offset,
                                int64_t bytes_to_read) {
    if (offset < 0 || force_failure_) {
      volume_reader_->ReadErrorSignal();
      return;
    }

    int64_t array_buffer_size = array_buffer_.ByteLength();
    if (offset >= array_buffer_size) {
      // Nothing left to read so return empty buffer.
      volume_reader_->SetBufferAndSignal(pp::VarArrayBuffer(0), offset);
    } else {
      // We checked above for negative offsets and offsets > array buffer size
      // so this should be safe.
      int64_t left_size = array_buffer_size - static_cast<int64_t>(offset);
      pp::VarArrayBuffer buffer_to_set(left_size);

      char* data = static_cast<char*>(buffer_to_set.Map());
      char* array_buffer_data = static_cast<char*>(array_buffer_.Map());
      memcpy(data, array_buffer_data + offset, left_size);
      buffer_to_set.Unmap();
      array_buffer_.Unmap();

      volume_reader_->SetBufferAndSignal(buffer_to_set, offset);
    }
  }

  void RequestPassphraseCallback(int32_t /*result*/) {
    if (force_failure_) {
      volume_reader_->PassphraseErrorSignal();
      return;
    }

    // TODO(mtomasz): Improve test coverage for passphrases.
    volume_reader_->SetPassphraseAndSignal("");
  }

  VolumeReaderJavaScriptStream* volume_reader_;
  // Content can be junk. Not important as long as buffer has size > 0. See
  // constructor.
  pp::VarArrayBuffer array_buffer_;

  // Worker to execute requests on a different thread. See
  // VolumeReaderJavaScriptStream method comments.
  pp::SimpleThread worker_;
  pp::CompletionCallbackFactory<FakeJavaScriptRequestor> callback_factory_;

  bool force_failure_;
};

// Class used by TEST_F macro to initialize the environment for testing
// VolumeReaderJavaScriptStream methods.
class VolumeReaderJavaScriptStreamTest : public testing::Test {
 protected:
  VolumeReaderJavaScriptStreamTest()
      : kArchiveSize(std::numeric_limits<int64_t>::max() - 100),
        fake_javascript_requestor(NULL),
        volume_reader(NULL),
        instance_handle(PSGetInstanceId()) {}

  virtual void SetUp() {
    fake_javascript_requestor = new FakeJavaScriptRequestor(instance_handle);
    ASSERT_TRUE(fake_javascript_requestor->Init());

    volume_reader = new VolumeReaderJavaScriptStream(kArchiveSize,
                                                     fake_javascript_requestor);
    fake_javascript_requestor->SetVolumeReader(volume_reader);
    ASSERT_EQ(0, volume_reader->offset());
  }

  virtual void TearDown() {
    delete volume_reader;
    volume_reader = NULL;
    delete fake_javascript_requestor;
    fake_javascript_requestor = NULL;
  }

  // The volume's archive size. Used to test values greater than int32_t.
  const int64_t kArchiveSize;
  FakeJavaScriptRequestor* fake_javascript_requestor;
  VolumeReaderJavaScriptStream* volume_reader;
  pp::InstanceHandle instance_handle;
};

TEST_F(VolumeReaderJavaScriptStreamTest, SmallSkip) {
  // Skip with value smaller than int32_t.
  EXPECT_EQ(1, volume_reader->Skip(1));
  EXPECT_EQ(1, volume_reader->offset());
}

TEST_F(VolumeReaderJavaScriptStreamTest, BigSkip) {
  // Skip with value greater than int32_t but less than archive size.
  int64_t bigBytesToSkipNum = kArchiveSize - 50;  // See kArchiveSize value.
  EXPECT_EQ(bigBytesToSkipNum, volume_reader->Skip(bigBytesToSkipNum));
  EXPECT_EQ(bigBytesToSkipNum, volume_reader->offset());
}

// If read_ahead_length has an invalid length then Skip will return 0.
TEST_F(VolumeReaderJavaScriptStreamTest, ZeroSkip) {
  // Skip with value greater than archive size but less han int64_t max value.
  int64_t veryBigBytesToSkipNum = kArchiveSize + 50;  // See kArchiveSize value.
  EXPECT_EQ(0, volume_reader->Skip(veryBigBytesToSkipNum));
  EXPECT_EQ(0, volume_reader->offset());

  // Can't skip backwards.
  int64_t invalidSkipNum = -1;  // See kArchiveSize value.
  EXPECT_EQ(0, volume_reader->Skip(invalidSkipNum));
  EXPECT_EQ(0, volume_reader->offset());
}

TEST_F(VolumeReaderJavaScriptStreamTest, Seek) {
  // Seek from start.
  EXPECT_EQ(10, volume_reader->Seek(10, SEEK_SET));
  EXPECT_EQ(10, volume_reader->offset());

  // Seek from current with positive value.
  EXPECT_EQ(15, volume_reader->Seek(5, SEEK_CUR));
  EXPECT_EQ(15, volume_reader->offset());

  // Seek from current with negative value.
  EXPECT_EQ(5, volume_reader->Seek(-10, SEEK_CUR));
  EXPECT_EQ(5, volume_reader->offset());

  // Seek from current with value greater than int32_t.
  int64_t positiveSkipValue = kArchiveSize - 50;  // See kArchiveSize value.
  EXPECT_EQ(positiveSkipValue + 5 /* +5 from last Seek call. */,
            volume_reader->Seek(positiveSkipValue, SEEK_CUR));
  EXPECT_EQ(positiveSkipValue + 5, volume_reader->offset());

  // Seek from current with value smaller than int32_t.
  int64_t negativeSkipValue = -positiveSkipValue;
  EXPECT_EQ(5, volume_reader->Seek(negativeSkipValue, SEEK_CUR));
  EXPECT_EQ(5, volume_reader->offset());

  // Seek from start with value greater than int32_t.
  EXPECT_EQ(positiveSkipValue,
            volume_reader->Seek(positiveSkipValue, SEEK_SET));
  EXPECT_EQ(positiveSkipValue, volume_reader->offset());

  // Seek from end. SEEK_END requires negative values.
  EXPECT_EQ(kArchiveSize - 5, volume_reader->Seek(-5, SEEK_END));
  EXPECT_EQ(kArchiveSize - 5, volume_reader->offset());

  // Seek from end with value smaller than int32_t.
  int64_t expectedOffset = kArchiveSize + negativeSkipValue;
  EXPECT_EQ(expectedOffset, volume_reader->Seek(negativeSkipValue, SEEK_END));
  EXPECT_EQ(expectedOffset, volume_reader->offset());

  // Seek from current with 0.
  EXPECT_EQ(expectedOffset, volume_reader->Seek(0, SEEK_CUR));
  EXPECT_EQ(expectedOffset, volume_reader->offset());

  // Seek from start with 0.
  EXPECT_EQ(0, volume_reader->Seek(0, SEEK_SET));
  EXPECT_EQ(0, volume_reader->offset());

  // Seek from end with 0.
  EXPECT_EQ(kArchiveSize, volume_reader->Seek(0, SEEK_END));
  EXPECT_EQ(kArchiveSize, volume_reader->offset());
}

TEST_F(VolumeReaderJavaScriptStreamTest, InvalidSeekNegativeOffsetResult) {
  // Seek that results in negative offsets is invalid.
  EXPECT_EQ(ARCHIVE_FATAL, volume_reader->Seek(-1, SEEK_SET));
}

TEST_F(VolumeReaderJavaScriptStreamTest,
       InvalidSeekOffsetLargerThanArchiveSize) {
  // Seek that results in offsets larger than archive size is invalid.
  EXPECT_EQ(ARCHIVE_FATAL, volume_reader->Seek(1, SEEK_END));
}

TEST_F(VolumeReaderJavaScriptStreamTest, Read) {
  // Valid read with bytes to read = arrayi buffer size.
  int64_t array_buffer_size =
      fake_javascript_requestor->array_buffer().ByteLength();
  const void* buffer = NULL;
  int64_t read_bytes = volume_reader->Read(array_buffer_size, &buffer);
  ASSERT_GE(array_buffer_size, read_bytes);  // Read can return less bytes
                                             // than required.
  ASSERT_GE(read_bytes, 0);

  const void* expected_buffer = fake_javascript_requestor->array_buffer().Map();
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, read_bytes));
  fake_javascript_requestor->array_buffer().Unmap();
}

TEST_F(VolumeReaderJavaScriptStreamTest, BigLengthRead) {
  // Valid read with bytes to read > array buffer size.
  int64_t array_buffer_size =
      fake_javascript_requestor->array_buffer().ByteLength();
  const void* buffer = NULL;
  int64_t read_bytes =
      volume_reader->Read(std::numeric_limits<int64_t>::max(), &buffer);

  // Though the request was for more than array_buffer_size, Read returns
  // maximum array_buffer_size as the buffer is set to array_buffer.
  ASSERT_GE(array_buffer_size, read_bytes);
  ASSERT_GE(read_bytes, 0);

  const void* expected_buffer = fake_javascript_requestor->array_buffer().Map();
  EXPECT_EQ(0, memcmp(buffer, expected_buffer, read_bytes));
  fake_javascript_requestor->array_buffer().Unmap();
}

TEST_F(VolumeReaderJavaScriptStreamTest, SmallReads) {
  // Test multiple reads with bytes to read < array buffer size.

  // First read.
  int64_t array_buffer_size =
      fake_javascript_requestor->array_buffer().ByteLength();
  const void* buffer_1 = NULL;
  int64_t bytes_to_read_1 = array_buffer_size / 4;
  int64_t read_bytes_1 = volume_reader->Read(bytes_to_read_1, &buffer_1);
  ASSERT_GE(bytes_to_read_1, read_bytes_1);
  ASSERT_GE(read_bytes_1, 0);

  const void* expected_buffer_1 =
      fake_javascript_requestor->array_buffer().Map();
  EXPECT_EQ(0, memcmp(buffer_1, expected_buffer_1, read_bytes_1));
  fake_javascript_requestor->array_buffer().Unmap();

  // Second read.
  int64_t bytes_to_read_2 = bytes_to_read_1 * 3;
  const void* buffer_2 = NULL;
  int64_t read_bytes_2 = volume_reader->Read(bytes_to_read_2, &buffer_2);
  ASSERT_GE(bytes_to_read_2, read_bytes_2);
  ASSERT_GE(read_bytes_2, 0);
  ASSERT_GE(array_buffer_size, read_bytes_1 + read_bytes_2);

  const void* expected_buffer_2 =
      static_cast<char*>(fake_javascript_requestor->array_buffer().Map()) +
      read_bytes_1;
  EXPECT_EQ(0, memcmp(buffer_2, expected_buffer_2, read_bytes_2));
  fake_javascript_requestor->array_buffer().Unmap();

  // Third read.
  int64_t bytes_to_read_3 = bytes_to_read_2;
  const void* buffer_3 = NULL;
  int64_t read_bytes_3 = volume_reader->Read(bytes_to_read_3, &buffer_3);
  ASSERT_GE(bytes_to_read_3, read_bytes_3);
  ASSERT_GE(read_bytes_3, 0);
  ASSERT_GE(array_buffer_size, read_bytes_1 + read_bytes_2 + read_bytes_3);

  const void* expected_buffer_3 =
      static_cast<char*>(fake_javascript_requestor->array_buffer().Map()) +
      read_bytes_1 + read_bytes_2;
  EXPECT_EQ(0, memcmp(buffer_3, expected_buffer_3, read_bytes_3));
  fake_javascript_requestor->array_buffer().Unmap();
}

TEST_F(VolumeReaderJavaScriptStreamTest, EndOfArchiveRead) {
  // Read at the end of archive.
  volume_reader->Seek(0, SEEK_END);
  const void* buffer = NULL;
  EXPECT_EQ(0, volume_reader->Read(1, &buffer));
}

TEST_F(VolumeReaderJavaScriptStreamTest, InvalidRead) {
  // Force read ahead with Seek after setting force failure to true. This is
  // necessary because the constructor read ahead will be successful if it
  // finishes before we do set_force_failure.
  fake_javascript_requestor->set_force_failure(true);
  volume_reader->Seek(0, SEEK_SET);
  int64_t bytes_to_read =
      fake_javascript_requestor->array_buffer().ByteLength();
  const void* buffer = NULL;
  EXPECT_EQ(ARCHIVE_FATAL, volume_reader->Read(bytes_to_read, &buffer));
}
