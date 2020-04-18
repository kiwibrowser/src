// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/bytes_consumer.h"

#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer_test_util.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

namespace blink {

namespace {

using BytesConsumerCommand = BytesConsumerTestUtil::Command;
using Result = BytesConsumer::Result;
using ReplayingBytesConsumer = BytesConsumerTestUtil::ReplayingBytesConsumer;

class BytesConsumerTestClient final
    : public GarbageCollectedFinalized<BytesConsumerTestClient>,
      public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(BytesConsumerTestClient);

 public:
  void OnStateChange() override { ++num_on_state_change_called_; }
  String DebugName() const override { return "BytesConsumerTestClient"; }
  int NumOnStateChangeCalled() const { return num_on_state_change_called_; }

 private:
  int num_on_state_change_called_ = 0;
};

class BytesConsumerTeeTest : public PageTestBase {
 public:
  void SetUp() override { PageTestBase::SetUp(IntSize()); }
};

class FakeBlobBytesConsumer : public BytesConsumer {
 public:
  explicit FakeBlobBytesConsumer(scoped_refptr<BlobDataHandle> handle)
      : blob_handle_(std::move(handle)) {}
  ~FakeBlobBytesConsumer() override {}

  Result BeginRead(const char** buffer, size_t* available) override {
    if (state_ == PublicState::kClosed)
      return Result::kDone;
    blob_handle_ = nullptr;
    state_ = PublicState::kErrored;
    return Result::kError;
  }
  Result EndRead(size_t read_size) override {
    if (state_ == PublicState::kClosed)
      return Result::kError;
    blob_handle_ = nullptr;
    state_ = PublicState::kErrored;
    return Result::kError;
  }
  scoped_refptr<BlobDataHandle> DrainAsBlobDataHandle(
      BlobSizePolicy policy) override {
    if (state_ != PublicState::kReadableOrWaiting)
      return nullptr;
    DCHECK(blob_handle_);
    if (policy == BlobSizePolicy::kDisallowBlobWithInvalidSize &&
        blob_handle_->size() == UINT64_MAX)
      return nullptr;
    state_ = PublicState::kClosed;
    return std::move(blob_handle_);
  }

  void SetClient(Client*) override {}
  void ClearClient() override {}
  void Cancel() override {}
  PublicState GetPublicState() const override { return state_; }
  Error GetError() const override { return Error(); }
  String DebugName() const override { return "FakeBlobBytesConsumer"; }

 private:
  PublicState state_ = PublicState::kReadableOrWaiting;
  scoped_refptr<BlobDataHandle> blob_handle_;
};

TEST_F(BytesConsumerTeeTest, CreateDone) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kDone));
  EXPECT_FALSE(src->IsCancelled());

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  auto result1 = (new BytesConsumerTestUtil::TwoPhaseReader(dest1))->Run();
  auto result2 = (new BytesConsumerTestUtil::TwoPhaseReader(dest2))->Run();

  EXPECT_EQ(Result::kDone, result1.first);
  EXPECT_TRUE(result1.second.IsEmpty());
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(Result::kDone, result2.first);
  EXPECT_TRUE(result2.second.IsEmpty());
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest2->GetPublicState());
  EXPECT_FALSE(src->IsCancelled());

  // Cancelling does nothing when closed.
  dest1->Cancel();
  dest2->Cancel();
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest2->GetPublicState());
  EXPECT_FALSE(src->IsCancelled());
}

TEST_F(BytesConsumerTeeTest, TwoPhaseRead) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());

  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "hello, "));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "world"));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kDone));

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest2->GetPublicState());

  auto result1 = (new BytesConsumerTestUtil::TwoPhaseReader(dest1))->Run();
  auto result2 = (new BytesConsumerTestUtil::TwoPhaseReader(dest2))->Run();

  EXPECT_EQ(Result::kDone, result1.first);
  EXPECT_EQ("hello, world",
            BytesConsumerTestUtil::CharVectorToString(result1.second));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(Result::kDone, result2.first);
  EXPECT_EQ("hello, world",
            BytesConsumerTestUtil::CharVectorToString(result2.second));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest2->GetPublicState());
  EXPECT_FALSE(src->IsCancelled());
}

