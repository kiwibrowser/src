// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/blob_bytes_consumer.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer_test_util.h"
#include "third_party/blink/renderer/core/fetch/data_consumer_handle_test_util.h"
#include "third_party/blink/renderer/core/loader/threadable_loader.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/network/encoded_form_data.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

using DataConsumerCommand = DataConsumerHandleTestUtil::Command;
using PublicState = BytesConsumer::PublicState;
using ReplayingHandle = DataConsumerHandleTestUtil::ReplayingHandle;
using Result = BytesConsumer::Result;

class TestThreadableLoader : public ThreadableLoader {
 public:
  ~TestThreadableLoader() override {
    EXPECT_FALSE(should_be_cancelled_ && !is_cancelled_)
        << "The loader should be cancelled but is not cancelled.";
  }

  void Start(const ResourceRequest& request) override { is_started_ = true; }

  void OverrideTimeout(unsigned long timeout_milliseconds) override {
    ADD_FAILURE() << "overrideTimeout should not be called.";
  }

  void Cancel() override { is_cancelled_ = true; }
  void Detach() override { NOTREACHED(); }

  bool IsStarted() const { return is_started_; }
  bool IsCancelled() const { return is_cancelled_; }
  void SetShouldBeCancelled() { should_be_cancelled_ = true; }

 private:
  bool is_started_ = false;
  bool is_cancelled_ = false;
  bool should_be_cancelled_ = false;
};

class SyncLoadingTestThreadableLoader : public ThreadableLoader {
 public:
  ~SyncLoadingTestThreadableLoader() override { DCHECK(!handle_); }

  void Start(const ResourceRequest& request) override {
    is_started_ = true;
    client_->DidReceiveResponse(0, ResourceResponse(), std::move(handle_));
    client_->DidFinishLoading(0);
  }

  void OverrideTimeout(unsigned long timeout_milliseconds) override {
    ADD_FAILURE() << "overrideTimeout should not be called.";
  }

  void Cancel() override { is_cancelled_ = true; }
  void Detach() override { NOTREACHED(); }

  bool IsStarted() const { return is_started_; }
  bool IsCancelled() const { return is_cancelled_; }

  void SetClient(ThreadableLoaderClient* client) { client_ = client; }

  void SetHandle(std::unique_ptr<WebDataConsumerHandle> handle) {
    handle_ = std::move(handle);
  }

 private:
  bool is_started_ = false;
  bool is_cancelled_ = false;
  ThreadableLoaderClient* client_ = nullptr;
  std::unique_ptr<WebDataConsumerHandle> handle_;
};

class SyncErrorTestThreadableLoader : public ThreadableLoader {
 public:
  ~SyncErrorTestThreadableLoader() override {}

  void Start(const ResourceRequest& request) override {
    is_started_ = true;
    client_->DidFail(ResourceError::Failure(NullURL()));
  }

  void OverrideTimeout(unsigned long timeout_milliseconds) override {
    ADD_FAILURE() << "overrideTimeout should not be called.";
  }

  void Cancel() override { is_cancelled_ = true; }
  void Detach() override { NOTREACHED(); }

  bool IsStarted() const { return is_started_; }
  bool IsCancelled() const { return is_cancelled_; }

  void SetClient(ThreadableLoaderClient* client) { client_ = client; }

 private:
  bool is_started_ = false;
  bool is_cancelled_ = false;
  ThreadableLoaderClient* client_ = nullptr;
};

class BlobBytesConsumerTestClient final
    : public GarbageCollectedFinalized<BlobBytesConsumerTestClient>,
      public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(BlobBytesConsumerTestClient);

 public:
  void OnStateChange() override { ++num_on_state_change_called_; }
  String DebugName() const override { return "BlobBytesConsumerTestClient"; }
  int NumOnStateChangeCalled() const { return num_on_state_change_called_; }

 private:
  int num_on_state_change_called_ = 0;
};

class BlobBytesConsumerTest : public PageTestBase {
 public:
  void SetUp() override { PageTestBase::SetUp(IntSize(1, 1)); }
};

TEST_F(BlobBytesConsumerTest, TwoPhaseRead) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  std::unique_ptr<ReplayingHandle> src = ReplayingHandle::Create();
  src->Add(DataConsumerCommand(DataConsumerCommand::kData, "hello, "));
  src->Add(DataConsumerCommand(DataConsumerCommand::kWait));
  src->Add(DataConsumerCommand(DataConsumerCommand::kData, "world"));
  src->Add(DataConsumerCommand(DataConsumerCommand::kDone));

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());

  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_TRUE(loader->IsStarted());
  EXPECT_FALSE(consumer->DrainAsBlobDataHandle(
      BytesConsumer::BlobSizePolicy::kAllowBlobWithInvalidSize));
  EXPECT_FALSE(consumer->DrainAsFormData());
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());

  consumer->DidReceiveResponse(0, ResourceResponse(), std::move(src));
  consumer->DidFinishLoading(0);

  auto result = (new BytesConsumerTestUtil::TwoPhaseReader(consumer))->Run();
  EXPECT_EQ(Result::kDone, result.first);
  EXPECT_EQ("hello, world",
            BytesConsumerTestUtil::CharVectorToString(result.second));
}

