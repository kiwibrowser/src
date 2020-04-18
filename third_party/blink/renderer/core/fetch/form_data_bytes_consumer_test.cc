// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/form_data_bytes_consumer.h"

#include "base/memory/scoped_refptr.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_http_body.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer_test_util.h"
#include "third_party/blink/renderer/core/html/forms/form_data.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/network/encoded_form_data.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
namespace {

using Result = BytesConsumer::Result;
using testing::_;
using testing::DoAll;
using testing::InSequence;
using testing::Return;
using Checkpoint = testing::StrictMock<testing::MockFunction<void(int)>>;
using MockBytesConsumer = BytesConsumerTestUtil::MockBytesConsumer;

class SimpleDataPipeGetter : public network::mojom::blink::DataPipeGetter {
 public:
  SimpleDataPipeGetter(const String& str,
                       network::mojom::blink::DataPipeGetterRequest request)
      : str_(str) {
    bindings_.set_connection_error_handler(WTF::BindRepeating(
        &SimpleDataPipeGetter::OnConnectionError, WTF::Unretained(this)));
    bindings_.AddBinding(this, std::move(request));
  }
  ~SimpleDataPipeGetter() override = default;

  // network::mojom::DataPipeGetter implementation:
  void Read(mojo::ScopedDataPipeProducerHandle handle,
            ReadCallback callback) override {
    bool result = mojo::BlockingCopyFromString(WebString(str_).Utf8(), handle);
    ASSERT_TRUE(result);
    std::move(callback).Run(0 /* OK */, str_.length());
  }

  void Clone(network::mojom::blink::DataPipeGetterRequest request) override {
    bindings_.AddBinding(this, std::move(request));
  }

  void OnConnectionError() {
    if (bindings_.empty())
      delete this;
  }

 private:
  String str_;
  mojo::BindingSet<network::mojom::blink::DataPipeGetter> bindings_;

