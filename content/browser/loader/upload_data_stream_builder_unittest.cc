// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/upload_data_stream_builder.h"

#include <stdint.h>

#include <algorithm>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_file_element_reader.h"
#include "net/log/net_log_with_source.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/upload_blob_element_reader.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using storage::BlobDataBuilder;
using storage::BlobDataHandle;
using storage::BlobStorageContext;

namespace content {

TEST(UploadDataStreamBuilderTest, CreateUploadDataStream) {
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  {
    scoped_refptr<network::ResourceRequestBody> request_body =
        new network::ResourceRequestBody;

    const std::string kBlob = "blobuuid";
    const std::string kBlobData = "blobdata";
    const char kData[] = "123";
    const base::FilePath::StringType kFilePath = FILE_PATH_LITERAL("abc");
    const uint64_t kFileOffset = 10U;
    const uint64_t kFileLength = 100U;
    const base::Time kFileTime = base::Time::FromDoubleT(999);
    const int64_t kIdentifier = 12345;

    BlobStorageContext context;
    auto builder = std::make_unique<BlobDataBuilder>(kBlob);
    builder->AppendData(kBlobData);
    std::unique_ptr<BlobDataHandle> handle =
        context.AddFinishedBlob(std::move(builder));

    request_body->AppendBytes(kData, arraysize(kData) - 1);
    request_body->AppendFileRange(base::FilePath(kFilePath), kFileOffset,
                                  kFileLength, kFileTime);
    request_body->AppendBlob(kBlob);
    request_body->set_identifier(kIdentifier);

    std::unique_ptr<net::UploadDataStream> upload(
        UploadDataStreamBuilder::Build(
            request_body.get(), &context, nullptr,
            base::ThreadTaskRunnerHandle::Get().get()));

    EXPECT_EQ(kIdentifier, upload->identifier());
    ASSERT_TRUE(upload->GetElementReaders());
    ASSERT_EQ(request_body->elements()->size(),
              upload->GetElementReaders()->size());

    const net::UploadBytesElementReader* r1 =
        (*upload->GetElementReaders())[0]->AsBytesReader();
    ASSERT_TRUE(r1);
    EXPECT_EQ(kData, std::string(r1->bytes(), r1->length()));

    const net::UploadFileElementReader* r2 =
        (*upload->GetElementReaders())[1]->AsFileReader();
    ASSERT_TRUE(r2);
    EXPECT_EQ(kFilePath, r2->path().value());
    EXPECT_EQ(kFileOffset, r2->range_offset());
    EXPECT_EQ(kFileLength, r2->range_length());
    EXPECT_EQ(kFileTime, r2->expected_modification_time());

    const storage::UploadBlobElementReader* r3 =
        static_cast<storage::UploadBlobElementReader*>(
            (*upload->GetElementReaders())[2].get());
    ASSERT_TRUE(r3);
    EXPECT_EQ("blobuuid", r3->uuid());
  }
  // Clean up for ASAN.
  base::RunLoop().RunUntilIdle();
}

TEST(UploadDataStreamBuilderTest,
     WriteUploadDataStreamWithEmptyFileBackedBlob) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);
  {
    base::FilePath test_blob_path;
    ASSERT_TRUE(base::CreateTemporaryFile(&test_blob_path));

    const uint64_t kZeroLength = 0;
    base::Time blob_time;
    ASSERT_TRUE(
        base::Time::FromString("Tue, 15 Nov 1994, 12:45:26 GMT", &blob_time));
    ASSERT_TRUE(base::TouchFile(test_blob_path, blob_time, blob_time));

    BlobStorageContext blob_storage_context;

    // A blob created from an empty file added several times.
    const std::string blob_id("id-0");
    std::unique_ptr<BlobDataBuilder> blob_data_builder(
        new BlobDataBuilder(blob_id));
    blob_data_builder->AppendFile(test_blob_path, 0, kZeroLength, blob_time);
    std::unique_ptr<BlobDataHandle> handle =
        blob_storage_context.AddFinishedBlob(std::move(blob_data_builder));

    scoped_refptr<network::ResourceRequestBody> request_body(
        new network::ResourceRequestBody());
    std::unique_ptr<net::UploadDataStream> upload(
        UploadDataStreamBuilder::Build(
            request_body.get(), &blob_storage_context, nullptr,
            base::ThreadTaskRunnerHandle::Get().get()));

    request_body = new network::ResourceRequestBody();
    request_body->AppendBlob(blob_id);
    request_body->AppendBlob(blob_id);
    request_body->AppendBlob(blob_id);

    upload = UploadDataStreamBuilder::Build(
        request_body.get(), &blob_storage_context, nullptr,
        base::ThreadTaskRunnerHandle::Get().get());
    ASSERT_TRUE(upload->GetElementReaders());
    const auto& readers = *upload->GetElementReaders();
    ASSERT_EQ(3U, readers.size());

    net::TestCompletionCallback init_callback;
    ASSERT_EQ(net::ERR_IO_PENDING,
              upload->Init(init_callback.callback(), net::NetLogWithSource()));
    EXPECT_EQ(net::OK, init_callback.WaitForResult());

    EXPECT_EQ(kZeroLength, upload->size());

    // Purposely (try to) read more than what is in the stream. If we try to
    // read zero bytes then UploadDataStream::Read will fail a DCHECK.
    int kBufferLength = kZeroLength + 1;
    std::unique_ptr<char[]> buffer(new char[kBufferLength]);
    scoped_refptr<net::IOBuffer> io_buffer =
        new net::WrappedIOBuffer(buffer.get());
    net::TestCompletionCallback read_callback;
    int result =
        upload->Read(io_buffer.get(), kBufferLength, read_callback.callback());
    EXPECT_EQ(static_cast<int>(kZeroLength), read_callback.GetResult(result));

    base::DeleteFile(test_blob_path, false);
  }
  // Clean up for ASAN.
  base::RunLoop().RunUntilIdle();
}

