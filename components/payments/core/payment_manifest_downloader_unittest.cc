// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_manifest_downloader.h"

#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

class PaymentMethodManifestDownloaderTest : public testing::Test {
 public:
  PaymentMethodManifestDownloaderTest()
      : context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())),
        downloader_(context_) {
    downloader_.DownloadPaymentMethodManifest(
        GURL("https://bobpay.com"),
        base::BindOnce(&PaymentMethodManifestDownloaderTest::OnManifestDownload,
                       base::Unretained(this)));
  }

  ~PaymentMethodManifestDownloaderTest() override {}

  MOCK_METHOD1(OnManifestDownload, void(const std::string& content));

  net::TestURLFetcher* fetcher() { return factory_.GetFetcherByID(0); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  net::TestURLFetcherFactory factory_;
  scoped_refptr<net::TestURLRequestContextGetter> context_;
  PaymentManifestDownloader downloader_;

  DISALLOW_COPY_AND_ASSIGN(PaymentMethodManifestDownloaderTest);
};

TEST_F(PaymentMethodManifestDownloaderTest, HttpHeadResponse404IsFailure) {
  fetcher()->set_response_code(404);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, NoHttpHeadersIsFailure) {
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, EmptyHttpHeaderIsFailure) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, EmptyHttpLinkHeaderIsFailure) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link:");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, NoRelInHttpLinkHeaderIsFailure) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, NoUrlInHttpLinkHeaderIsFailure) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest,
       NoManifestRellInHttpLinkHeaderIsFailure) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=web-app-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, HttpGetResponse404IsFailure) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());
  fetcher()->set_response_code(404);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, EmptyHttpGetResponseIsFailure) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, NonEmptyHttpGetResponseIsSuccess) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());
  fetcher()->SetResponseString("manifest content");
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload("manifest content"));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, HeaderResponseCode204IsSuccess) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  // HTTP code 204 means "no content", which is not a problem for an HTTP HEAD
  // request.
  fetcher()->set_response_code(204);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());
  fetcher()->SetResponseString("manifest content");
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload("manifest content"));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, RelativeHttpHeaderLinkUrl) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ("https://bobpay.com/manifest.json",
            fetcher()->GetOriginalURL().spec());
}

TEST_F(PaymentMethodManifestDownloaderTest, AbsoluteHttpsHeaderLinkUrl) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader(
      "Link: <https://alicepay.com/manifest.json>; "
      "rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ("https://alicepay.com/manifest.json",
            fetcher()->GetOriginalURL().spec());
}

TEST_F(PaymentMethodManifestDownloaderTest, AbsoluteHttpHeaderLinkUrl) {
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader(
      "Link: <http://alicepay.com/manifest.json>; "
      "rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, 300IsUnsupportedRedirect) {
  fetcher()->set_response_code(300);
  fetcher()->set_url(GURL("https://alicepay.com"));

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, 301And302AreSupportedRedirects) {
  fetcher()->set_response_code(301);
  fetcher()->set_url(GURL("https://alicepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ(fetcher()->GetOriginalURL(), GURL("https://alicepay.com"));

  fetcher()->set_response_code(302);
  fetcher()->set_url(GURL("https://charliepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ(fetcher()->GetOriginalURL(), GURL("https://charliepay.com"));

  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());
  fetcher()->SetResponseString("manifest content");
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload("manifest content"));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, 302And303AreSupportedRedirects) {
  fetcher()->set_response_code(302);
  fetcher()->set_url(GURL("https://alicepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ(fetcher()->GetOriginalURL(), GURL("https://alicepay.com"));

  fetcher()->set_response_code(303);
  fetcher()->set_url(GURL("https://charliepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ(fetcher()->GetOriginalURL(), GURL("https://charliepay.com"));

  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());
  fetcher()->SetResponseString("manifest content");
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload("manifest content"));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, 304IsUnsupportedRedirect) {
  fetcher()->set_response_code(304);
  fetcher()->set_url(GURL("https://alicepay.com"));

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, 305IsUnsupportedRedirect) {
  fetcher()->set_response_code(305);
  fetcher()->set_url(GURL("https://alicepay.com"));

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, 307And308AreSupportedRedirects) {
  fetcher()->set_response_code(307);
  fetcher()->set_url(GURL("https://alicepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ(fetcher()->GetOriginalURL(), GURL("https://alicepay.com"));

  fetcher()->set_response_code(308);
  fetcher()->set_url(GURL("https://charliepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ(fetcher()->GetOriginalURL(), GURL("https://charliepay.com"));

  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(std::string()));
  headers->AddHeader("Link: <manifest.json>; rel=payment-method-manifest");
  fetcher()->set_response_headers(headers);
  fetcher()->set_response_code(200);
  fetcher()->delegate()->OnURLFetchComplete(fetcher());
  fetcher()->SetResponseString("manifest content");
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload("manifest content"));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, NoMoreThanThreeRediects) {
  fetcher()->set_response_code(301);
  fetcher()->set_url(GURL("https://alicepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ(fetcher()->GetOriginalURL(), GURL("https://alicepay.com"));

  fetcher()->set_response_code(302);
  fetcher()->set_url(GURL("https://charliepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_EQ(fetcher()->GetOriginalURL(), GURL("https://charliepay.com"));

  fetcher()->set_response_code(308);
  fetcher()->set_url(GURL("https://davepay.com"));
  fetcher()->delegate()->OnURLFetchComplete(fetcher());

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(PaymentMethodManifestDownloaderTest, InvalidRedirectUrlIsFailure) {
  fetcher()->set_response_code(308);
  fetcher()->set_url(GURL("alicepay.com"));

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

class WebAppManifestDownloaderTest : public testing::Test {
 public:
  WebAppManifestDownloaderTest()
      : context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())),
        downloader_(context_) {
    downloader_.DownloadWebAppManifest(
        GURL("https://bobpay.com"),
        base::BindOnce(&WebAppManifestDownloaderTest::OnManifestDownload,
                       base::Unretained(this)));
  }

  ~WebAppManifestDownloaderTest() override {}

  MOCK_METHOD1(OnManifestDownload, void(const std::string& content));

  net::TestURLFetcher* fetcher() { return factory_.GetFetcherByID(0); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  net::TestURLFetcherFactory factory_;
  scoped_refptr<net::TestURLRequestContextGetter> context_;
  PaymentManifestDownloader downloader_;

  DISALLOW_COPY_AND_ASSIGN(WebAppManifestDownloaderTest);
};

TEST_F(WebAppManifestDownloaderTest, HttpGetResponse404IsFailure) {
  fetcher()->set_response_code(404);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(WebAppManifestDownloaderTest, EmptyHttpGetResponseIsFailure) {
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload(std::string()));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

TEST_F(WebAppManifestDownloaderTest, NonEmptyHttpGetResponseIsSuccess) {
  fetcher()->SetResponseString("manifest content");
  fetcher()->set_response_code(200);

  EXPECT_CALL(*this, OnManifestDownload("manifest content"));

  fetcher()->delegate()->OnURLFetchComplete(fetcher());
}

}  // namespace
}  // namespace payments
