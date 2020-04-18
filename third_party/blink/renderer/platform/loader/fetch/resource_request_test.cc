// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/platform/network/encoded_form_data.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

TEST(ResourceRequestTest, CrossThreadResourceRequestData) {
  ResourceRequest original;
  original.SetURL(KURL("http://www.example.com/test.htm"));
  original.SetCacheMode(mojom::FetchCacheMode::kDefault);
  original.SetTimeoutInterval(10);
  original.SetSiteForCookies(KURL("http://www.example.com/first_party.htm"));
  original.SetRequestorOrigin(
      SecurityOrigin::Create(KURL("http://www.example.com/first_party.htm")));
  original.SetHTTPMethod(HTTPNames::GET);
  original.SetHTTPHeaderField(AtomicString("Foo"), AtomicString("Bar"));
  original.SetHTTPHeaderField(AtomicString("Piyo"), AtomicString("Fuga"));
  original.SetPriority(ResourceLoadPriority::kLow, 20);

  scoped_refptr<EncodedFormData> original_body(
      EncodedFormData::Create("Test Body"));
  original.SetHTTPBody(original_body);
  original.SetAllowStoredCredentials(false);
  original.SetReportUploadProgress(false);
  original.SetHasUserGesture(false);
  original.SetDownloadToFile(false);
  original.SetSkipServiceWorker(false);
  original.SetFetchRequestMode(network::mojom::FetchRequestMode::kCORS);
  original.SetFetchCredentialsMode(
      network::mojom::FetchCredentialsMode::kSameOrigin);
  original.SetRequestorID(30);
  original.SetPluginChildID(40);
  original.SetAppCacheHostID(50);
  original.SetRequestContext(WebURLRequest::kRequestContextAudio);
  original.SetFrameType(network::mojom::RequestContextFrameType::kNested);
  original.SetHTTPReferrer(
      Referrer("http://www.example.com/referrer.htm", kReferrerPolicyDefault));

  EXPECT_STREQ("http://www.example.com/test.htm",
               original.Url().GetString().Utf8().data());
  EXPECT_EQ(mojom::FetchCacheMode::kDefault, original.GetCacheMode());
  EXPECT_EQ(10, original.TimeoutInterval());
  EXPECT_STREQ("http://www.example.com/first_party.htm",
               original.SiteForCookies().GetString().Utf8().data());
  EXPECT_STREQ("www.example.com",
               original.RequestorOrigin()->Host().Utf8().data());
  EXPECT_STREQ("GET", original.HttpMethod().Utf8().data());
  EXPECT_STREQ("Bar", original.HttpHeaderFields().Get("Foo").Utf8().data());
  EXPECT_STREQ("Fuga", original.HttpHeaderFields().Get("Piyo").Utf8().data());
  EXPECT_EQ(ResourceLoadPriority::kLow, original.Priority());
  EXPECT_STREQ("Test Body",
               original.HttpBody()->FlattenToString().Utf8().data());
  EXPECT_FALSE(original.AllowStoredCredentials());
  EXPECT_FALSE(original.ReportUploadProgress());
  EXPECT_FALSE(original.HasUserGesture());
  EXPECT_FALSE(original.DownloadToFile());
  EXPECT_FALSE(original.GetSkipServiceWorker());
  EXPECT_EQ(network::mojom::FetchRequestMode::kCORS,
            original.GetFetchRequestMode());
  EXPECT_EQ(network::mojom::FetchCredentialsMode::kSameOrigin,
            original.GetFetchCredentialsMode());
  EXPECT_EQ(30, original.RequestorID());
  EXPECT_EQ(40, original.GetPluginChildID());
  EXPECT_EQ(50, original.AppCacheHostID());
  EXPECT_EQ(WebURLRequest::kRequestContextAudio, original.GetRequestContext());
  EXPECT_EQ(network::mojom::RequestContextFrameType::kNested,
            original.GetFrameType());
  EXPECT_STREQ("http://www.example.com/referrer.htm",
               original.HttpReferrer().Utf8().data());
  EXPECT_EQ(kReferrerPolicyDefault, original.GetReferrerPolicy());

  std::unique_ptr<CrossThreadResourceRequestData> data1(original.CopyData());
  ResourceRequest copy1(data1.get());

  EXPECT_STREQ("http://www.example.com/test.htm",
               copy1.Url().GetString().Utf8().data());
  EXPECT_EQ(mojom::FetchCacheMode::kDefault, copy1.GetCacheMode());
  EXPECT_EQ(10, copy1.TimeoutInterval());
  EXPECT_STREQ("http://www.example.com/first_party.htm",
               copy1.SiteForCookies().GetString().Utf8().data());
  EXPECT_STREQ("www.example.com",
               copy1.RequestorOrigin()->Host().Utf8().data());
  EXPECT_STREQ("GET", copy1.HttpMethod().Utf8().data());
  EXPECT_STREQ("Bar", copy1.HttpHeaderFields().Get("Foo").Utf8().data());
  EXPECT_EQ(ResourceLoadPriority::kLow, copy1.Priority());
  EXPECT_STREQ("Test Body", copy1.HttpBody()->FlattenToString().Utf8().data());
  EXPECT_FALSE(copy1.AllowStoredCredentials());
  EXPECT_FALSE(copy1.ReportUploadProgress());
  EXPECT_FALSE(copy1.HasUserGesture());
  EXPECT_FALSE(copy1.DownloadToFile());
  EXPECT_FALSE(copy1.GetSkipServiceWorker());
  EXPECT_EQ(network::mojom::FetchRequestMode::kCORS,
            copy1.GetFetchRequestMode());
  EXPECT_EQ(network::mojom::FetchCredentialsMode::kSameOrigin,
            copy1.GetFetchCredentialsMode());
  EXPECT_EQ(30, copy1.RequestorID());
  EXPECT_EQ(40, copy1.GetPluginChildID());
  EXPECT_EQ(50, copy1.AppCacheHostID());
  EXPECT_EQ(WebURLRequest::kRequestContextAudio, copy1.GetRequestContext());
  EXPECT_EQ(network::mojom::RequestContextFrameType::kNested,
            copy1.GetFrameType());
  EXPECT_STREQ("http://www.example.com/referrer.htm",
               copy1.HttpReferrer().Utf8().data());
  EXPECT_EQ(kReferrerPolicyDefault, copy1.GetReferrerPolicy());

  copy1.SetAllowStoredCredentials(true);
  copy1.SetReportUploadProgress(true);
  copy1.SetHasUserGesture(true);
  copy1.SetDownloadToFile(true);
  copy1.SetSkipServiceWorker(true);
  copy1.SetFetchRequestMode(network::mojom::FetchRequestMode::kNoCORS);
  copy1.SetFetchCredentialsMode(network::mojom::FetchCredentialsMode::kInclude);

  std::unique_ptr<CrossThreadResourceRequestData> data2(copy1.CopyData());
  ResourceRequest copy2(data2.get());
  EXPECT_TRUE(copy2.AllowStoredCredentials());
  EXPECT_TRUE(copy2.ReportUploadProgress());
  EXPECT_TRUE(copy2.HasUserGesture());
  EXPECT_TRUE(copy2.DownloadToFile());
  EXPECT_TRUE(copy2.GetSkipServiceWorker());
  EXPECT_EQ(network::mojom::FetchRequestMode::kNoCORS,
            copy1.GetFetchRequestMode());
  EXPECT_EQ(network::mojom::FetchCredentialsMode::kInclude,
            copy1.GetFetchCredentialsMode());
}

TEST(ResourceRequestTest, SetHasUserGesture) {
  ResourceRequest original;
  EXPECT_FALSE(original.HasUserGesture());
  original.SetHasUserGesture(true);
  EXPECT_TRUE(original.HasUserGesture());
  original.SetHasUserGesture(false);
  EXPECT_TRUE(original.HasUserGesture());
}

TEST(ResourceRequestTest, SetIsAdResource) {
  ResourceRequest original;
  EXPECT_FALSE(original.IsAdResource());
  original.SetIsAdResource();
  EXPECT_TRUE(original.IsAdResource());

  // Should persist across redirects.
  std::unique_ptr<ResourceRequest> redirect_request =
      original.CreateRedirectRequest(
          KURL("https://example.test/redirect"), original.HttpMethod(),
          original.SiteForCookies(), original.HttpReferrer(),
          original.GetReferrerPolicy(), original.GetSkipServiceWorker());
  EXPECT_TRUE(redirect_request->IsAdResource());
}

}  // namespace blink
