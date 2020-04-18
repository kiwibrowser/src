// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/fileapi/webfilewriter_base.h"

#include <stdint.h>

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_file_error.h"
#include "third_party/blink/public/platform/web_file_writer_client.h"
#include "third_party/blink/public/platform/web_url.h"
#include "url/gurl.h"

namespace content {

namespace {

// We use particular offsets to trigger particular behaviors
// in the TestableFileWriter.
const int kNoOffset = -1;
const int kBasicFileTruncate_Offset = 1;
const int kErrorFileTruncate_Offset = 2;
const int kCancelFileTruncate_Offset = 3;
const int kCancelFailedTruncate_Offset = 4;
const int kBasicFileWrite_Offset = 1;
const int kErrorFileWrite_Offset = 2;
const int kMultiFileWrite_Offset = 3;
const int kCancelFileWriteBeforeCompletion_Offset = 4;
const int kCancelFileWriteAfterCompletion_Offset = 5;

GURL mock_path_as_gurl() {
  return GURL("MockPath");
}

}  // namespace

class TestableFileWriter : public WebFileWriterBase {
 public:
  explicit TestableFileWriter(blink::WebFileWriterClient* client)
      : WebFileWriterBase(mock_path_as_gurl(), client) {
    reset();
  }

  void reset() {
    received_truncate_ = false;
    received_truncate_path_ = GURL();
    received_truncate_offset_ = kNoOffset;
    received_write_ = false;
    received_write_path_ = GURL();
    received_write_offset_ = kNoOffset;
    received_write_blob_uuid_ = std::string();
    received_cancel_ = false;
  }

  bool received_truncate_;
  GURL received_truncate_path_;
  int64_t received_truncate_offset_;
  bool received_write_;
  GURL received_write_path_;
  std::string received_write_blob_uuid_;
  int64_t received_write_offset_;
  bool received_cancel_;

 protected:
  void DoTruncate(const GURL& path, int64_t offset) override {
    received_truncate_ = true;
    received_truncate_path_ = path;
    received_truncate_offset_ = offset;

    if (offset == kBasicFileTruncate_Offset) {
      DidSucceed();
    } else if (offset == kErrorFileTruncate_Offset) {
      DidFail(base::File::FILE_ERROR_NOT_FOUND);
    } else if (offset == kCancelFileTruncate_Offset) {
      Cancel();
      DidSucceed();  // truncate completion
      DidSucceed();  // cancel completion
    } else if (offset == kCancelFailedTruncate_Offset) {
      Cancel();
      DidFail(base::File::FILE_ERROR_NOT_FOUND);  // truncate completion
      DidSucceed();  // cancel completion
    } else {
      FAIL();
    }
  }

  void DoWrite(const GURL& path,
               const std::string& blob_uuid,
               int64_t offset) override {
    received_write_ = true;
    received_write_path_ = path;
    received_write_offset_ = offset;
    received_write_blob_uuid_ = blob_uuid;

    if (offset == kBasicFileWrite_Offset) {
      DidWrite(1, true);
    } else if (offset == kErrorFileWrite_Offset) {
      DidFail(base::File::FILE_ERROR_NOT_FOUND);
    } else if (offset == kMultiFileWrite_Offset) {
      DidWrite(1, false);
      DidWrite(1, false);
      DidWrite(1, true);
    } else if (offset == kCancelFileWriteBeforeCompletion_Offset) {
      DidWrite(1, false);
      Cancel();
      DidWrite(1, false);
      DidWrite(1, false);
      DidFail(base::File::FILE_ERROR_FAILED);  // write completion
      DidSucceed();  // cancel completion
    } else if (offset == kCancelFileWriteAfterCompletion_Offset) {
      DidWrite(1, false);
      Cancel();
      DidWrite(1, false);
      DidWrite(1, false);
      DidWrite(1, true);  // write completion
      DidFail(base::File::FILE_ERROR_FAILED);  // cancel completion
    } else {
      FAIL();
    }
  }