  DISALLOW_COPY_AND_ASSIGN(SimpleDataPipeGetter);
};

scoped_refptr<EncodedFormData> ComplexFormData() {
  scoped_refptr<EncodedFormData> data = EncodedFormData::Create();

  data->AppendData("foo", 3);
  data->AppendFileRange("/foo/bar/baz", 3, 4, 5);
  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->AppendText("hello", false);
  auto size = blob_data->length();
  scoped_refptr<BlobDataHandle> blob_data_handle =
      BlobDataHandle::Create(std::move(blob_data), size);
  data->AppendBlob(blob_data_handle->Uuid(), blob_data_handle);
  Vector<char> boundary;
  boundary.Append("\0", 1);
  data->SetBoundary(boundary);
  return data;
}

scoped_refptr<EncodedFormData> DataPipeFormData() {
  WebHTTPBody body;
  body.Initialize();
  // Add data.
  body.AppendData(WebData("foo", 3));

  // Add data pipe.
  network::mojom::blink::DataPipeGetterPtr data_pipe_getter_ptr;
  // Object deletes itself.
  new SimpleDataPipeGetter(String(" hello world"),
                           mojo::MakeRequest(&data_pipe_getter_ptr));
  body.AppendDataPipe(data_pipe_getter_ptr.PassInterface().PassHandle());

  // Add another data pipe.
  network::mojom::blink::DataPipeGetterPtr data_pipe_getter_ptr2;
  // Object deletes itself.
  new SimpleDataPipeGetter(String(" here's another data pipe "),
                           mojo::MakeRequest(&data_pipe_getter_ptr2));
  body.AppendDataPipe(data_pipe_getter_ptr2.PassInterface().PassHandle());

  // Add some more data.
  body.AppendData(WebData("bar baz", 7));

  body.SetUniqueBoundary();
  return body;
}

class NoopClient final : public GarbageCollectedFinalized<NoopClient>,
                         public BytesConsumer::Client {
  USING_GARBAGE_COLLECTED_MIXIN(NoopClient);

 public:
  void OnStateChange() override {}
  String DebugName() const override { return "NoopClient"; }
};

class FormDataBytesConsumerTest : public PageTestBase {
 public:
  void SetUp() override { PageTestBase::SetUp(IntSize()); }
};

TEST_F(FormDataBytesConsumerTest, TwoPhaseReadFromString) {
  auto result = (new BytesConsumerTestUtil::TwoPhaseReader(
                     new FormDataBytesConsumer("hello, world")))
                    ->Run();
  EXPECT_EQ(Result::kDone, result.first);
  EXPECT_EQ("hello, world",
            BytesConsumerTestUtil::CharVectorToString(result.second));
}

TEST_F(FormDataBytesConsumerTest, TwoPhaseReadFromStringNonLatin) {
  constexpr UChar kCs[] = {0x3042, 0};
  auto result = (new BytesConsumerTestUtil::TwoPhaseReader(
                     new FormDataBytesConsumer(String(kCs))))
                    ->Run();
  EXPECT_EQ(Result::kDone, result.first);
  EXPECT_EQ("\xe3\x81\x82",
            BytesConsumerTestUtil::CharVectorToString(result.second));
}

TEST_F(FormDataBytesConsumerTest, TwoPhaseReadFromArrayBuffer) {
  constexpr unsigned char kData[] = {0x21, 0xfe, 0x00, 0x00, 0xff, 0xa3,
                                     0x42, 0x30, 0x42, 0x99, 0x88};
  DOMArrayBuffer* buffer = DOMArrayBuffer::Create(kData, arraysize(kData));
  auto result = (new BytesConsumerTestUtil::TwoPhaseReader(
                     new FormDataBytesConsumer(buffer)))
                    ->Run();
  Vector<char> expected;
  expected.Append(kData, arraysize(kData));

  EXPECT_EQ(Result::kDone, result.first);
  EXPECT_EQ(expected, result.second);
}

TEST_F(FormDataBytesConsumerTest, TwoPhaseReadFromArrayBufferView) {
  constexpr unsigned char kData[] = {0x21, 0xfe, 0x00, 0x00, 0xff, 0xa3,
                                     0x42, 0x30, 0x42, 0x99, 0x88};
  constexpr size_t kOffset = 1, kSize = 4;
  DOMArrayBuffer* buffer = DOMArrayBuffer::Create(kData, arraysize(kData));
  auto result =
      (new BytesConsumerTestUtil::TwoPhaseReader(new FormDataBytesConsumer(
           DOMUint8Array::Create(buffer, kOffset, kSize))))
          ->Run();
  Vector<char> expected;
  expected.Append(kData + kOffset, kSize);

  EXPECT_EQ(Result::kDone, result.first);
  EXPECT_EQ(expected, result.second);
}

TEST_F(FormDataBytesConsumerTest, TwoPhaseReadFromSimpleFormData) {
  scoped_refptr<EncodedFormData> data = EncodedFormData::Create();
  data->AppendData("foo", 3);
  data->AppendData("hoge", 4);

  auto result = (new BytesConsumerTestUtil::TwoPhaseReader(
                     new FormDataBytesConsumer(&GetDocument(), data)))
                    ->Run();
  EXPECT_EQ(Result::kDone, result.first);
  EXPECT_EQ("foohoge",
            BytesConsumerTestUtil::CharVectorToString(result.second));
}

TEST_F(FormDataBytesConsumerTest, TwoPhaseReadFromComplexFormData) {
  scoped_refptr<EncodedFormData> data = ComplexFormData();
  MockBytesConsumer* underlying = MockBytesConsumer::Create();
  BytesConsumer* consumer =
      FormDataBytesConsumer::CreateForTesting(&GetDocument(), data, underlying);
  Checkpoint checkpoint;

  const char* buffer = nullptr;
  size_t available = 0;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*underlying, BeginRead(&buffer, &available))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*underlying, EndRead(0)).WillOnce(Return(Result::kOk));
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(1);
  ASSERT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  checkpoint.Call(2);
  EXPECT_EQ(Result::kOk, consumer->EndRead(0));
  checkpoint.Call(3);
}