TEST(UploadDataStreamBuilderTest, ResetUploadStreamWithBlob) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);
  {
    scoped_refptr<network::ResourceRequestBody> request_body =
        new network::ResourceRequestBody;

    const std::string kBlob = "blobuuid";
    const std::string kBlobData = "blobdata";
    const int kBlobDataLength = 8;
    const int64_t kIdentifier = 12345;

    BlobStorageContext blob_storage_context;
    auto builder = std::make_unique<BlobDataBuilder>(kBlob);
    builder->AppendData(kBlobData);
    std::unique_ptr<BlobDataHandle> handle =
        blob_storage_context.AddFinishedBlob(std::move(builder));
    request_body->AppendBlob(kBlob);
    request_body->set_identifier(kIdentifier);

    std::unique_ptr<net::UploadDataStream> upload(
        UploadDataStreamBuilder::Build(
            request_body.get(), &blob_storage_context, nullptr,
            base::ThreadTaskRunnerHandle::Get().get()));

    net::TestCompletionCallback init_callback;
    ASSERT_EQ(net::OK,
              upload->Init(init_callback.callback(), net::NetLogWithSource()));

    // Read part of the data.
    const int kBufferLength = 4;
    scoped_refptr<net::IOBufferWithSize> buffer(
        new net::IOBufferWithSize(kBufferLength));
    net::TestCompletionCallback read_callback;
    int result =
        upload->Read(buffer.get(), buffer->size(), read_callback.callback());
    EXPECT_EQ(kBufferLength, read_callback.GetResult(result));
    EXPECT_EQ(0,
              std::memcmp(kBlobData.c_str(), buffer->data(), buffer->size()));

    // Reset.
    ASSERT_EQ(net::OK,
              upload->Init(init_callback.callback(), net::NetLogWithSource()));

    // Read all the data.
    buffer = new net::IOBufferWithSize(kBlobDataLength);
    result =
        upload->Read(buffer.get(), buffer->size(), read_callback.callback());
    EXPECT_EQ(kBlobDataLength, read_callback.GetResult(result));
    EXPECT_EQ(0,
              std::memcmp(kBlobData.c_str(), buffer->data(), buffer->size()));
  }
  // Clean up for ASAN.
  base::RunLoop().RunUntilIdle();
}
}  // namespace content
