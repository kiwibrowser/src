// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/fetch_data_loader.h"

#include <memory>
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer_for_data_consumer_handle.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer_test_util.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"
#include "third_party/blink/renderer/core/html/forms/form_data.h"

namespace blink {

namespace {

using testing::ByMove;
using testing::InSequence;
using testing::Return;
using testing::DoAll;
using testing::StrictMock;
using testing::_;
using testing::SaveArg;
using testing::SetArgPointee;
using Checkpoint = StrictMock<testing::MockFunction<void(int)>>;
using MockFetchDataLoaderClient =
    BytesConsumerTestUtil::MockFetchDataLoaderClient;
using MockBytesConsumer = BytesConsumerTestUtil::MockBytesConsumer;
using Result = BytesConsumer::Result;

constexpr char kQuickBrownFox[] = "Quick brown fox";
constexpr size_t kQuickBrownFoxLength = 15;
constexpr size_t kQuickBrownFoxLengthWithTerminatingNull = 16;
constexpr char kQuickBrownFoxFormData[] =
    "--boundary\r\n"
    "Content-Disposition: form-data; name=blob; filename=blob\r\n"
    "Content-Type: text/plain; charset=iso-8859-1\r\n"
    "\r\n"
    "Quick brown fox\r\n"
    "--boundary\r\n"
    "Content-Disposition: form-data; name=\"blob\xC2\xA0without\xC2\xA0type\"; "
    "filename=\"blob\xC2\xA0without\xC2\xA0type.txt\"\r\n"
    "\r\n"
    "Quick brown fox\r\n"
    "--boundary\r\n"
    "Content-Disposition: form-data; name=string\r\n"
    "\r\n"
    "Quick brown fox\r\n"
    "--boundary\r\n"
    "Content-Disposition: form-data; name=string-with-type\r\n"
    "Content-Type: text/plain; charset=invalid\r\n"
    "\r\n"
    "Quick brown fox\r\n"
    "--boundary--\r\n";
constexpr size_t kQuickBrownFoxFormDataLength =
    arraysize(kQuickBrownFoxFormData) - 1u;

TEST(FetchDataLoaderTest, LoadAsBlob) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsBlobHandle("text/test");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();
  scoped_refptr<BlobDataHandle> blob_data_handle;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer,
              DrainAsBlobDataHandle(
                  BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize))
      .WillOnce(Return(ByMove(nullptr)));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFox),
                      SetArgPointee<1>(kQuickBrownFoxLengthWithTerminatingNull),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxLengthWithTerminatingNull))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kDone));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadedBlobHandleMock(_))
      .WillOnce(SaveArg<0>(&blob_data_handle));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);

  ASSERT_TRUE(blob_data_handle);
  EXPECT_EQ(kQuickBrownFoxLengthWithTerminatingNull, blob_data_handle->size());
  EXPECT_EQ(String("text/test"), blob_data_handle->GetType());
}

TEST(FetchDataLoaderTest, LoadAsBlobFailed) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsBlobHandle("text/test");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer,
              DrainAsBlobDataHandle(
                  BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize))
      .WillOnce(Return(ByMove(nullptr)));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFox),
                      SetArgPointee<1>(kQuickBrownFoxLengthWithTerminatingNull),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxLengthWithTerminatingNull))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kError));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadFailed());
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);
}

TEST(FetchDataLoaderTest, LoadAsBlobCancel) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsBlobHandle("text/test");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer,
              DrainAsBlobDataHandle(
                  BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize))
      .WillOnce(Return(ByMove(nullptr)));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  fetch_data_loader->Cancel();
  checkpoint.Call(3);
}

TEST(FetchDataLoaderTest,
     LoadAsBlobViaDrainAsBlobDataHandleWithSameContentType) {
  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->AppendBytes(kQuickBrownFox,
                         kQuickBrownFoxLengthWithTerminatingNull);
  blob_data->SetContentType("text/test");
  scoped_refptr<BlobDataHandle> input_blob_data_handle = BlobDataHandle::Create(
      std::move(blob_data), kQuickBrownFoxLengthWithTerminatingNull);

  Checkpoint checkpoint;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsBlobHandle("text/test");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();
  scoped_refptr<BlobDataHandle> blob_data_handle;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer,
              DrainAsBlobDataHandle(
                  BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize))
      .WillOnce(Return(ByMove(input_blob_data_handle)));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadedBlobHandleMock(_))
      .WillOnce(SaveArg<0>(&blob_data_handle));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  fetch_data_loader->Cancel();
  checkpoint.Call(3);

  ASSERT_TRUE(blob_data_handle);
  EXPECT_EQ(input_blob_data_handle, blob_data_handle);
  EXPECT_EQ(kQuickBrownFoxLengthWithTerminatingNull, blob_data_handle->size());
  EXPECT_EQ(String("text/test"), blob_data_handle->GetType());
}

