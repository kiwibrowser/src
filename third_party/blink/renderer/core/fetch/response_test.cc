// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/response.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-blink.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_response.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer.h"
#include "third_party/blink/renderer/core/fetch/bytes_consumer_test_util.h"
#include "third_party/blink/renderer/core/fetch/data_consumer_handle_test_util.h"
#include "third_party/blink/renderer/core/fetch/fetch_response_data.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
namespace {

std::unique_ptr<WebServiceWorkerResponse> CreateTestWebServiceWorkerResponse() {
  const KURL url("http://www.webresponse.com/");
  const unsigned short kStatus = 200;
  const String status_text = "the best status text";
  struct {
    const char* key;
    const char* value;
  } headers[] = {{"cache-control", "no-cache"},
                 {"set-cookie", "foop"},
                 {"foo", "bar"},
                 {nullptr, nullptr}};
  Vector<WebURL> url_list;
  url_list.push_back(url);
  std::unique_ptr<WebServiceWorkerResponse> web_response =
      std::make_unique<WebServiceWorkerResponse>();
  web_response->SetURLList(url_list);
  web_response->SetStatus(kStatus);
  web_response->SetStatusText(status_text);
  web_response->SetResponseType(network::mojom::FetchResponseType::kDefault);
  for (int i = 0; headers[i].key; ++i) {
    web_response->SetHeader(WebString::FromUTF8(headers[i].key),
                            WebString::FromUTF8(headers[i].value));
  }
  return web_response;
}

TEST(ServiceWorkerResponseTest, FromFetchResponseData) {
  std::unique_ptr<DummyPageHolder> page =
      DummyPageHolder::Create(IntSize(1, 1));
  const KURL url("http://www.response.com");

  FetchResponseData* fetch_response_data = FetchResponseData::Create();
  Vector<KURL> url_list;
  url_list.push_back(url);
  fetch_response_data->SetURLList(url_list);
  Response* response =
      Response::Create(&page->GetDocument(), fetch_response_data);
  DCHECK(response);
  EXPECT_EQ(url, response->url());
}

TEST(ServiceWorkerResponseTest, FromWebServiceWorkerResponse) {
  V8TestingScope scope;
  std::unique_ptr<WebServiceWorkerResponse> web_response =
      CreateTestWebServiceWorkerResponse();
  Response* response = Response::Create(scope.GetScriptState(), *web_response);
  DCHECK(response);
  ASSERT_EQ(1u, web_response->UrlList().size());
  EXPECT_EQ(web_response->UrlList()[0], response->url());
  EXPECT_EQ(web_response->Status(), response->status());
  EXPECT_STREQ(web_response->StatusText().Utf8().c_str(),
               response->statusText().Utf8().data());

  Headers* response_headers = response->headers();

  WebVector<WebString> keys = web_response->GetHeaderKeys();
  EXPECT_EQ(keys.size(), response_headers->HeaderList()->size());
  for (size_t i = 0, max = keys.size(); i < max; ++i) {
    WebString key = keys[i];
    DummyExceptionStateForTesting exception_state;
    EXPECT_STREQ(web_response->GetHeader(key).Utf8().c_str(),
                 response_headers->get(key, exception_state).Utf8().data());
    EXPECT_FALSE(exception_state.HadException());
  }
}

TEST(ServiceWorkerResponseTest, FromWebServiceWorkerResponseDefault) {
  V8TestingScope scope;
  std::unique_ptr<WebServiceWorkerResponse> web_response =
      CreateTestWebServiceWorkerResponse();
  web_response->SetResponseType(network::mojom::FetchResponseType::kDefault);
  Response* response = Response::Create(scope.GetScriptState(), *web_response);

  Headers* response_headers = response->headers();
  DummyExceptionStateForTesting exception_state;
  EXPECT_STREQ(
      "foop",
      response_headers->get("set-cookie", exception_state).Utf8().data());
  EXPECT_STREQ("bar",
               response_headers->get("foo", exception_state).Utf8().data());
  EXPECT_STREQ(
      "no-cache",
      response_headers->get("cache-control", exception_state).Utf8().data());
  EXPECT_FALSE(exception_state.HadException());
}

TEST(ServiceWorkerResponseTest, FromWebServiceWorkerResponseBasic) {
  V8TestingScope scope;
  std::unique_ptr<WebServiceWorkerResponse> web_response =
      CreateTestWebServiceWorkerResponse();
  web_response->SetResponseType(network::mojom::FetchResponseType::kBasic);
  Response* response = Response::Create(scope.GetScriptState(), *web_response);

  Headers* response_headers = response->headers();
  DummyExceptionStateForTesting exception_state;
  EXPECT_STREQ(
      "", response_headers->get("set-cookie", exception_state).Utf8().data());
  EXPECT_STREQ("bar",
               response_headers->get("foo", exception_state).Utf8().data());
  EXPECT_STREQ(
      "no-cache",
      response_headers->get("cache-control", exception_state).Utf8().data());
  EXPECT_FALSE(exception_state.HadException());
}

TEST(ServiceWorkerResponseTest, FromWebServiceWorkerResponseCORS) {
  V8TestingScope scope;
  std::unique_ptr<WebServiceWorkerResponse> web_response =
      CreateTestWebServiceWorkerResponse();
  web_response->SetResponseType(network::mojom::FetchResponseType::kCORS);
  Response* response = Response::Create(scope.GetScriptState(), *web_response);

  Headers* response_headers = response->headers();
  DummyExceptionStateForTesting exception_state;
  EXPECT_STREQ(
      "", response_headers->get("set-cookie", exception_state).Utf8().data());
  EXPECT_STREQ("", response_headers->get("foo", exception_state).Utf8().data());
  EXPECT_STREQ(
      "no-cache",
      response_headers->get("cache-control", exception_state).Utf8().data());
  EXPECT_FALSE(exception_state.HadException());
}

TEST(ServiceWorkerResponseTest, FromWebServiceWorkerResponseOpaque) {
  V8TestingScope scope;
  std::unique_ptr<WebServiceWorkerResponse> web_response =
      CreateTestWebServiceWorkerResponse();
  web_response->SetResponseType(network::mojom::FetchResponseType::kOpaque);
  Response* response = Response::Create(scope.GetScriptState(), *web_response);

  Headers* response_headers = response->headers();
  DummyExceptionStateForTesting exception_state;
  EXPECT_STREQ(
      "", response_headers->get("set-cookie", exception_state).Utf8().data());
  EXPECT_STREQ("", response_headers->get("foo", exception_state).Utf8().data());
  EXPECT_STREQ(
      "",
      response_headers->get("cache-control", exception_state).Utf8().data());
  EXPECT_FALSE(exception_state.HadException());
}

void CheckResponseStream(ScriptState* script_state,
                         Response* response,
                         bool check_response_body_stream_buffer) {
  BodyStreamBuffer* original_internal = response->InternalBodyBuffer();
  if (check_response_body_stream_buffer) {
    EXPECT_EQ(response->BodyBuffer(), original_internal);
  } else {
    EXPECT_FALSE(response->BodyBuffer());
  }

  DummyExceptionStateForTesting exception_state;
  Response* cloned_response = response->clone(script_state, exception_state);
  EXPECT_FALSE(exception_state.HadException());

  if (!response->InternalBodyBuffer())
    FAIL() << "internalBodyBuffer() must not be null.";
  if (!cloned_response->InternalBodyBuffer())
    FAIL() << "internalBodyBuffer() must not be null.";
  EXPECT_TRUE(response->InternalBodyBuffer());
  EXPECT_TRUE(cloned_response->InternalBodyBuffer());
  EXPECT_TRUE(response->InternalBodyBuffer());
  EXPECT_TRUE(cloned_response->InternalBodyBuffer());
  EXPECT_NE(response->InternalBodyBuffer(), original_internal);
  EXPECT_NE(cloned_response->InternalBodyBuffer(), original_internal);
  EXPECT_NE(response->InternalBodyBuffer(),
            cloned_response->InternalBodyBuffer());
  if (check_response_body_stream_buffer) {
    EXPECT_EQ(response->BodyBuffer(), response->InternalBodyBuffer());
    EXPECT_EQ(cloned_response->BodyBuffer(),
              cloned_response->InternalBodyBuffer());
  } else {
    EXPECT_FALSE(response->BodyBuffer());
    EXPECT_FALSE(cloned_response->BodyBuffer());
  }
  BytesConsumerTestUtil::MockFetchDataLoaderClient* client1 =
      new BytesConsumerTestUtil::MockFetchDataLoaderClient();
  BytesConsumerTestUtil::MockFetchDataLoaderClient* client2 =
      new BytesConsumerTestUtil::MockFetchDataLoaderClient();
  EXPECT_CALL(*client1, DidFetchDataLoadedString(String("Hello, world")));
  EXPECT_CALL(*client2, DidFetchDataLoadedString(String("Hello, world")));

  response->InternalBodyBuffer()->StartLoading(
      FetchDataLoader::CreateLoaderAsString(), client1);
  cloned_response->InternalBodyBuffer()->StartLoading(
      FetchDataLoader::CreateLoaderAsString(), client2);
  blink::test::RunPendingTasks();
}

BodyStreamBuffer* CreateHelloWorldBuffer(ScriptState* script_state) {
  using BytesConsumerCommand = BytesConsumerTestUtil::Command;
  BytesConsumerTestUtil::ReplayingBytesConsumer* src =
      new BytesConsumerTestUtil::ReplayingBytesConsumer(
          ExecutionContext::From(script_state));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "Hello, "));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kData, "world"));
  src->Add(BytesConsumerCommand(BytesConsumerCommand::kDone));
  return new BodyStreamBuffer(script_state, src, nullptr);
}