  void DoCancel() override { received_cancel_ = true; }
};

class FileWriterTest : public testing::Test,
                       public blink::WebFileWriterClient {
 public:
  FileWriterTest() {
    reset();
  }

  blink::WebFileWriter* writer() {
    return testable_writer_.get();
  }

  // WebFileWriterClient overrides
  void DidWrite(long long bytes, bool complete) override {
    EXPECT_FALSE(received_did_write_complete_);
    ++received_did_write_count_;
    received_did_write_bytes_total_ += bytes;
    if (complete)
      received_did_write_complete_ = true;

    if (delete_in_client_callback_)
      testable_writer_.reset(nullptr);
  }

  void DidTruncate() override {
    EXPECT_FALSE(received_did_truncate_);
    received_did_truncate_ = true;
    if (delete_in_client_callback_)
      testable_writer_.reset(nullptr);
  }

  void DidFail(blink::WebFileError error) override {
    EXPECT_FALSE(received_did_fail_);
    received_did_fail_ = true;
    fail_error_received_ = error;
    if (delete_in_client_callback_)
      testable_writer_.reset(nullptr);
  }

 protected:
  void reset() {
    testable_writer_.reset(new TestableFileWriter(this));
    delete_in_client_callback_ = false;
    received_did_write_count_ = 0;
    received_did_write_bytes_total_ = 0;
    received_did_write_complete_ = false;
    received_did_truncate_ = false;
    received_did_fail_ = false;
    fail_error_received_ = static_cast<blink::WebFileError>(0);
  }

  std::unique_ptr<TestableFileWriter> testable_writer_;
  bool delete_in_client_callback_;

  // Observed WebFileWriterClient artifacts.
  int received_did_write_count_;
  long long received_did_write_bytes_total_;
  bool received_did_write_complete_;
  bool received_did_truncate_;
  bool received_did_fail_;
  blink::WebFileError fail_error_received_;

  DISALLOW_COPY_AND_ASSIGN(FileWriterTest);
};

TEST_F(FileWriterTest, BasicFileWrite) {
  // Call the webkit facing api.
  const std::string kBlobId("1234");
  writer()->Write(kBasicFileWrite_Offset, blink::WebString::FromUTF8(kBlobId));

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_write_);
  EXPECT_EQ(testable_writer_->received_write_path_,
            mock_path_as_gurl());
  EXPECT_EQ(kBasicFileWrite_Offset,
            testable_writer_->received_write_offset_);
  EXPECT_EQ(kBlobId, testable_writer_->received_write_blob_uuid_);
  EXPECT_FALSE(testable_writer_->received_truncate_);
  EXPECT_FALSE(testable_writer_->received_cancel_);

  // Check that the client gets called correctly.
  EXPECT_EQ(1, received_did_write_count_);
  EXPECT_TRUE(received_did_write_complete_);
  EXPECT_EQ(1, received_did_write_bytes_total_);
  EXPECT_FALSE(received_did_truncate_);
  EXPECT_FALSE(received_did_fail_);
}

TEST_F(FileWriterTest, BasicFileTruncate) {
  // Call the webkit facing api.
  writer()->Truncate(kBasicFileTruncate_Offset);

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_truncate_);
  EXPECT_EQ(mock_path_as_gurl(),
            testable_writer_->received_truncate_path_);
  EXPECT_EQ(kBasicFileTruncate_Offset,
            testable_writer_->received_truncate_offset_);
  EXPECT_FALSE(testable_writer_->received_write_);
  EXPECT_FALSE(testable_writer_->received_cancel_);

  // Check that the client gets called correctly.
  EXPECT_TRUE(received_did_truncate_);
  EXPECT_EQ(0, received_did_write_count_);
  EXPECT_FALSE(received_did_fail_);
}

