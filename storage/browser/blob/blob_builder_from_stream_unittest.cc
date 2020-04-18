// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_builder_from_stream.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_item.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/test/fake_progress_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace storage {

namespace {

constexpr size_t kTestBlobStorageMaxBytesDataItemSize = 13;
constexpr size_t kTestBlobStorageMaxBlobMemorySize = 500;
constexpr uint64_t kTestBlobStorageMinFileSizeBytes = 32;
constexpr uint64_t kTestBlobStorageMaxFileSizeBytes = 100;
constexpr uint64_t kTestBlobStorageMaxDiskSpace = 1000;

enum class LengthHintTestType {
  kUnknownSize,
  kCorrectSize,
  kTooLargeSize,
  kTooSmallSize
};

}  // namespace

class BlobBuilderFromStreamTestWithDelayedLimits
    : public testing::TestWithParam<LengthHintTestType> {
 public:
  void SetUp() override {
    ASSERT_TRUE(data_dir_.CreateUniqueTempDir());
    context_ = std::make_unique<BlobStorageContext>(
        data_dir_.GetPath(),
        base::CreateTaskRunnerWithTraits({base::MayBlock()}));

    limits_.max_ipc_memory_size = kTestBlobStorageMaxBytesDataItemSize;
    limits_.max_shared_memory_size = kTestBlobStorageMaxBytesDataItemSize;
    limits_.max_bytes_data_item_size = kTestBlobStorageMaxBytesDataItemSize;
    limits_.max_blob_in_memory_space = kTestBlobStorageMaxBlobMemorySize;
    limits_.min_page_file_size = kTestBlobStorageMinFileSizeBytes;
    limits_.max_file_size = kTestBlobStorageMaxFileSizeBytes;
    limits_.desired_max_disk_space = kTestBlobStorageMaxDiskSpace;
    limits_.effective_max_disk_space = kTestBlobStorageMaxDiskSpace;
  }

  void TearDown() override {
    // Make sure we clean up files.
    base::RunLoop().RunUntilIdle();
    base::TaskScheduler::GetInstance()->FlushForTesting();
    base::RunLoop().RunUntilIdle();
  }

  uint64_t GetLengthHint(uint64_t actual_size) {
    switch (GetParam()) {
      case LengthHintTestType::kUnknownSize:
        return 0;
      case LengthHintTestType::kCorrectSize:
        return actual_size;
      case LengthHintTestType::kTooLargeSize:
        return actual_size + actual_size / 2;
      case LengthHintTestType::kTooSmallSize:
        return actual_size / 2;
    }
    NOTREACHED();
    return 0;
  }

  std::unique_ptr<BlobDataHandle> BuildFromString(
      std::string data,
      bool initial_allocation_should_succeed = true) {
    mojo::DataPipe pipe;
    base::RunLoop loop;
    std::unique_ptr<BlobDataHandle> result;
    uint64_t length_hint = GetLengthHint(data.length());
    BlobBuilderFromStream* finished_builder = nullptr;
    BlobBuilderFromStream builder(
        context_->AsWeakPtr(), kContentType, kContentDisposition, length_hint,
        std::move(pipe.consumer_handle), nullptr,
        base::BindLambdaForTesting([&](BlobBuilderFromStream* result_builder,
                                       std::unique_ptr<BlobDataHandle> blob) {
          finished_builder = result_builder;
          result = std::move(blob);
          loop.Quit();
        }));

    // Make sure the initial memory allocation done by the builder matches the
    // length hint passed in.
    if (initial_allocation_should_succeed &&
        GetParam() != LengthHintTestType::kUnknownSize && length_hint != 0) {
      EXPECT_EQ(length_hint, context_->memory_controller().memory_usage() +
                                 context_->memory_controller().disk_usage())
          << " memory_usage: " << context_->memory_controller().memory_usage()
          << ", disk_usage: " << context_->memory_controller().disk_usage();
    }

    mojo::BlockingCopyFromString(data, pipe.producer_handle);
    pipe.producer_handle.reset();

    loop.Run();
    EXPECT_EQ(&builder, finished_builder);
    return result;
  }

  void VerifyBlobContents(base::span<const char> in_memory_data,
                          base::span<const char> on_disk_data,
                          const BlobDataSnapshot& blob_data) {
    size_t next_memory_offset = 0;
    size_t next_file_offset = 0;
    for (const auto& item : blob_data.items()) {
      if (item->type() == BlobDataItem::Type::kBytes) {
        EXPECT_EQ(0u, next_file_offset)
            << "Bytes item after file items is invalid";

        EXPECT_LE(item->length(), kTestBlobStorageMaxBlobMemorySize);
        ASSERT_LE(next_memory_offset + item->length(), in_memory_data.size());

        EXPECT_EQ(in_memory_data.subspan(next_memory_offset, item->length()),
                  item->bytes());

        next_memory_offset += item->length();
      } else if (item->type() == BlobDataItem::Type::kFile) {
        EXPECT_EQ(next_memory_offset, in_memory_data.size())
            << "File item before all in memory data was found";

        EXPECT_LE(item->length(), kTestBlobStorageMaxFileSizeBytes);
        ASSERT_LE(next_file_offset + item->length(), on_disk_data.size());

        std::string file_contents;
        EXPECT_TRUE(base::ReadFileToString(item->path(), &file_contents));
        EXPECT_EQ(item->length(), file_contents.size());
        EXPECT_EQ(on_disk_data.subspan(next_file_offset, item->length()),
                  base::make_span(file_contents));

        next_file_offset += item->length();
        if (next_file_offset < on_disk_data.size()) {
          EXPECT_EQ(kTestBlobStorageMaxFileSizeBytes, item->length())
              << "All but the last file should be max sized";
        }
      } else {
        ADD_FAILURE() << "Invalid blob item type: "
                      << static_cast<int>(item->type());
      }
    }

    EXPECT_EQ(next_memory_offset, in_memory_data.size());
    EXPECT_EQ(next_file_offset, on_disk_data.size());
  }

 protected:
  const std::string kContentType = "content/type";
  const std::string kContentDisposition = "disposition";

  base::ScopedTempDir data_dir_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  BlobStorageLimits limits_;
  std::unique_ptr<BlobStorageContext> context_;
};