TEST_F(BytesConsumerTeeTest, Error) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());

  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "hello, "));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "world"));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kError));

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  EXPECT_EQ(BytesConsumer::PublicState::kErrored, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, dest2->GetPublicState());

  auto result1 = (new BytesConsumerTestUtil::TwoPhaseReader(dest1))->Run();
  auto result2 = (new BytesConsumerTestUtil::TwoPhaseReader(dest2))->Run();

  EXPECT_EQ(Result::kError, result1.first);
  EXPECT_TRUE(result1.second.IsEmpty());
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, dest1->GetPublicState());
  EXPECT_EQ(Result::kError, result2.first);
  EXPECT_TRUE(result2.second.IsEmpty());
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, dest2->GetPublicState());
  EXPECT_FALSE(src->IsCancelled());

  // Cancelling does nothing when errored.
  dest1->Cancel();
  dest2->Cancel();
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, dest2->GetPublicState());
  EXPECT_FALSE(src->IsCancelled());
}

TEST_F(BytesConsumerTeeTest, Cancel) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());

  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "hello, "));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest2->GetPublicState());

  EXPECT_FALSE(src->IsCancelled());
  dest1->Cancel();
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest2->GetPublicState());
  EXPECT_FALSE(src->IsCancelled());
  dest2->Cancel();
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest2->GetPublicState());
  EXPECT_TRUE(src->IsCancelled());
}

TEST_F(BytesConsumerTeeTest, CancelShouldNotAffectTheOtherDestination) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());

  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "hello, "));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "world"));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kDone));

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest2->GetPublicState());

  EXPECT_FALSE(src->IsCancelled());
  dest1->Cancel();
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest2->GetPublicState());
  EXPECT_FALSE(src->IsCancelled());

  auto result2 = (new BytesConsumerTestUtil::TwoPhaseReader(dest2))->Run();

  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest2->GetPublicState());
  EXPECT_EQ(Result::kDone, result2.first);
  EXPECT_EQ("hello, world",
            BytesConsumerTestUtil::CharVectorToString(result2.second));
  EXPECT_FALSE(src->IsCancelled());
}

TEST_F(BytesConsumerTeeTest, CancelShouldNotAffectTheOtherDestination2) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());

  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "hello, "));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "world"));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kError));

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest2->GetPublicState());

  EXPECT_FALSE(src->IsCancelled());
  dest1->Cancel();
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest2->GetPublicState());
  EXPECT_FALSE(src->IsCancelled());

  auto result2 = (new BytesConsumerTestUtil::TwoPhaseReader(dest2))->Run();

  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, dest2->GetPublicState());
  EXPECT_EQ(Result::kError, result2.first);
  EXPECT_FALSE(src->IsCancelled());
}

TEST_F(BytesConsumerTeeTest, BlobHandle) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  BytesConsumer* src = new FakeBlobBytesConsumer(blob_data_handle);

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  scoped_refptr<BlobDataHandle> dest_blob_data_handle1 =
      dest1->DrainAsBlobDataHandle(
          BytesConsumer::BlobSizePolicy::kAllowBlobWithInvalidSize);
  scoped_refptr<BlobDataHandle> dest_blob_data_handle2 =
      dest2->DrainAsBlobDataHandle(
          BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize);
  ASSERT_TRUE(dest_blob_data_handle1);
  ASSERT_TRUE(dest_blob_data_handle2);
  EXPECT_EQ(12345u, dest_blob_data_handle1->size());
  EXPECT_EQ(12345u, dest_blob_data_handle2->size());
}

TEST_F(BytesConsumerTeeTest, BlobHandleWithInvalidSize) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), -1);
  BytesConsumer* src = new FakeBlobBytesConsumer(blob_data_handle);

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  scoped_refptr<BlobDataHandle> dest_blob_data_handle1 =
      dest1->DrainAsBlobDataHandle(
          BytesConsumer::BlobSizePolicy::kAllowBlobWithInvalidSize);
  scoped_refptr<BlobDataHandle> dest_blob_data_handle2 =
      dest2->DrainAsBlobDataHandle(
          BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize);
  ASSERT_TRUE(dest_blob_data_handle1);
  ASSERT_FALSE(dest_blob_data_handle2);
  EXPECT_EQ(UINT64_MAX, dest_blob_data_handle1->size());
}