TEST_F(FileWriterTest, ErrorFileWrite) {
  // Call the webkit facing api.
  const std::string kBlobId("1234");
  writer()->Write(kErrorFileWrite_Offset, blink::WebString::FromUTF8(kBlobId));

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_write_);
  EXPECT_EQ(testable_writer_->received_write_path_,
            mock_path_as_gurl());
  EXPECT_EQ(kErrorFileWrite_Offset,
            testable_writer_->received_write_offset_);
  EXPECT_EQ(kBlobId, testable_writer_->received_write_blob_uuid_);
  EXPECT_FALSE(testable_writer_->received_truncate_);
  EXPECT_FALSE(testable_writer_->received_cancel_);

  // Check that the client gets called correctly.
  EXPECT_TRUE(received_did_fail_);
  EXPECT_EQ(blink::kWebFileErrorNotFound, fail_error_received_);
  EXPECT_EQ(0, received_did_write_count_);
  EXPECT_FALSE(received_did_truncate_);
}

TEST_F(FileWriterTest, ErrorFileTruncate) {
  // Call the webkit facing api.
  writer()->Truncate(kErrorFileTruncate_Offset);

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_truncate_);
  EXPECT_EQ(mock_path_as_gurl(),
            testable_writer_->received_truncate_path_);
  EXPECT_EQ(kErrorFileTruncate_Offset,
            testable_writer_->received_truncate_offset_);
  EXPECT_FALSE(testable_writer_->received_write_);
  EXPECT_FALSE(testable_writer_->received_cancel_);

  // Check that the client gets called correctly.
  EXPECT_TRUE(received_did_fail_);
  EXPECT_EQ(blink::kWebFileErrorNotFound, fail_error_received_);
  EXPECT_FALSE(received_did_truncate_);
  EXPECT_EQ(0, received_did_write_count_);
}

TEST_F(FileWriterTest, MultiFileWrite) {
  // Call the webkit facing api.
  const std::string kBlobId("1234");
  writer()->Write(kMultiFileWrite_Offset, blink::WebString::FromUTF8(kBlobId));

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_write_);
  EXPECT_EQ(testable_writer_->received_write_path_,
            mock_path_as_gurl());
  EXPECT_EQ(kMultiFileWrite_Offset,
            testable_writer_->received_write_offset_);
  EXPECT_EQ(kBlobId, testable_writer_->received_write_blob_uuid_);
  EXPECT_FALSE(testable_writer_->received_truncate_);
  EXPECT_FALSE(testable_writer_->received_cancel_);

  // Check that the client gets called correctly.
  EXPECT_EQ(3, received_did_write_count_);
  EXPECT_TRUE(received_did_write_complete_);
  EXPECT_EQ(3, received_did_write_bytes_total_);
  EXPECT_FALSE(received_did_truncate_);
  EXPECT_FALSE(received_did_fail_);
}

TEST_F(FileWriterTest, CancelFileWriteBeforeCompletion) {
  // Call the webkit facing api.
  const std::string kBlobId("1234");
  writer()->Write(kCancelFileWriteBeforeCompletion_Offset,
                  blink::WebString::FromUTF8(kBlobId));

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_write_);
  EXPECT_EQ(testable_writer_->received_write_path_,
            mock_path_as_gurl());
  EXPECT_EQ(kCancelFileWriteBeforeCompletion_Offset,
            testable_writer_->received_write_offset_);
  EXPECT_EQ(kBlobId, testable_writer_->received_write_blob_uuid_);
  EXPECT_TRUE(testable_writer_->received_cancel_);
  EXPECT_FALSE(testable_writer_->received_truncate_);

  // Check that the client gets called correctly.
  EXPECT_TRUE(received_did_fail_);
  EXPECT_EQ(blink::kWebFileErrorAbort, fail_error_received_);
  EXPECT_EQ(1, received_did_write_count_);
  EXPECT_FALSE(received_did_write_complete_);
  EXPECT_EQ(1, received_did_write_bytes_total_);
  EXPECT_FALSE(received_did_truncate_);
}