class BlobBuilderFromStreamTest
    : public BlobBuilderFromStreamTestWithDelayedLimits {
 public:
  void SetUp() override {
    BlobBuilderFromStreamTestWithDelayedLimits::SetUp();
    context_->set_limits_for_testing(limits_);
  }
};

TEST_P(BlobBuilderFromStreamTest, CallbackCalledOnDeletion) {
  mojo::DataPipe pipe;

  base::RunLoop loop;
  BlobBuilderFromStream* builder_ptr = nullptr;
  auto builder = std::make_unique<BlobBuilderFromStream>(
      context_->AsWeakPtr(), "", "", GetLengthHint(16),
      std::move(pipe.consumer_handle), nullptr,
      base::BindLambdaForTesting([&](BlobBuilderFromStream* result_builder,
                                     std::unique_ptr<BlobDataHandle> blob) {
        EXPECT_EQ(builder_ptr, result_builder);
        EXPECT_FALSE(blob);
        loop.Quit();
      }));
  builder_ptr = builder.get();
  builder.reset();
  loop.Run();
}

TEST_P(BlobBuilderFromStreamTest, EmptyStream) {
  std::unique_ptr<BlobDataHandle> result = BuildFromString("");

  ASSERT_TRUE(result);
  EXPECT_FALSE(result->uuid().empty());
  EXPECT_EQ(BlobStatus::DONE, result->GetBlobStatus());
  EXPECT_EQ(kContentType, result->content_type());
  EXPECT_EQ(kContentDisposition, result->content_disposition());
  EXPECT_EQ(0u, result->size());

  // Verify memory usage.
  EXPECT_EQ(0u, context_->memory_controller().memory_usage());
  EXPECT_EQ(0u, context_->memory_controller().disk_usage());

  // Verify blob contents.
  VerifyBlobContents(base::span<const char>(), base::span<const char>(),
                     *result->CreateSnapshot());
}

