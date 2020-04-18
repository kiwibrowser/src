// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_manager.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_request.h"
#include "third_party/blink/renderer/bindings/core/v8/dictionary.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/request_or_usv_string.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/bindings/modules/v8/request_or_usv_string_or_request_or_usv_string_sequence.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/fetch/request.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

class BackgroundFetchManagerTest : public testing::Test {
 protected:
  // Creates a vector of WebServiceWorkerRequest entries for the given
  // |requests| based on the |scope|. Proxied in the fixture to reduce the
  // number of friend declarations necessary in the BackgroundFetchManager.
  Vector<WebServiceWorkerRequest> CreateWebRequestVector(
      V8TestingScope& scope,
      const RequestOrUSVStringOrRequestOrUSVStringSequence& requests) {
    return BackgroundFetchManager::CreateWebRequestVector(
        scope.GetScriptState(), requests, scope.GetExceptionState());
  }

  // Returns a Dictionary object that represents a JavaScript dictionary with
  // a single key-value pair, where the key always is "method" with the value
  // set to |method|.
  Dictionary GetDictionaryForMethod(V8TestingScope& scope, const char* method) {
    v8::Isolate* isolate = scope.GetIsolate();
    v8::Local<v8::Object> data = v8::Object::New(isolate);

    data->Set(isolate->GetCurrentContext(), V8String(isolate, "method"),
              V8String(isolate, method))
        .ToChecked();

    return Dictionary(scope.GetIsolate(), data, scope.GetExceptionState());
  }
};

TEST_F(BackgroundFetchManagerTest, NullValue) {
  V8TestingScope scope;

  RequestOrUSVStringOrRequestOrUSVStringSequence requests;

  Vector<WebServiceWorkerRequest> web_requests =
      CreateWebRequestVector(scope, requests);
  ASSERT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(scope.GetExceptionState().Code(), kV8TypeError);
}

TEST_F(BackgroundFetchManagerTest, SingleUSVString) {
  V8TestingScope scope;

  KURL image_url("https://www.example.com/my_image.png");

  RequestOrUSVStringOrRequestOrUSVStringSequence requests =
      RequestOrUSVStringOrRequestOrUSVStringSequence::FromUSVString(
          image_url.GetString());

  Vector<WebServiceWorkerRequest> web_requests =
      CreateWebRequestVector(scope, requests);
  ASSERT_FALSE(scope.GetExceptionState().HadException());

  ASSERT_EQ(web_requests.size(), 1u);

  WebServiceWorkerRequest& web_request = web_requests[0];
  EXPECT_EQ(web_request.Url(), WebURL(image_url));
  EXPECT_EQ(web_request.Method(), "GET");
}

TEST_F(BackgroundFetchManagerTest, SingleRequest) {
  V8TestingScope scope;

  KURL image_url("https://www.example.com/my_image.png");

  Request* request = Request::Create(
      scope.GetScriptState(), image_url.GetString(),
      GetDictionaryForMethod(scope, "POST"), scope.GetExceptionState());
  ASSERT_FALSE(scope.GetExceptionState().HadException());
  ASSERT_TRUE(request);

  RequestOrUSVStringOrRequestOrUSVStringSequence requests =
      RequestOrUSVStringOrRequestOrUSVStringSequence::FromRequest(request);

  Vector<WebServiceWorkerRequest> web_requests =
      CreateWebRequestVector(scope, requests);
  ASSERT_FALSE(scope.GetExceptionState().HadException());

  ASSERT_EQ(web_requests.size(), 1u);

  WebServiceWorkerRequest& web_request = web_requests[0];
  EXPECT_EQ(web_request.Url(), WebURL(image_url));
  EXPECT_EQ(web_request.Method(), "POST");
}

TEST_F(BackgroundFetchManagerTest, Sequence) {
  V8TestingScope scope;

  KURL image_url("https://www.example.com/my_image.png");
  KURL icon_url("https://www.example.com/my_icon.jpg");
  KURL cat_video_url("https://www.example.com/my_cat_video.avi");

  RequestOrUSVString image_request =
      RequestOrUSVString::FromUSVString(image_url.GetString());
  RequestOrUSVString icon_request =
      RequestOrUSVString::FromUSVString(icon_url.GetString());

  Request* request = Request::Create(
      scope.GetScriptState(), cat_video_url.GetString(),
      GetDictionaryForMethod(scope, "DELETE"), scope.GetExceptionState());
  ASSERT_FALSE(scope.GetExceptionState().HadException());
  ASSERT_TRUE(request);

  RequestOrUSVString cat_video_request =
      RequestOrUSVString::FromRequest(request);

  HeapVector<RequestOrUSVString> request_sequence;
  request_sequence.push_back(image_request);
  request_sequence.push_back(icon_request);
  request_sequence.push_back(cat_video_request);

  RequestOrUSVStringOrRequestOrUSVStringSequence requests =
      RequestOrUSVStringOrRequestOrUSVStringSequence::
          FromRequestOrUSVStringSequence(request_sequence);

  Vector<WebServiceWorkerRequest> web_requests =
      CreateWebRequestVector(scope, requests);
  ASSERT_FALSE(scope.GetExceptionState().HadException());

  ASSERT_EQ(web_requests.size(), 3u);
  EXPECT_EQ(web_requests[0].Url(), WebURL(image_url));
  EXPECT_EQ(web_requests[0].Method(), "GET");

  EXPECT_EQ(web_requests[1].Url(), WebURL(icon_url));
  EXPECT_EQ(web_requests[1].Method(), "GET");

  EXPECT_EQ(web_requests[2].Url(), WebURL(cat_video_url));
  EXPECT_EQ(web_requests[2].Method(), "DELETE");
}

TEST_F(BackgroundFetchManagerTest, SequenceEmpty) {
  V8TestingScope scope;

  HeapVector<RequestOrUSVString> request_sequence;
  RequestOrUSVStringOrRequestOrUSVStringSequence requests =
      RequestOrUSVStringOrRequestOrUSVStringSequence::
          FromRequestOrUSVStringSequence(request_sequence);

  Vector<WebServiceWorkerRequest> web_requests =
      CreateWebRequestVector(scope, requests);
  ASSERT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(scope.GetExceptionState().Code(), kV8TypeError);
}

TEST_F(BackgroundFetchManagerTest, SequenceWithNullValue) {
  V8TestingScope scope;

  KURL image_url("https://www.example.com/my_image.png");

  RequestOrUSVString null_request;
  RequestOrUSVString image_request =
      RequestOrUSVString::FromUSVString(image_url.GetString());

  HeapVector<RequestOrUSVString> request_sequence;
  request_sequence.push_back(image_request);
  request_sequence.push_back(null_request);

  RequestOrUSVStringOrRequestOrUSVStringSequence requests =
      RequestOrUSVStringOrRequestOrUSVStringSequence::
          FromRequestOrUSVStringSequence(request_sequence);

  Vector<WebServiceWorkerRequest> web_requests =
      CreateWebRequestVector(scope, requests);
  ASSERT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(scope.GetExceptionState().Code(), kV8TypeError);
}

}  // namespace blink