TEST_F(FileWriterTest, CancelFileWriteAfterCompletion) {
  // Call the webkit facing api.
  const std::string kBlobId("1234");
  writer()->Write(kCancelFileWriteAfterCompletion_Offset,
                  blink::WebString::FromUTF8(kBlobId));

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_write_);
  EXPECT_EQ(testable_writer_->received_write_path_,
            mock_path_as_gurl());
  EXPECT_EQ(kCancelFileWriteAfterCompletion_Offset,
            testable_writer_->received_write_offset_);
  EXPECT_EQ(kBlobId, testable_writer_->received_write_blob_uuid_);
  EXPECT_TRUE(testable_writer_->received_cancel_);
  EXPECT_FALSE(testable_writer_->received_truncate_);

  // Check that the client gets called correctly.
  EXPECT_TRUE(received_did_fail_);
  EXPECT_EQ(blink::kWebFileErrorAbort, fail_error_received_);
  EXPECT_EQ(1, received_did_write_count_);
  EXPECT_FALSE(received_did_write_complete_);
  EXPECT_EQ(1, received_did_write_bytes_total_);
  EXPECT_FALSE(received_did_truncate_);
}

TEST_F(FileWriterTest, CancelFileTruncate) {
  // Call the webkit facing api.
  writer()->Truncate(kCancelFileTruncate_Offset);

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_truncate_);
  EXPECT_EQ(mock_path_as_gurl(),
            testable_writer_->received_truncate_path_);
  EXPECT_EQ(kCancelFileTruncate_Offset,
            testable_writer_->received_truncate_offset_);
  EXPECT_TRUE(testable_writer_->received_cancel_);
  EXPECT_FALSE(testable_writer_->received_write_);

  // Check that the client gets called correctly.
  EXPECT_TRUE(received_did_fail_);
  EXPECT_EQ(blink::kWebFileErrorAbort, fail_error_received_);
  EXPECT_FALSE(received_did_truncate_);
  EXPECT_EQ(0, received_did_write_count_);
}

TEST_F(FileWriterTest, CancelFailedTruncate) {
  // Call the webkit facing api.
  writer()->Truncate(kCancelFailedTruncate_Offset);

  // Check that the derived class gets called correctly.
  EXPECT_TRUE(testable_writer_->received_truncate_);
  EXPECT_EQ(mock_path_as_gurl(),
            testable_writer_->received_truncate_path_);
  EXPECT_EQ(kCancelFailedTruncate_Offset,
            testable_writer_->received_truncate_offset_);
  EXPECT_TRUE(testable_writer_->received_cancel_);
  EXPECT_FALSE(testable_writer_->received_write_);

  // Check that the client gets called correctly.
  EXPECT_TRUE(received_did_fail_);
  EXPECT_EQ(blink::kWebFileErrorAbort, fail_error_received_);
  EXPECT_FALSE(received_did_truncate_);
  EXPECT_EQ(0, received_did_write_count_);
}

TEST_F(FileWriterTest, DeleteInCompletionCallbacks) {
  const std::string kBlobId("1234");
  delete_in_client_callback_ = true;
  writer()->Write(kBasicFileWrite_Offset, blink::WebString::FromUTF8(kBlobId));
  EXPECT_FALSE(testable_writer_.get());

  reset();
  delete_in_client_callback_ = true;
  writer()->Truncate(kBasicFileTruncate_Offset);
  EXPECT_FALSE(testable_writer_.get());

  reset();
  delete_in_client_callback_ = true;
  writer()->Write(kErrorFileWrite_Offset, blink::WebString::FromUTF8(kBlobId));
  EXPECT_FALSE(testable_writer_.get());

  reset();
  delete_in_client_callback_ = true;
  writer()->Truncate(kErrorFileTruncate_Offset);
  EXPECT_FALSE(testable_writer_.get());

  // Not crashing counts as passing.
}

}  // namespace content