TEST_P(BlobBuilderFromStreamTest, SmallStream) {
  const std::string kData =
      base::RandBytesAsString(kTestBlobStorageMaxBytesDataItemSize + 5);
  std::unique_ptr<BlobDataHandle> result = BuildFromString(kData);

  ASSERT_TRUE(result);
  EXPECT_FALSE(result->uuid().empty());
  EXPECT_EQ(BlobStatus::DONE, result->GetBlobStatus());
  EXPECT_EQ(kData.size(), result->size());

  // Verify memory usage.
  EXPECT_EQ(kData.size(), context_->memory_controller().memory_usage());
  EXPECT_EQ(0u, context_->memory_controller().disk_usage());

  // Verify blob contents.
  VerifyBlobContents(kData, base::span<const char>(),
                     *result->CreateSnapshot());
}

TEST_P(BlobBuilderFromStreamTest, MediumStream) {
  const std::string kData =
      base::RandBytesAsString(kTestBlobStorageMinFileSizeBytes * 3 + 13);
  std::unique_ptr<BlobDataHandle> result = BuildFromString(kData);

  ASSERT_TRUE(result);
  EXPECT_FALSE(result->uuid().empty());
  EXPECT_EQ(BlobStatus::DONE, result->GetBlobStatus());
  EXPECT_EQ(kData.size(), result->size());

  // Verify memory usage.
  if (GetParam() == LengthHintTestType::kUnknownSize) {
    EXPECT_EQ(2 * kTestBlobStorageMaxBytesDataItemSize,
              context_->memory_controller().memory_usage());
    EXPECT_EQ(kData.size() - 2 * kTestBlobStorageMaxBytesDataItemSize,
              context_->memory_controller().disk_usage());
  } else {
    EXPECT_EQ(0u, context_->memory_controller().memory_usage());
    EXPECT_EQ(kData.size(), context_->memory_controller().disk_usage());
  }

  // Verify blob contents.
  if (GetParam() == LengthHintTestType::kUnknownSize) {
    VerifyBlobContents(base::make_span(kData).subspan(
                           0, 2 * kTestBlobStorageMaxBytesDataItemSize),
                       base::make_span(kData).subspan(
                           2 * kTestBlobStorageMaxBytesDataItemSize),
                       *result->CreateSnapshot());
  } else {
    VerifyBlobContents(base::span<const char>(), kData,
                       *result->CreateSnapshot());
  }
}

TEST_P(BlobBuilderFromStreamTest, LargeStream) {
  const std::string kData = base::RandBytesAsString(
      kTestBlobStorageMaxDiskSpace - kTestBlobStorageMinFileSizeBytes);
  std::unique_ptr<BlobDataHandle> result =
      BuildFromString(kData, GetParam() != LengthHintTestType::kTooLargeSize);

  if (GetParam() == LengthHintTestType::kTooLargeSize) {
    EXPECT_FALSE(result);
    EXPECT_EQ(0u, context_->memory_controller().memory_usage());
    EXPECT_EQ(0u, context_->memory_controller().disk_usage());
    return;
  }

  ASSERT_TRUE(result);
  EXPECT_FALSE(result->uuid().empty());
  EXPECT_EQ(BlobStatus::DONE, result->GetBlobStatus());
  EXPECT_EQ(kData.size(), result->size());

  // Verify memory usage.
  if (GetParam() == LengthHintTestType::kUnknownSize) {
    EXPECT_EQ(2 * kTestBlobStorageMaxBytesDataItemSize,
              context_->memory_controller().memory_usage());
    EXPECT_EQ(kData.size() - 2 * kTestBlobStorageMaxBytesDataItemSize,
              context_->memory_controller().disk_usage());
  } else {
    EXPECT_EQ(0u, context_->memory_controller().memory_usage());
    EXPECT_EQ(kData.size(), context_->memory_controller().disk_usage());
  }

  // Verify blob contents.
  if (GetParam() == LengthHintTestType::kUnknownSize) {
    VerifyBlobContents(base::make_span(kData).subspan(
                           0, 2 * kTestBlobStorageMaxBytesDataItemSize),
                       base::make_span(kData).subspan(
                           2 * kTestBlobStorageMaxBytesDataItemSize),
                       *result->CreateSnapshot());
  } else {
    VerifyBlobContents(base::span<const char>(), kData,
                       *result->CreateSnapshot());
  }
}

