// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/document_threadable_loader.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_cors.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_response.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"

namespace blink {

namespace {

TEST(DocumentThreadableLoaderCreatePreflightRequestTest, LexicographicalOrder) {
  ResourceRequest request;
  request.AddHTTPHeaderField("Orange", "Orange");
  request.AddHTTPHeaderField("Apple", "Red");
  request.AddHTTPHeaderField("Kiwifruit", "Green");
  request.AddHTTPHeaderField("Content-Type", "application/octet-stream");
  request.AddHTTPHeaderField("Strawberry", "Red");

  std::unique_ptr<ResourceRequest> preflight =
      DocumentThreadableLoader::CreateAccessControlPreflightRequestForTesting(
          request);

  EXPECT_EQ("apple,content-type,kiwifruit,orange,strawberry",
            preflight->HttpHeaderField("Access-Control-Request-Headers"));
}

TEST(DocumentThreadableLoaderCreatePreflightRequestTest, ExcludeSimpleHeaders) {
  ResourceRequest request;
  request.AddHTTPHeaderField("Accept", "everything");
  request.AddHTTPHeaderField("Accept-Language", "everything");
  request.AddHTTPHeaderField("Content-Language", "everything");
  request.AddHTTPHeaderField("Save-Data", "on");

  std::unique_ptr<ResourceRequest> preflight =
      DocumentThreadableLoader::CreateAccessControlPreflightRequestForTesting(
          request);

  // Do not emit empty-valued headers; an empty list of non-"CORS safelisted"
  // request headers should cause "Access-Control-Request-Headers:" to be
  // left out in the preflight request.
  EXPECT_EQ(g_null_atom,
            preflight->HttpHeaderField("Access-Control-Request-Headers"));
}

TEST(DocumentThreadableLoaderCreatePreflightRequestTest,
     ExcludeSimpleContentTypeHeader) {
  ResourceRequest request;
  request.AddHTTPHeaderField("Content-Type", "text/plain");

  std::unique_ptr<ResourceRequest> preflight =
      DocumentThreadableLoader::CreateAccessControlPreflightRequestForTesting(
          request);

  // Empty list also; see comment in test above.
  EXPECT_EQ(g_null_atom,
            preflight->HttpHeaderField("Access-Control-Request-Headers"));
}

TEST(DocumentThreadableLoaderCreatePreflightRequestTest,
     IncludeNonSimpleHeader) {
  ResourceRequest request;
  request.AddHTTPHeaderField("X-Custom-Header", "foobar");

  std::unique_ptr<ResourceRequest> preflight =
      DocumentThreadableLoader::CreateAccessControlPreflightRequestForTesting(
          request);

  EXPECT_EQ("x-custom-header",
            preflight->HttpHeaderField("Access-Control-Request-Headers"));
}

TEST(DocumentThreadableLoaderCreatePreflightRequestTest,
     IncludeNonSimpleContentTypeHeader) {
  ResourceRequest request;
  request.AddHTTPHeaderField("Content-Type", "application/octet-stream");

  std::unique_ptr<ResourceRequest> preflight =
      DocumentThreadableLoader::CreateAccessControlPreflightRequestForTesting(
          request);

  EXPECT_EQ("content-type",
            preflight->HttpHeaderField("Access-Control-Request-Headers"));
}

}  // namespace

}  // namespace blink