TEST(FetchDataLoaderTest,
     LoadAsBlobViaDrainAsBlobDataHandleWithDifferentContentType) {
  std::unique_ptr<BlobData> blob_data = BlobData::Create();
  blob_data->AppendBytes(kQuickBrownFox,
                         kQuickBrownFoxLengthWithTerminatingNull);
  blob_data->SetContentType("text/different");
  scoped_refptr<BlobDataHandle> input_blob_data_handle = BlobDataHandle::Create(
      std::move(blob_data), kQuickBrownFoxLengthWithTerminatingNull);

  Checkpoint checkpoint;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsBlobHandle("text/test");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();
  scoped_refptr<BlobDataHandle> blob_data_handle;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer,
              DrainAsBlobDataHandle(
                  BytesConsumer::BlobSizePolicy::kDisallowBlobWithInvalidSize))
      .WillOnce(Return(ByMove(input_blob_data_handle)));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadedBlobHandleMock(_))
      .WillOnce(SaveArg<0>(&blob_data_handle));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  fetch_data_loader->Cancel();
  checkpoint.Call(3);

  ASSERT_TRUE(blob_data_handle);
  EXPECT_NE(input_blob_data_handle, blob_data_handle);
  EXPECT_EQ(kQuickBrownFoxLengthWithTerminatingNull, blob_data_handle->size());
  EXPECT_EQ(String("text/test"), blob_data_handle->GetType());
}

TEST(FetchDataLoaderTest, LoadAsArrayBuffer) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsArrayBuffer();
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();
  DOMArrayBuffer* array_buffer = nullptr;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFox),
                      SetArgPointee<1>(kQuickBrownFoxLengthWithTerminatingNull),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxLengthWithTerminatingNull))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kDone));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadedArrayBufferMock(_))
      .WillOnce(SaveArg<0>(&array_buffer));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);

  ASSERT_TRUE(array_buffer);
  ASSERT_EQ(kQuickBrownFoxLengthWithTerminatingNull,
            array_buffer->ByteLength());
  EXPECT_STREQ(kQuickBrownFox, static_cast<const char*>(array_buffer->Data()));
}

TEST(FetchDataLoaderTest, LoadAsArrayBufferFailed) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsArrayBuffer();
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFox),
                      SetArgPointee<1>(kQuickBrownFoxLengthWithTerminatingNull),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxLengthWithTerminatingNull))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kError));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadFailed());
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);
}

TEST(FetchDataLoaderTest, LoadAsArrayBufferCancel) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsArrayBuffer();
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  fetch_data_loader->Cancel();
  checkpoint.Call(3);
}

TEST(FetchDataLoaderTest, LoadAsFormData) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsFormData("boundary");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();
  FormData* form_data = nullptr;

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFoxFormData),
                      SetArgPointee<1>(kQuickBrownFoxFormDataLength),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxFormDataLength))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kDone));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadedFormDataMock(_))
      .WillOnce(SaveArg<0>(&form_data));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);

  ASSERT_TRUE(form_data);
  ASSERT_EQ(4u, form_data->Entries().size());

  EXPECT_EQ("blob", form_data->Entries()[0]->name());
  EXPECT_EQ("blob", form_data->Entries()[0]->Filename());
  ASSERT_TRUE(form_data->Entries()[0]->isFile());
  EXPECT_EQ(kQuickBrownFoxLength, form_data->Entries()[0]->GetBlob()->size());
  EXPECT_EQ("text/plain; charset=iso-8859-1",
            form_data->Entries()[0]->GetBlob()->type());

  EXPECT_EQ("blob\xC2\xA0without\xC2\xA0type",
            form_data->Entries()[1]->name().Utf8());
  EXPECT_EQ("blob\xC2\xA0without\xC2\xA0type.txt",
            form_data->Entries()[1]->Filename().Utf8());
  ASSERT_TRUE(form_data->Entries()[1]->isFile());
  EXPECT_EQ(kQuickBrownFoxLength, form_data->Entries()[1]->GetBlob()->size());
  EXPECT_EQ("text/plain", form_data->Entries()[1]->GetBlob()->type());

  EXPECT_EQ("string", form_data->Entries()[2]->name());
  EXPECT_TRUE(form_data->Entries()[2]->Filename().IsNull());
  ASSERT_TRUE(form_data->Entries()[2]->IsString());
  EXPECT_EQ(kQuickBrownFox, form_data->Entries()[2]->Value());

  EXPECT_EQ("string-with-type", form_data->Entries()[3]->name());
  EXPECT_TRUE(form_data->Entries()[3]->Filename().IsNull());
  ASSERT_TRUE(form_data->Entries()[3]->IsString());
  EXPECT_EQ(kQuickBrownFox, form_data->Entries()[3]->Value());
}