TEST_F(BlobBytesConsumerTest, FailLoading) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);

  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_TRUE(loader->IsStarted());
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());

  int num_on_state_change_called = client->NumOnStateChangeCalled();
  consumer->DidFail(ResourceError::Failure(NullURL()));

  EXPECT_EQ(num_on_state_change_called + 1, client->NumOnStateChangeCalled());
  EXPECT_EQ(PublicState::kErrored, consumer->GetPublicState());
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
}

TEST_F(BlobBytesConsumerTest, FailLoadingAfterResponseReceived) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);

  const char* buffer = nullptr;
  size_t available;
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_TRUE(loader->IsStarted());
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());

  int num_on_state_change_called = client->NumOnStateChangeCalled();
  consumer->DidReceiveResponse(
      0, ResourceResponse(),
      DataConsumerHandleTestUtil::CreateWaitingDataConsumerHandle());
  EXPECT_EQ(num_on_state_change_called + 1, client->NumOnStateChangeCalled());
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());

  consumer->DidFail(ResourceError::Failure(NullURL()));
  EXPECT_EQ(num_on_state_change_called + 2, client->NumOnStateChangeCalled());
  EXPECT_EQ(PublicState::kErrored, consumer->GetPublicState());
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
}

TEST_F(BlobBytesConsumerTest, FailAccessControlCheck) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);

  const char* buffer = nullptr;
  size_t available;
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_TRUE(loader->IsStarted());
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());

  int num_on_state_change_called = client->NumOnStateChangeCalled();
  consumer->DidFail(ResourceError::Failure(NullURL()));
  EXPECT_EQ(num_on_state_change_called + 1, client->NumOnStateChangeCalled());

  EXPECT_EQ(PublicState::kErrored, consumer->GetPublicState());
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
}

TEST_F(BlobBytesConsumerTest, CancelBeforeStarting) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);

  consumer->Cancel();
  // This should be FALSE in production, but TRUE here because we set the
  // loader before starting loading in tests.
  EXPECT_TRUE(loader->IsCancelled());

  const char* buffer = nullptr;
  size_t available;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(PublicState::kClosed, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());
  EXPECT_EQ(0, client->NumOnStateChangeCalled());
}

TEST_F(BlobBytesConsumerTest, CancelAfterStarting) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);

  const char* buffer = nullptr;
  size_t available;
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_TRUE(loader->IsStarted());
  EXPECT_EQ(0, client->NumOnStateChangeCalled());

  consumer->Cancel();
  EXPECT_TRUE(loader->IsCancelled());
  EXPECT_EQ(PublicState::kClosed, consumer->GetPublicState());
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(0, client->NumOnStateChangeCalled());
}

TEST_F(BlobBytesConsumerTest, ReadLastChunkBeforeDidFinishLoadingArrives) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);
  std::unique_ptr<ReplayingHandle> src = ReplayingHandle::Create();
  src->Add(DataConsumerCommand(DataConsumerCommand::kData, "hello"));
  src->Add(DataConsumerCommand(DataConsumerCommand::kDone));

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());

  const char* buffer = nullptr;
  size_t available;
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_TRUE(loader->IsStarted());
  EXPECT_EQ(0, client->NumOnStateChangeCalled());

  consumer->DidReceiveResponse(0, ResourceResponse(), std::move(src));
  EXPECT_EQ(1, client->NumOnStateChangeCalled());
  test::RunPendingTasks();
  EXPECT_EQ(2, client->NumOnStateChangeCalled());

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  ASSERT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(5u, available);
  EXPECT_EQ("hello", String(buffer, available));
  ASSERT_EQ(Result::kOk, consumer->EndRead(available));

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  ASSERT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());

  consumer->DidFinishLoading(0);
  EXPECT_EQ(3, client->NumOnStateChangeCalled());
  EXPECT_EQ(PublicState::kClosed, consumer->GetPublicState());
  ASSERT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
}

TEST_F(BlobBytesConsumerTest, ReadLastChunkAfterDidFinishLoadingArrives) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);
  std::unique_ptr<ReplayingHandle> src = ReplayingHandle::Create();
  src->Add(DataConsumerCommand(DataConsumerCommand::kData, "hello"));
  src->Add(DataConsumerCommand(DataConsumerCommand::kDone));

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());

  const char* buffer = nullptr;
  size_t available;
  EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_TRUE(loader->IsStarted());
  EXPECT_EQ(0, client->NumOnStateChangeCalled());

  consumer->DidReceiveResponse(0, ResourceResponse(), std::move(src));
  EXPECT_EQ(1, client->NumOnStateChangeCalled());
  test::RunPendingTasks();
  EXPECT_EQ(2, client->NumOnStateChangeCalled());

  consumer->DidFinishLoading(0);
  test::RunPendingTasks();
  EXPECT_EQ(2, client->NumOnStateChangeCalled());

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  ASSERT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(5u, available);
  EXPECT_EQ("hello", String(buffer, available));
  ASSERT_EQ(Result::kOk, consumer->EndRead(available));

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(2, client->NumOnStateChangeCalled());
}