TEST_F(BytesConsumerTeeTest, ConsumerCanBeErroredInTwoPhaseRead) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "a"));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kError));

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);
  BytesConsumerTestClient* client = new BytesConsumerTestClient();
  dest1->SetClient(client);

  const char* buffer = nullptr;
  size_t available = 0;
  ASSERT_EQ(Result::kOk, dest1->BeginRead(&buffer, &available));
  ASSERT_EQ(1u, available);

  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest1->GetPublicState());
  int num_on_state_change_called = client->NumOnStateChangeCalled();
  EXPECT_EQ(Result::kError,
            (new BytesConsumerTestUtil::TwoPhaseReader(dest2))->Run().first);
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, dest1->GetPublicState());
  EXPECT_EQ(num_on_state_change_called + 1, client->NumOnStateChangeCalled());
  EXPECT_EQ('a', buffer[0]);
  EXPECT_EQ(Result::kOk, dest1->EndRead(available));
}

TEST_F(BytesConsumerTeeTest,
       AsyncNotificationShouldBeDispatchedWhenAllDataIsConsumed) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "a"));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kWait));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kDone));
  BytesConsumerTestClient* client = new BytesConsumerTestClient();

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  dest1->SetClient(client);

  const char* buffer = nullptr;
  size_t available = 0;
  ASSERT_EQ(Result::kOk, dest1->BeginRead(&buffer, &available));
  ASSERT_EQ(1u, available);
  EXPECT_EQ('a', buffer[0]);

  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            src->GetPublicState());
  test::RunPendingTasks();
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, src->GetPublicState());
  // Just for checking UAF.
  EXPECT_EQ('a', buffer[0]);
  ASSERT_EQ(Result::kOk, dest1->EndRead(1));

  EXPECT_EQ(0, client->NumOnStateChangeCalled());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest1->GetPublicState());
  test::RunPendingTasks();
  EXPECT_EQ(1, client->NumOnStateChangeCalled());
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
}

TEST_F(BytesConsumerTeeTest,
       AsyncCloseNotificationShouldBeCancelledBySubsequentReadCall) {
  ReplayingBytesConsumer* src = new ReplayingBytesConsumer(&GetDocument());
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "a"));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kDone));
  BytesConsumerTestClient* client = new BytesConsumerTestClient();

  BytesConsumer* dest1 = nullptr;
  BytesConsumer* dest2 = nullptr;
  BytesConsumer::Tee(&GetDocument(), src, &dest1, &dest2);

  dest1->SetClient(client);

  const char* buffer = nullptr;
  size_t available = 0;
  ASSERT_EQ(Result::kOk, dest1->BeginRead(&buffer, &available));
  ASSERT_EQ(1u, available);
  EXPECT_EQ('a', buffer[0]);

  test::RunPendingTasks();
  // Just for checking UAF.
  EXPECT_EQ('a', buffer[0]);
  ASSERT_EQ(Result::kOk, dest1->EndRead(1));
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            dest1->GetPublicState());

  EXPECT_EQ(Result::kDone, dest1->BeginRead(&buffer, &available));
  EXPECT_EQ(0, client->NumOnStateChangeCalled());
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
  test::RunPendingTasks();
  EXPECT_EQ(0, client->NumOnStateChangeCalled());
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, dest1->GetPublicState());
}

TEST(BytesConusmerTest, ClosedBytesConsumer) {
  BytesConsumer* consumer = BytesConsumer::CreateClosed();

  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST(BytesConusmerTest, ErroredBytesConsumer) {
  BytesConsumer::Error error("hello");
  BytesConsumer* consumer = BytesConsumer::CreateErrored(error);

  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, consumer->GetPublicState());
  EXPECT_EQ(error.Message(), consumer->GetError().Message());

  consumer->Cancel();
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, consumer->GetPublicState());
}

}  // namespace

}  // namespace blink