TEST(FetchDataLoaderTest, LoadAsFormDataPartialInput) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsFormData("boundary");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFoxFormData),
                      SetArgPointee<1>(kQuickBrownFoxFormDataLength - 3u),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxFormDataLength - 3u))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kDone));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadFailed());
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);
}

TEST(FetchDataLoaderTest, LoadAsFormDataFailed) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsFormData("boundary");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFoxFormData),
                      SetArgPointee<1>(kQuickBrownFoxFormDataLength),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxFormDataLength))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kError));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadFailed());
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);
}

TEST(FetchDataLoaderTest, LoadAsFormDataCancel) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader =
      FetchDataLoader::CreateLoaderAsFormData("boundary");
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  fetch_data_loader->Cancel();
  checkpoint.Call(3);
}

TEST(FetchDataLoaderTest, LoadAsString) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader = FetchDataLoader::CreateLoaderAsString();
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFox),
                      SetArgPointee<1>(kQuickBrownFoxLength),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxLength))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kDone));
  EXPECT_CALL(*fetch_data_loader_client,
              DidFetchDataLoadedString(String(kQuickBrownFox)));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);
}

TEST(FetchDataLoaderTest, LoadAsStringWithNullBytes) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader = FetchDataLoader::CreateLoaderAsString();
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  constexpr char kPattern[] = "Quick\0brown\0fox";
  constexpr size_t kLength = sizeof(kPattern);

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kPattern), SetArgPointee<1>(kLength),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(16)).WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kDone));
  EXPECT_CALL(*fetch_data_loader_client,
              DidFetchDataLoadedString(String(kPattern, kLength)));
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);
}

TEST(FetchDataLoaderTest, LoadAsStringError) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader = FetchDataLoader::CreateLoaderAsString();
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(kQuickBrownFox),
                      SetArgPointee<1>(kQuickBrownFoxLength),
                      Return(Result::kOk)));
  EXPECT_CALL(*consumer, EndRead(kQuickBrownFoxLength))
      .WillOnce(Return(Result::kOk));
  EXPECT_CALL(*consumer, BeginRead(_, _)).WillOnce(Return(Result::kError));
  EXPECT_CALL(*fetch_data_loader_client, DidFetchDataLoadFailed());
  EXPECT_CALL(checkpoint, Call(3));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(4));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  ASSERT_TRUE(client);
  client->OnStateChange();
  checkpoint.Call(3);
  fetch_data_loader->Cancel();
  checkpoint.Call(4);
}

TEST(FetchDataLoaderTest, LoadAsStringCancel) {
  Checkpoint checkpoint;
  BytesConsumer::Client* client = nullptr;
  MockBytesConsumer* consumer = MockBytesConsumer::Create();

  FetchDataLoader* fetch_data_loader = FetchDataLoader::CreateLoaderAsString();
  MockFetchDataLoaderClient* fetch_data_loader_client =
      MockFetchDataLoaderClient::Create();

  InSequence s;
  EXPECT_CALL(checkpoint, Call(1));
  EXPECT_CALL(*consumer, SetClient(_)).WillOnce(SaveArg<0>(&client));
  EXPECT_CALL(*consumer, BeginRead(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                      Return(Result::kShouldWait)));
  EXPECT_CALL(checkpoint, Call(2));
  EXPECT_CALL(*consumer, Cancel());
  EXPECT_CALL(checkpoint, Call(3));

  checkpoint.Call(1);
  fetch_data_loader->Start(consumer, fetch_data_loader_client);
  checkpoint.Call(2);
  fetch_data_loader->Cancel();
  checkpoint.Call(3);
}

}  // namespace

}  // namespace blink