TEST_F(FormDataBytesConsumerTest, EndReadCanReturnDone) {
  BytesConsumer* consumer = new FormDataBytesConsumer("hello, world");
  const char* buffer = nullptr;
  size_t available = 0;
  ASSERT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(12u, available);
  EXPECT_EQ("hello, world", String(buffer, available));
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            consumer->GetPublicState());
  EXPECT_EQ(Result::kDone, consumer->EndRead(available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, DrainAsBlobDataHandleFromString) {
  BytesConsumer* consumer = new FormDataBytesConsumer("hello, world");
  scoped_refptr<BlobDataHandle> blob_data_handle =
      consumer->DrainAsBlobDataHandle();
  ASSERT_TRUE(blob_data_handle);

  EXPECT_EQ(String(), blob_data_handle->GetType());
  EXPECT_EQ(12u, blob_data_handle->size());
  EXPECT_FALSE(consumer->DrainAsFormData());
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, DrainAsBlobDataHandleFromArrayBuffer) {
  BytesConsumer* consumer =
      new FormDataBytesConsumer(DOMArrayBuffer::Create("foo", 3));
  scoped_refptr<BlobDataHandle> blob_data_handle =
      consumer->DrainAsBlobDataHandle();
  ASSERT_TRUE(blob_data_handle);

  EXPECT_EQ(String(), blob_data_handle->GetType());
  EXPECT_EQ(3u, blob_data_handle->size());
  EXPECT_FALSE(consumer->DrainAsFormData());
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, DrainAsBlobDataHandleFromSimpleFormData) {
  FormData* data = FormData::Create(UTF8Encoding());
  data->append("name1", "value1");
  data->append("name2", "value2");
  scoped_refptr<EncodedFormData> input_form_data =
      data->EncodeMultiPartFormData();

  BytesConsumer* consumer =
      new FormDataBytesConsumer(&GetDocument(), input_form_data);
  scoped_refptr<BlobDataHandle> blob_data_handle =
      consumer->DrainAsBlobDataHandle();
  ASSERT_TRUE(blob_data_handle);

  EXPECT_EQ(String(), blob_data_handle->GetType());
  EXPECT_EQ(input_form_data->FlattenToString().Utf8().length(),
            blob_data_handle->size());
  EXPECT_FALSE(consumer->DrainAsFormData());
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, DrainAsBlobDataHandleFromComplexFormData) {
  scoped_refptr<EncodedFormData> input_form_data = ComplexFormData();

  BytesConsumer* consumer =
      new FormDataBytesConsumer(&GetDocument(), input_form_data);
  scoped_refptr<BlobDataHandle> blob_data_handle =
      consumer->DrainAsBlobDataHandle();
  ASSERT_TRUE(blob_data_handle);

  EXPECT_FALSE(consumer->DrainAsFormData());
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, DrainAsFormDataFromString) {
  BytesConsumer* consumer = new FormDataBytesConsumer("hello, world");
  scoped_refptr<EncodedFormData> form_data = consumer->DrainAsFormData();
  ASSERT_TRUE(form_data);
  EXPECT_EQ("hello, world", form_data->FlattenToString());

  EXPECT_FALSE(consumer->DrainAsBlobDataHandle());
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, DrainAsFormDataFromArrayBuffer) {
  BytesConsumer* consumer =
      new FormDataBytesConsumer(DOMArrayBuffer::Create("foo", 3));
  scoped_refptr<EncodedFormData> form_data = consumer->DrainAsFormData();
  ASSERT_TRUE(form_data);
  EXPECT_TRUE(form_data->IsSafeToSendToAnotherThread());
  EXPECT_EQ("foo", form_data->FlattenToString());

  EXPECT_FALSE(consumer->DrainAsBlobDataHandle());
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, DrainAsFormDataFromSimpleFormData) {
  FormData* data = FormData::Create(UTF8Encoding());
  data->append("name1", "value1");
  data->append("name2", "value2");
  scoped_refptr<EncodedFormData> input_form_data =
      data->EncodeMultiPartFormData();

  BytesConsumer* consumer =
      new FormDataBytesConsumer(&GetDocument(), input_form_data);
  EXPECT_EQ(input_form_data, consumer->DrainAsFormData());
  EXPECT_FALSE(consumer->DrainAsBlobDataHandle());
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, DrainAsFormDataFromComplexFormData) {
  scoped_refptr<EncodedFormData> input_form_data = ComplexFormData();

  BytesConsumer* consumer =
      new FormDataBytesConsumer(&GetDocument(), input_form_data);
  EXPECT_EQ(input_form_data, consumer->DrainAsFormData());
  EXPECT_FALSE(consumer->DrainAsBlobDataHandle());
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(Result::kDone, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, BeginReadAffectsDraining) {
  const char* buffer = nullptr;
  size_t available = 0;
  BytesConsumer* consumer = new FormDataBytesConsumer("hello, world");
  ASSERT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  EXPECT_EQ("hello, world", String(buffer, available));

  ASSERT_EQ(Result::kOk, consumer->EndRead(0));
  EXPECT_FALSE(consumer->DrainAsFormData());
  EXPECT_FALSE(consumer->DrainAsBlobDataHandle());
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            consumer->GetPublicState());
}

TEST_F(FormDataBytesConsumerTest, BeginReadAffectsDrainingWithComplexFormData) {
  MockBytesConsumer* underlying = MockBytesConsumer::Create();
  BytesConsumer* consumer = FormDataBytesConsumer::CreateForTesting(
      &GetDocument(), ComplexFormData(), underlying);

  const char* buffer = nullptr;
  size_t available = 0;
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*underlying, BeginRead(&buffer, &available))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*underlying, EndRead(0)).WillOnce(Return(Result::kOk));
  EXPECT_CALL(checkpoint, Call(2));
  // drainAsFormData should not be called here.
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*underlying, DrainAsBlobDataHandle(_));
  EXPECT_CALL(checkpoint, Call(4));
  // |consumer| delegates the getPublicState call to |underlying|.
  EXPECT_CALL(*underlying, GetPublicState())
      .WillOnce(Return(BytesConsumer::PublicState::kReadableOrWaiting));
  EXPECT_CALL(checkpoint, Call(5));

  checkpoint.Call(1);
  ASSERT_EQ(Result::kOk, consumer->BeginRead(&buffer, &available));
  ASSERT_EQ(Result::kOk, consumer->EndRead(0));
  checkpoint.Call(2);
  EXPECT_FALSE(consumer->DrainAsFormData());
  checkpoint.Call(3);
  EXPECT_FALSE(consumer->DrainAsBlobDataHandle());
  checkpoint.Call(4);
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            consumer->GetPublicState());
  checkpoint.Call(5);
}