TEST_P(BlobBuilderFromStreamTest, TooLargeForQuota) {
  const std::string kData = base::RandBytesAsString(
      kTestBlobStorageMaxDiskSpace + kTestBlobStorageMaxBlobMemorySize + 1);
  std::unique_ptr<BlobDataHandle> result =
      BuildFromString(kData, GetParam() == LengthHintTestType::kTooSmallSize);
  EXPECT_FALSE(result);

  // Make sure we clean up files.
  base::RunLoop().RunUntilIdle();
  base::TaskScheduler::GetInstance()->FlushForTesting();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0u, context_->memory_controller().memory_usage());
  EXPECT_EQ(0u, context_->memory_controller().disk_usage());
}

TEST_P(BlobBuilderFromStreamTest, TooLargeForQuotaAndNoDisk) {
  context_->DisableFilePagingForTesting();

  const std::string kData =
      base::RandBytesAsString(kTestBlobStorageMaxBlobMemorySize + 1);
  std::unique_ptr<BlobDataHandle> result =
      BuildFromString(kData, GetParam() == LengthHintTestType::kTooSmallSize);
  EXPECT_FALSE(result);
  EXPECT_EQ(0u, context_->memory_controller().memory_usage());
  EXPECT_EQ(0u, context_->memory_controller().disk_usage());
}

// The next two tests are similar to the previous two, except they don't send
// any data over the datapipe, but should still result in failure as the
// initial memory/file allocation should fail.
TEST_F(BlobBuilderFromStreamTest, HintTooLargeForQuota) {
  const uint64_t kLengthHint =
      kTestBlobStorageMaxDiskSpace + kTestBlobStorageMaxBlobMemorySize + 1;
  mojo::DataPipe pipe;
  base::RunLoop loop;
  std::unique_ptr<BlobDataHandle> result;
  BlobBuilderFromStream builder(
      context_->AsWeakPtr(), "", "", kLengthHint,
      std::move(pipe.consumer_handle), nullptr,
      base::BindLambdaForTesting(
          [&](BlobBuilderFromStream*, std::unique_ptr<BlobDataHandle> blob) {
            result = std::move(blob);
            loop.Quit();
          }));
  pipe.producer_handle.reset();
  loop.Run();

  EXPECT_FALSE(result);
  EXPECT_EQ(0u, context_->memory_controller().memory_usage());
  EXPECT_EQ(0u, context_->memory_controller().disk_usage());
}

TEST_F(BlobBuilderFromStreamTest, HintTooLargeForQuotaAndNoDisk) {
  context_->DisableFilePagingForTesting();

  const uint64_t kLengthHint = kTestBlobStorageMaxBlobMemorySize + 1;
  mojo::DataPipe pipe;
  base::RunLoop loop;
  std::unique_ptr<BlobDataHandle> result;
  BlobBuilderFromStream builder(
      context_->AsWeakPtr(), "", "", kLengthHint,
      std::move(pipe.consumer_handle), nullptr,
      base::BindLambdaForTesting(
          [&](BlobBuilderFromStream*, std::unique_ptr<BlobDataHandle> blob) {
            result = std::move(blob);
            loop.Quit();
          }));
  pipe.producer_handle.reset();
  loop.Run();

  EXPECT_FALSE(result);
  EXPECT_EQ(0u, context_->memory_controller().memory_usage());
  EXPECT_EQ(0u, context_->memory_controller().disk_usage());
}