TEST(ServiceWorkerResponseTest, BodyStreamBufferCloneDefault) {
  V8TestingScope scope;
  BodyStreamBuffer* buffer = CreateHelloWorldBuffer(scope.GetScriptState());
  FetchResponseData* fetch_response_data =
      FetchResponseData::CreateWithBuffer(buffer);
  Vector<KURL> url_list;
  url_list.push_back(KURL("http://www.response.com"));
  fetch_response_data->SetURLList(url_list);
  Response* response =
      Response::Create(scope.GetExecutionContext(), fetch_response_data);
  EXPECT_EQ(response->InternalBodyBuffer(), buffer);
  CheckResponseStream(scope.GetScriptState(), response, true);
}

TEST(ServiceWorkerResponseTest, BodyStreamBufferCloneBasic) {
  V8TestingScope scope;
  BodyStreamBuffer* buffer = CreateHelloWorldBuffer(scope.GetScriptState());
  FetchResponseData* fetch_response_data =
      FetchResponseData::CreateWithBuffer(buffer);
  Vector<KURL> url_list;
  url_list.push_back(KURL("http://www.response.com"));
  fetch_response_data->SetURLList(url_list);
  fetch_response_data = fetch_response_data->CreateBasicFilteredResponse();
  Response* response =
      Response::Create(scope.GetExecutionContext(), fetch_response_data);
  EXPECT_EQ(response->InternalBodyBuffer(), buffer);
  CheckResponseStream(scope.GetScriptState(), response, true);
}

