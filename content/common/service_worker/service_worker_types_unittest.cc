// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_types.h"
#include "base/guid.h"
#include "content/common/service_worker/service_worker_fetch_response_mojom_traits.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "mojo/public/cpp/test_support/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_response.mojom.h"
#include "url/mojom/url_gurl_mojom_traits.h"

#include "net/base/load_flags.h"

namespace content {

namespace {

using blink::mojom::FetchCacheMode;

TEST(ServiceWorkerFetchRequestTest, CacheModeTest) {
  EXPECT_EQ(FetchCacheMode::kDefault,
            ServiceWorkerFetchRequest::GetCacheModeFromLoadFlags(0));
  EXPECT_EQ(FetchCacheMode::kNoStore,
            ServiceWorkerFetchRequest::GetCacheModeFromLoadFlags(
                net::LOAD_DISABLE_CACHE));
  EXPECT_EQ(FetchCacheMode::kValidateCache,
            ServiceWorkerFetchRequest::GetCacheModeFromLoadFlags(
                net::LOAD_VALIDATE_CACHE));
  EXPECT_EQ(FetchCacheMode::kBypassCache,
            ServiceWorkerFetchRequest::GetCacheModeFromLoadFlags(
                net::LOAD_BYPASS_CACHE));
  EXPECT_EQ(FetchCacheMode::kForceCache,
            ServiceWorkerFetchRequest::GetCacheModeFromLoadFlags(
                net::LOAD_SKIP_CACHE_VALIDATION));
  EXPECT_EQ(FetchCacheMode::kOnlyIfCached,
            ServiceWorkerFetchRequest::GetCacheModeFromLoadFlags(
                net::LOAD_ONLY_FROM_CACHE | net::LOAD_SKIP_CACHE_VALIDATION));
  EXPECT_EQ(FetchCacheMode::kUnspecifiedOnlyIfCachedStrict,
            ServiceWorkerFetchRequest::GetCacheModeFromLoadFlags(
                net::LOAD_ONLY_FROM_CACHE));
  EXPECT_EQ(FetchCacheMode::kUnspecifiedForceCacheMiss,
            ServiceWorkerFetchRequest::GetCacheModeFromLoadFlags(
                net::LOAD_ONLY_FROM_CACHE | net::LOAD_BYPASS_CACHE));
}

// Tests that mojo serialization/deserialization of ServiceWorkerResponse works.
TEST(ServiceWorkerResponseTest, StructTraits) {
  ServiceWorkerResponse input;
  ServiceWorkerResponse output;

  input.url_list = {GURL("https://www.google.ca/"),
                    GURL("https://www.google.com")};
  input.status_code = 200;
  input.status_text = "status_text";
  input.response_type = network::mojom::FetchResponseType::kDefault;
  input.headers.insert(
      std::pair<std::string, std::string>("header1", "value1"));
  input.headers.insert(
      std::pair<std::string, std::string>("header2", "value2"));
  input.error = blink::mojom::ServiceWorkerResponseError::kUnknown;
  input.response_time = base::Time::Now();
  input.is_in_cache_storage = true;
  input.cache_storage_cache_name = "cache_name";

  mojo::test::SerializeAndDeserialize<blink::mojom::FetchAPIResponse>(&input,
                                                                      &output);

  EXPECT_EQ(input.url_list, output.url_list);
  EXPECT_EQ(input.status_code, output.status_code);
  EXPECT_EQ(input.status_text, output.status_text);
  EXPECT_EQ(input.response_type, output.response_type);
  EXPECT_EQ(input.headers, output.headers);
  EXPECT_EQ(input.blob, output.blob);
  EXPECT_EQ(input.error, output.error);
  EXPECT_EQ(input.response_time, output.response_time);
  EXPECT_EQ(input.is_in_cache_storage, output.is_in_cache_storage);
  EXPECT_EQ(input.cache_storage_cache_name, output.cache_storage_cache_name);
  EXPECT_EQ(input.cors_exposed_header_names, output.cors_exposed_header_names);
  EXPECT_EQ(input.side_data_blob, output.side_data_blob);
}

TEST(ServiceWorkerRequestTest, SerialiazeDeserializeRoundTrip) {
  ServiceWorkerFetchRequest request(
      GURL("foo.com"), "GET", {{"User-Agent", "Chrome"}},
      Referrer(
          GURL("bar.com"),
          blink::WebReferrerPolicy::kWebReferrerPolicyNoReferrerWhenDowngrade),
      true);
  request.mode = network::mojom::FetchRequestMode::kSameOrigin;
  request.is_main_resource_load = true;
  request.request_context_type =
      RequestContextType::REQUEST_CONTEXT_TYPE_IFRAME;
  request.credentials_mode = network::mojom::FetchCredentialsMode::kSameOrigin;
  request.cache_mode = blink::mojom::FetchCacheMode::kForceCache;
  request.redirect_mode = network::mojom::FetchRedirectMode::kManual;
  request.integrity = "integrity";
  request.keepalive = true;
  request.client_id = "42";

  EXPECT_EQ(request.Serialize(),
            ServiceWorkerFetchRequest::ParseFromString(request.Serialize())
                .Serialize());
}

}  // namespace

}  // namespace content