TEST_F(FormDataBytesConsumerTest, SetClientWithComplexFormData) {
  scoped_refptr<EncodedFormData> input_form_data = ComplexFormData();

  MockBytesConsumer* underlying = MockBytesConsumer::Create();
  BytesConsumer* consumer = FormDataBytesConsumer::CreateForTesting(
      &GetDocument(), input_form_data, underlying);
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*underlying, SetClient(_));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*underlying, ClearClient());
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(1);
  consumer->SetClient(new NoopClient());
  checkpoint.Call(2);
  consumer->ClearClient();
  checkpoint.Call(3);
}

TEST_F(FormDataBytesConsumerTest, CancelWithComplexFormData) {
  scoped_refptr<EncodedFormData> input_form_data = ComplexFormData();

  MockBytesConsumer* underlying = MockBytesConsumer::Create();
  BytesConsumer* consumer = FormDataBytesConsumer::CreateForTesting(
      &GetDocument(), input_form_data, underlying);
  Checkpoint checkpoint;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*underlying, Cancel());
  EXPECT_CALL(checkpoint, Call(2));

  checkpoint.Call(1);
  consumer->Cancel();
  checkpoint.Call(2);
}

// Tests consuming an EncodedFormData with data pipe elements.
TEST_F(FormDataBytesConsumerTest, DataPipeFormData) {
  scoped_refptr<EncodedFormData> input_form_data = DataPipeFormData();
  auto* consumer = new FormDataBytesConsumer(&GetDocument(), input_form_data);
  auto* reader = new BytesConsumerTestUtil::TwoPhaseReader(consumer);
  std::pair<BytesConsumer::Result, Vector<char>> result = reader->Run();
  EXPECT_EQ(Result::kDone, result.first);
  EXPECT_EQ("foo hello world here's another data pipe bar baz",
            BytesConsumerTestUtil::CharVectorToString(result.second));
}

// Tests DrainAsFormData() on an EncodedFormData with data pipe elements.
TEST_F(FormDataBytesConsumerTest, DataPipeFormData_DrainAsFormData) {
  scoped_refptr<EncodedFormData> input_form_data = DataPipeFormData();
  auto* consumer = new FormDataBytesConsumer(&GetDocument(), input_form_data);
  scoped_refptr<EncodedFormData> drained_form_data =
      consumer->DrainAsFormData();
  EXPECT_EQ(*input_form_data, *drained_form_data);
  EXPECT_EQ(BytesConsumer::PublicState::kClosed, consumer->GetPublicState());
}

// Tests DrainAsFormData() on an EncodedFormData with data pipe elements after
// starting to read.
TEST_F(FormDataBytesConsumerTest,
       DataPipeFormData_DrainAsFormDataWhileReading) {
  // Create the consumer and start reading.
  scoped_refptr<EncodedFormData> input_form_data = DataPipeFormData();
  auto* consumer = new FormDataBytesConsumer(&GetDocument(), input_form_data);
  const char* buffer = nullptr;
  size_t available = 0;
  EXPECT_EQ(BytesConsumer::Result::kOk,
            consumer->BeginRead(&buffer, &available));
  EXPECT_EQ("foo", String(buffer, available));

  // Try to drain form data. It should return null since we started reading.
  scoped_refptr<EncodedFormData> drained_form_data =
      consumer->DrainAsFormData();
  EXPECT_FALSE(drained_form_data);
  EXPECT_EQ(BytesConsumer::PublicState::kReadableOrWaiting,
            consumer->GetPublicState());
  EXPECT_EQ(BytesConsumer::Result::kOk, consumer->EndRead(available));

  // The consumer should still be readable. Finish reading.
  auto* reader = new BytesConsumerTestUtil::TwoPhaseReader(consumer);
  std::pair<BytesConsumer::Result, Vector<char>> result = reader->Run();
  EXPECT_EQ(Result::kDone, result.first);
  EXPECT_EQ(" hello world here's another data pipe bar baz",
            BytesConsumerTestUtil::CharVectorToString(result.second));
}

}  // namespace
}  // namespace blink