TEST(ServiceWorkerResponseTest, BodyStreamBufferCloneCORS) {
  V8TestingScope scope;
  BodyStreamBuffer* buffer = CreateHelloWorldBuffer(scope.GetScriptState());
  FetchResponseData* fetch_response_data =
      FetchResponseData::CreateWithBuffer(buffer);
  Vector<KURL> url_list;
  url_list.push_back(KURL("http://www.response.com"));
  fetch_response_data->SetURLList(url_list);
  fetch_response_data = fetch_response_data->CreateCORSFilteredResponse({});
  Response* response =
      Response::Create(scope.GetExecutionContext(), fetch_response_data);
  EXPECT_EQ(response->InternalBodyBuffer(), buffer);
  CheckResponseStream(scope.GetScriptState(), response, true);
}

TEST(ServiceWorkerResponseTest, BodyStreamBufferCloneOpaque) {
  V8TestingScope scope;
  BodyStreamBuffer* buffer = CreateHelloWorldBuffer(scope.GetScriptState());
  FetchResponseData* fetch_response_data =
      FetchResponseData::CreateWithBuffer(buffer);
  Vector<KURL> url_list;
  url_list.push_back(KURL("http://www.response.com"));
  fetch_response_data->SetURLList(url_list);
  fetch_response_data = fetch_response_data->CreateOpaqueFilteredResponse();
  Response* response =
      Response::Create(scope.GetExecutionContext(), fetch_response_data);
  EXPECT_EQ(response->InternalBodyBuffer(), buffer);
  CheckResponseStream(scope.GetScriptState(), response, false);
}

TEST(ServiceWorkerResponseTest, BodyStreamBufferCloneError) {
  V8TestingScope scope;
  BodyStreamBuffer* buffer = new BodyStreamBuffer(
      scope.GetScriptState(),
      BytesConsumer::CreateErrored(BytesConsumer::Error()), nullptr);
  FetchResponseData* fetch_response_data =
      FetchResponseData::CreateWithBuffer(buffer);
  Vector<KURL> url_list;
  url_list.push_back(KURL("http://www.response.com"));
  fetch_response_data->SetURLList(url_list);
  Response* response =
      Response::Create(scope.GetExecutionContext(), fetch_response_data);
  DummyExceptionStateForTesting exception_state;
  Response* cloned_response =
      response->clone(scope.GetScriptState(), exception_state);
  EXPECT_FALSE(exception_state.HadException());

  BytesConsumerTestUtil::MockFetchDataLoaderClient* client1 =
      new BytesConsumerTestUtil::MockFetchDataLoaderClient();
  BytesConsumerTestUtil::MockFetchDataLoaderClient* client2 =
      new BytesConsumerTestUtil::MockFetchDataLoaderClient();
  EXPECT_CALL(*client1, DidFetchDataLoadFailed());
  EXPECT_CALL(*client2, DidFetchDataLoadFailed());

  response->InternalBodyBuffer()->StartLoading(
      FetchDataLoader::CreateLoaderAsString(), client1);
  cloned_response->InternalBodyBuffer()->StartLoading(
      FetchDataLoader::CreateLoaderAsString(), client2);
  blink::test::RunPendingTasks();
}

}  // namespace
}  // namespace blink
