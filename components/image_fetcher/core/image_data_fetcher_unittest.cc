// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/image_fetcher/core/image_data_fetcher.h"

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "net/base/load_flags.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kImageURL[] = "http://www.example.com/image";
const char kURLResponseData[] = "EncodedImageData";

}  // namespace

namespace image_fetcher {

class ImageDataFetcherTest : public testing::Test {
 public:
  ImageDataFetcherTest()
      : test_request_context_getter_(
            new net::TestURLRequestContextGetter(message_loop_.task_runner())),
        image_data_fetcher_(test_request_context_getter_.get()) {}
  ~ImageDataFetcherTest() override {}

  MOCK_METHOD2(OnImageDataFetched,
               void(const std::string&, const RequestMetadata&));

  MOCK_METHOD2(OnImageDataFetchedFailedRequest,
               void(const std::string&, const RequestMetadata&));

  MOCK_METHOD2(OnImageDataFetchedMultipleRequests,
               void(const std::string&, const RequestMetadata&));

 protected:
  base::MessageLoop message_loop_;

  scoped_refptr<net::URLRequestContextGetter> test_request_context_getter_;

  ImageDataFetcher image_data_fetcher_;

  net::TestURLFetcherFactory fetcher_factory_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ImageDataFetcherTest);
};

TEST_F(ImageDataFetcherTest, FetchImageData) {
  image_data_fetcher_.FetchImageData(
      GURL(kImageURL),
      base::Bind(&ImageDataFetcherTest::OnImageDataFetched,
                 base::Unretained(this)),
      TRAFFIC_ANNOTATION_FOR_TESTS);

  RequestMetadata expected_metadata;
  expected_metadata.mime_type = std::string("image/png");
  expected_metadata.http_response_code = net::HTTP_OK;
  EXPECT_CALL(*this, OnImageDataFetched(std::string(kURLResponseData),
                                        expected_metadata));

  // Get and configure the TestURLFetcher.
  net::TestURLFetcher* test_url_fetcher =
      fetcher_factory_.GetFetcherByID(ImageDataFetcher::kFirstUrlFetcherId);
  ASSERT_NE(nullptr, test_url_fetcher);
  EXPECT_TRUE(test_url_fetcher->GetLoadFlags() & net::LOAD_DO_NOT_SEND_COOKIES);
  EXPECT_TRUE(test_url_fetcher->GetLoadFlags() & net::LOAD_DO_NOT_SAVE_COOKIES);
  EXPECT_TRUE(test_url_fetcher->GetLoadFlags() &
              net::LOAD_DO_NOT_SEND_AUTH_DATA);
  test_url_fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  test_url_fetcher->SetResponseString(kURLResponseData);
  test_url_fetcher->set_response_code(net::HTTP_OK);

  std::string raw_header =
      "HTTP/1.1 200 OK\n"
      "Content-type: image/png\n\n";
  std::replace(raw_header.begin(), raw_header.end(), '\n', '\0');
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(raw_header));
  test_url_fetcher->set_response_headers(headers);

  // Call the URLFetcher delegate to continue the test.
  test_url_fetcher->delegate()->OnURLFetchComplete(test_url_fetcher);
}

TEST_F(ImageDataFetcherTest, FetchImageData_NotFound) {
  image_data_fetcher_.FetchImageData(
      GURL(kImageURL),
      base::Bind(&ImageDataFetcherTest::OnImageDataFetched,
                 base::Unretained(this)),
      TRAFFIC_ANNOTATION_FOR_TESTS);

  RequestMetadata expected_metadata;
  expected_metadata.mime_type = std::string("image/png");
  expected_metadata.http_response_code = net::HTTP_NOT_FOUND;
  // For 404, expect an empty result even though correct image data is sent.
  EXPECT_CALL(*this, OnImageDataFetched(std::string(), expected_metadata));

  // Get and configure the TestURLFetcher.
  net::TestURLFetcher* test_url_fetcher =
      fetcher_factory_.GetFetcherByID(ImageDataFetcher::kFirstUrlFetcherId);
  ASSERT_NE(nullptr, test_url_fetcher);
  test_url_fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  test_url_fetcher->SetResponseString(kURLResponseData);

  std::string raw_header =
      "HTTP/1.1 404 Not Found\n"
      "Content-type: image/png\n\n";
  std::replace(raw_header.begin(), raw_header.end(), '\n', '\0');
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(raw_header));
  test_url_fetcher->set_response_headers(headers);

  // Call the URLFetcher delegate to continue the test.
  test_url_fetcher->delegate()->OnURLFetchComplete(test_url_fetcher);
}

