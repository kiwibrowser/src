// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/presentation/presentation_request.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {
namespace {

TEST(PresentationRequestTest, TestSingleUrlConstructor) {
  V8TestingScope scope;
  PresentationRequest* request = PresentationRequest::Create(
      scope.GetExecutionContext(), "https://example.com",
      scope.GetExceptionState());
  ASSERT_FALSE(scope.GetExceptionState().HadException());

  WTF::Vector<KURL> request_urls = request->Urls();
  EXPECT_EQ(static_cast<size_t>(1), request_urls.size());
  EXPECT_TRUE(request_urls[0].IsValid());
  EXPECT_EQ("https://example.com/", request_urls[0].GetString());
}

TEST(PresentationRequestTest, TestMultipleUrlConstructor) {
  V8TestingScope scope;
  WTF::Vector<String> urls;
  urls.push_back("https://example.com");
  urls.push_back("cast://deadbeef?param=foo");

  PresentationRequest* request = PresentationRequest::Create(
      scope.GetExecutionContext(), urls, scope.GetExceptionState());
  ASSERT_FALSE(scope.GetExceptionState().HadException());

  WTF::Vector<KURL> request_urls = request->Urls();
  EXPECT_EQ(static_cast<size_t>(2), request_urls.size());
  EXPECT_TRUE(request_urls[0].IsValid());
  EXPECT_EQ("https://example.com/", request_urls[0].GetString());
  EXPECT_TRUE(request_urls[1].IsValid());
  EXPECT_EQ("cast://deadbeef?param=foo", request_urls[1].GetString());
}

TEST(PresentationRequestTest, TestMultipleUrlConstructorInvalidUrl) {
  V8TestingScope scope;
  WTF::Vector<String> urls;
  urls.push_back("https://example.com");
  urls.push_back("");

  PresentationRequest::Create(scope.GetExecutionContext(), urls,
                              scope.GetExceptionState());
  EXPECT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(kSyntaxError, scope.GetExceptionState().Code());
}

TEST(PresentationRequestTest, TestMixedContentNotCheckedForNonHttpFamily) {
  V8TestingScope scope;
  scope.GetExecutionContext()->GetSecurityContext().SetSecurityOrigin(
      SecurityOrigin::CreateFromString("https://example.test"));

  PresentationRequest* request = PresentationRequest::Create(
      scope.GetExecutionContext(), "foo://bar", scope.GetExceptionState());
  ASSERT_FALSE(scope.GetExceptionState().HadException());

  WTF::Vector<KURL> request_urls = request->Urls();
  EXPECT_EQ(static_cast<size_t>(1), request_urls.size());
  EXPECT_TRUE(request_urls[0].IsValid());
  EXPECT_EQ("foo://bar", request_urls[0].GetString());
}

TEST(PresentationRequestTest, TestSingleUrlConstructorMixedContent) {
  V8TestingScope scope;
  scope.GetExecutionContext()->GetSecurityContext().SetSecurityOrigin(
      SecurityOrigin::CreateFromString("https://example.test"));

  PresentationRequest::Create(scope.GetExecutionContext(), "http://example.com",
                              scope.GetExceptionState());
  EXPECT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(kSecurityError, scope.GetExceptionState().Code());
}

TEST(PresentationRequestTest, TestMultipleUrlConstructorMixedContent) {
  V8TestingScope scope;
  scope.GetExecutionContext()->GetSecurityContext().SetSecurityOrigin(
      SecurityOrigin::CreateFromString("https://example.test"));

  WTF::Vector<String> urls;
  urls.push_back("http://example.com");
  urls.push_back("https://example1.com");

  PresentationRequest::Create(scope.GetExecutionContext(), urls,
                              scope.GetExceptionState());
  EXPECT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(kSecurityError, scope.GetExceptionState().Code());
}

TEST(PresentationRequestTest, TestMultipleUrlConstructorEmptySequence) {
  V8TestingScope scope;
  WTF::Vector<String> urls;

  PresentationRequest::Create(scope.GetExecutionContext(), urls,
                              scope.GetExceptionState());
  EXPECT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(kNotSupportedError, scope.GetExceptionState().Code());
}

}  // anonymous namespace
}  // namespace blink