TEST_P(BlobBuilderFromStreamTest, ProgressEvents) {
  const std::string kData =
      base::RandBytesAsString(kTestBlobStorageMaxBytesDataItemSize + 5);

  FakeProgressClient progress_client;
  blink::mojom::ProgressClientAssociatedPtr progress_client_ptr;
  mojo::AssociatedBinding<blink::mojom::ProgressClient> progress_binding(
      &progress_client,
      MakeRequestAssociatedWithDedicatedPipe(&progress_client_ptr));

  mojo::DataPipe pipe;
  base::RunLoop loop;
  std::unique_ptr<BlobDataHandle> result;
  BlobBuilderFromStream builder(
      context_->AsWeakPtr(), "", "", GetLengthHint(kData.size()),
      std::move(pipe.consumer_handle), progress_client_ptr.PassInterface(),
      base::BindLambdaForTesting(
          [&](BlobBuilderFromStream*, std::unique_ptr<BlobDataHandle> blob) {
            result = std::move(blob);
            loop.Quit();
          }));
  mojo::BlockingCopyFromString(kData, pipe.producer_handle);
  pipe.producer_handle.reset();

  loop.Run();
  progress_binding.FlushForTesting();

  EXPECT_EQ(kData.size(), progress_client.total_size);
  EXPECT_GE(progress_client.call_count, 2);
}

INSTANTIATE_TEST_CASE_P(BlobBuilderFromStreamTest,
                        BlobBuilderFromStreamTest,
                        ::testing::Values(LengthHintTestType::kUnknownSize,
                                          LengthHintTestType::kCorrectSize,
                                          LengthHintTestType::kTooLargeSize,
                                          LengthHintTestType::kTooSmallSize));

TEST_F(BlobBuilderFromStreamTestWithDelayedLimits, LargeStream) {
  const std::string kData =
      base::RandBytesAsString(kDefaultMinPageFileSize + 32);
  limits_.desired_max_disk_space = kDefaultMinPageFileSize * 2;
  limits_.effective_max_disk_space = kDefaultMinPageFileSize * 2;

  mojo::DataPipe pipe;
  base::RunLoop loop;
  std::unique_ptr<BlobDataHandle> result;
  BlobBuilderFromStream builder(
      context_->AsWeakPtr(), kContentType, kContentDisposition, kData.size(),
      std::move(pipe.consumer_handle), nullptr,
      base::BindLambdaForTesting([&](BlobBuilderFromStream* result_builder,
                                     std::unique_ptr<BlobDataHandle> blob) {
        result = std::move(blob);
        loop.Quit();
      }));

  context_->set_limits_for_testing(limits_);
  auto data_producer = std::make_unique<mojo::StringDataPipeProducer>(
      std::move(pipe.producer_handle));
  auto* producer_ptr = data_producer.get();
  producer_ptr->Write(
      kData,
      mojo::StringDataPipeProducer::AsyncWritingMode::
          STRING_STAYS_VALID_UNTIL_COMPLETION,
      base::BindOnce(
          base::DoNothing::Once<std::unique_ptr<mojo::StringDataPipeProducer>,
                                MojoResult>(),
          std::move(data_producer)));
  pipe.producer_handle.reset();
  loop.Run();

  ASSERT_TRUE(result);
  EXPECT_FALSE(result->uuid().empty());
  EXPECT_EQ(BlobStatus::DONE, result->GetBlobStatus());
  EXPECT_EQ(kData.size(), result->size());
}

}  // namespace storage