TEST_F(ImageDataFetcherTest, FetchImageData_WithContentLocation) {
  image_data_fetcher_.FetchImageData(
      GURL(kImageURL),
      base::Bind(&ImageDataFetcherTest::OnImageDataFetched,
                 base::Unretained(this)),
      TRAFFIC_ANNOTATION_FOR_TESTS);

  RequestMetadata expected_metadata;
  expected_metadata.mime_type = std::string("image/png");
  expected_metadata.http_response_code = net::HTTP_NOT_FOUND;
  expected_metadata.content_location_header = "http://test-location/image.png";
  // For 404, expect an empty result even though correct image data is sent.
  EXPECT_CALL(*this, OnImageDataFetched(std::string(), expected_metadata));

  // Get and configure the TestURLFetcher.
  net::TestURLFetcher* test_url_fetcher =
      fetcher_factory_.GetFetcherByID(ImageDataFetcher::kFirstUrlFetcherId);
  ASSERT_NE(nullptr, test_url_fetcher);
  test_url_fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  test_url_fetcher->SetResponseString(kURLResponseData);

  std::string raw_header =
      "HTTP/1.1 404 Not Found\n"
      "Content-type: image/png\n"
      "Content-location: http://test-location/image.png\n\n";
  std::replace(raw_header.begin(), raw_header.end(), '\n', '\0');
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(raw_header));
  test_url_fetcher->set_response_headers(headers);

  // Call the URLFetcher delegate to continue the test.
  test_url_fetcher->delegate()->OnURLFetchComplete(test_url_fetcher);
}

TEST_F(ImageDataFetcherTest, FetchImageData_FailedRequest) {
  image_data_fetcher_.FetchImageData(
      GURL(kImageURL),
      base::Bind(&ImageDataFetcherTest::OnImageDataFetchedFailedRequest,
                 base::Unretained(this)),
      TRAFFIC_ANNOTATION_FOR_TESTS);

  RequestMetadata expected_metadata;
  expected_metadata.http_response_code = net::URLFetcher::RESPONSE_CODE_INVALID;
  EXPECT_CALL(
      *this, OnImageDataFetchedFailedRequest(std::string(), expected_metadata));

  // Get and configure the TestURLFetcher.
  net::TestURLFetcher* test_url_fetcher =
      fetcher_factory_.GetFetcherByID(ImageDataFetcher::kFirstUrlFetcherId);
  ASSERT_NE(nullptr, test_url_fetcher);
  test_url_fetcher->set_status(net::URLRequestStatus(
      net::URLRequestStatus::FAILED, net::ERR_INVALID_URL));

  // Call the URLFetcher delegate to continue the test.
  test_url_fetcher->delegate()->OnURLFetchComplete(test_url_fetcher);
}

TEST_F(ImageDataFetcherTest, FetchImageData_MultipleRequests) {
  ImageDataFetcher::ImageDataFetcherCallback callback =
      base::Bind(&ImageDataFetcherTest::OnImageDataFetchedMultipleRequests,
                 base::Unretained(this));
  EXPECT_CALL(*this, OnImageDataFetchedMultipleRequests(testing::_, testing::_))
      .Times(2);

  image_data_fetcher_.FetchImageData(GURL(kImageURL), callback,
                                     TRAFFIC_ANNOTATION_FOR_TESTS);
  image_data_fetcher_.FetchImageData(GURL(kImageURL), callback,
                                     TRAFFIC_ANNOTATION_FOR_TESTS);

  // Multiple calls to FetchImageData for the same URL will result in
  // multiple URLFetchers being created.
  net::TestURLFetcher* test_url_fetcher =
      fetcher_factory_.GetFetcherByID(ImageDataFetcher::kFirstUrlFetcherId);
  ASSERT_NE(nullptr, test_url_fetcher);
  test_url_fetcher->delegate()->OnURLFetchComplete(test_url_fetcher);

  test_url_fetcher =
      fetcher_factory_.GetFetcherByID(ImageDataFetcher::kFirstUrlFetcherId + 1);
  ASSERT_NE(nullptr, test_url_fetcher);
  test_url_fetcher->delegate()->OnURLFetchComplete(test_url_fetcher);
}