TEST_F(BlobBytesConsumerTest, DrainAsBlobDataHandle) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());

  scoped_refptr<BlobDataHandle> result = consumer->DrainAsBlobDataHandle(
      BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize);
  ASSERT_TRUE(result);
  EXPECT_FALSE(consumer->DrainAsBlobDataHandle(
      BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize));
  EXPECT_EQ(12345u, result->size());
  EXPECT_EQ(PublicState::kClosed, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());
}

TEST_F(BlobBytesConsumerTest, DrainAsBlobDataHandle_2) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), -1);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());

  scoped_refptr<BlobDataHandle> result = consumer->DrainAsBlobDataHandle(
      BytesConsumer::BlobSizePolicy::kAllowBlobWithInvalidSize);
  ASSERT_TRUE(result);
  EXPECT_FALSE(consumer->DrainAsBlobDataHandle(
      BytesConsumer::BlobSizePolicy::kAllowBlobWithInvalidSize));
  EXPECT_EQ(UINT64_MAX, result->size());
  EXPECT_EQ(PublicState::kClosed, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());
}

TEST_F(BlobBytesConsumerTest, DrainAsBlobDataHandle_3) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), -1);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());

  EXPECT_FALSE(consumer->DrainAsBlobDataHandle(
      BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize));
  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());
}

TEST_F(BlobBytesConsumerTest, DrainAsFormData) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  TestThreadableLoader* loader = new TestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);

  EXPECT_EQ(PublicState::kReadableOrWaiting, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());

  scoped_refptr<EncodedFormData> result = consumer->DrainAsFormData();
  ASSERT_TRUE(result);
  ASSERT_EQ(1u, result->Elements().size());
  ASSERT_EQ(FormDataElement::kEncodedBlob, result->Elements()[0].type_);
  ASSERT_TRUE(result->Elements()[0].optional_blob_data_handle_);
  EXPECT_EQ(12345u, result->Elements()[0].optional_blob_data_handle_->size());
  EXPECT_EQ(blob_data_handle->Uuid(), result->Elements()[0].blob_uuid_);
  EXPECT_EQ(PublicState::kClosed, consumer->GetPublicState());
  EXPECT_FALSE(loader->IsStarted());
}

TEST_F(BlobBytesConsumerTest, LoaderShouldBeCancelled) {
  {
    scoped_refptr<BlobDataHandle> blob_data_handle =
        BlobDataHandle::Create(BlobData::Create(), 12345);
    TestThreadableLoader* loader = new TestThreadableLoader();
    BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
        &GetDocument(), blob_data_handle, loader);

    const char* buffer = nullptr;
    size_t available;
    EXPECT_EQ(Result::kShouldWait, consumer->BeginRead(&buffer, &available));
    EXPECT_TRUE(loader->IsStarted());
    loader->SetShouldBeCancelled();
  }
  ThreadState::Current()->CollectAllGarbage();
}

TEST_F(BlobBytesConsumerTest, SyncErrorDispatch) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  SyncErrorTestThreadableLoader* loader = new SyncErrorTestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  loader->SetClient(consumer);
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);

  const char* buffer = nullptr;
  size_t available;
  EXPECT_EQ(Result::kError, consumer->BeginRead(&buffer, &available));
  EXPECT_TRUE(loader->IsStarted());

  EXPECT_EQ(0, client->NumOnStateChangeCalled());
  EXPECT_EQ(BytesConsumer::PublicState::kErrored, consumer->GetPublicState());
}

TEST_F(BlobBytesConsumerTest, SyncLoading) {
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(BlobData::Create(), 12345);
  SyncLoadingTestThreadableLoader* loader =
      new SyncLoadingTestThreadableLoader();
  BlobBytesConsumer* consumer = BlobBytesConsumer::CreateForTesting(
      &GetDocument(), blob_data_handle, loader);
  std::unique_ptr<ReplayingHandle> src = ReplayingHandle::Create();
  src->Add(DataConsumerCommand(DataConsumerCommand::kData, "hello, "));
  src->Add(DataConsumerCommand(DataConsumerCommand::kWait));
  src->Add(DataConsumerCommand(DataConsumerCommand::kData, "world"));
  src->Add(DataConsumerCommand(DataConsumerCommand::kDone));
  loader->SetClient(consumer);
  loader->SetHandle(std::move(src));
  BlobBytesConsumerTestClient* client = new BlobBytesConsumerTestClient();
  consumer->SetClient(client);

  const char* buffer = nullptr;
  size_t available;
  ASSERT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  EXPECT_TRUE(loader->IsStarted());
  ASSERT_EQ(7u, available);
  EXPECT_EQ("hello, ", String(buffer, available));

  EXPECT_EQ(0, client->NumOnStateChangeCalled());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            consumer->GetPublicState());
}

TEST_F(BlobBytesConsumerTest, ConstructedFromNullHandle) {
  BlobBytesConsumer* consumer = new BlobBytesConsumer(&GetDocument(), nullptr);
  const char* buffer = nullptr;
  size_t available;
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
}

}  // namespace

}  // namespace blink