TEST_F(ImageDataFetcherTest, FetchImageData_CancelFetchIfImageExceedsMaxSize) {
  // In order to know whether the fetcher was canceled, it must notify about its
  // deletion.
  fetcher_factory_.set_remove_fetcher_on_delete(true);

  const int64_t kMaxDownloadBytes = 1024 * 1024;
  image_data_fetcher_.SetImageDownloadLimit(kMaxDownloadBytes);
  image_data_fetcher_.FetchImageData(
      GURL(kImageURL),
      base::Bind(&ImageDataFetcherTest::OnImageDataFetched,
                 base::Unretained(this)),
      TRAFFIC_ANNOTATION_FOR_TESTS);

  // Fetching an oversized image will behave like any other failed request.
  // There will be exactly one call to OnImageDataFetched containing a response
  // code that would be impossible for a completed fetch.
  RequestMetadata expected_metadata;
  expected_metadata.http_response_code = net::URLFetcher::RESPONSE_CODE_INVALID;
  EXPECT_CALL(*this, OnImageDataFetched(std::string(), expected_metadata));

  // Get and configure the TestURLFetcher.
  net::TestURLFetcher* test_url_fetcher =
      fetcher_factory_.GetFetcherByID(ImageDataFetcher::kFirstUrlFetcherId);
  ASSERT_NE(nullptr, test_url_fetcher);

  // Create a completely valid response that is never used. This is to make sure
  // that the answer isn't accidentally invalid but intentionally.
  test_url_fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  test_url_fetcher->SetResponseString(kURLResponseData);
  test_url_fetcher->set_response_code(net::HTTP_OK);
  std::string raw_header =
      "HTTP/1.1 200 OK\n"
      "Content-type: image/png\n\n";
  std::replace(raw_header.begin(), raw_header.end(), '\n', '\0');
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders(raw_header));
  test_url_fetcher->set_response_headers(headers);

  test_url_fetcher->delegate()->OnURLFetchDownloadProgress(
      test_url_fetcher,
      /*current=*/0,                 // Bytes received up to the call.
      /*total=*/-1,                  // not determined
      /*current_network_bytes=*/0);  // not relevant
  // The URL fetch should be running ...
  ASSERT_NE(nullptr, fetcher_factory_.GetFetcherByID(
                         ImageDataFetcher::kFirstUrlFetcherId));

  test_url_fetcher->delegate()->OnURLFetchDownloadProgress(
      test_url_fetcher,
      768 * 1024,  // Current bytes are not exeeding the limit.
      /*total=*/-1, /*current_network_bytes=*/0);
  // ... and running ...
  ASSERT_NE(nullptr, fetcher_factory_.GetFetcherByID(
                         ImageDataFetcher::kFirstUrlFetcherId));

  test_url_fetcher->delegate()->OnURLFetchDownloadProgress(
      test_url_fetcher, kMaxDownloadBytes,  // Still not exeeding the limit.
      /*total=*/-1, /*current_network_bytes=*/0);
  // ... and running ...
  ASSERT_NE(nullptr, fetcher_factory_.GetFetcherByID(
                         ImageDataFetcher::kFirstUrlFetcherId));

  test_url_fetcher->delegate()->OnURLFetchDownloadProgress(
      test_url_fetcher, kMaxDownloadBytes + 1,  // Limits are exceeded.
      /*total=*/-1, /*current_network_bytes=*/0);
  // ... and be canceled.
  EXPECT_EQ(nullptr, fetcher_factory_.GetFetcherByID(
                         ImageDataFetcher::kFirstUrlFetcherId));
}

}  // namespace image_fetcher
