// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/media/router/discovery/dial/device_description_fetcher.h"
#include "chrome/browser/media/router/discovery/dial/dial_device_data.h"
#include "chrome/browser/media/router/test/test_helper.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace media_router {

class TestDeviceDescriptionFetcher : public DeviceDescriptionFetcher {
 public:
  TestDeviceDescriptionFetcher(
      const GURL& device_description_url,
      base::OnceCallback<void(const DialDeviceDescriptionData&)> success_cb,
      base::OnceCallback<void(const std::string&)> error_cb,
      network::TestURLLoaderFactory* factory)
      : DeviceDescriptionFetcher(device_description_url,
                                 std::move(success_cb),
                                 std::move(error_cb)),
        factory_(factory) {}
  ~TestDeviceDescriptionFetcher() override = default;

  void Start() override {
    fetcher_ = std::make_unique<TestDialURLFetcher>(
        base::BindOnce(&DeviceDescriptionFetcher::ProcessResponse,
                       base::Unretained(this)),
        base::BindOnce(&DeviceDescriptionFetcher::ReportError,
                       base::Unretained(this)),
        factory_);
    fetcher_->Get(device_description_url_);
  }

 private:
  std::vector<network::mojom::URLLoaderFactoryRequest> requests_;
  network::TestURLLoaderFactory* const factory_;
};

class DeviceDescriptionFetcherTest : public testing::Test {
 public:
  DeviceDescriptionFetcherTest() : url_("http://127.0.0.1/description.xml") {}

  void ExpectSuccess(const GURL& expected_app_url,
                     const std::string& expected_description) {
    expected_app_url_ = expected_app_url;
    expected_description_ = expected_description;
    EXPECT_CALL(*this, DoOnSuccess());
  }

  void ExpectError(const std::string expected_error) {
    expected_error_ = expected_error;
    EXPECT_CALL(*this, DoOnError());
  }

  void StartRequest() {
    description_fetcher_ = std::make_unique<TestDeviceDescriptionFetcher>(
        url_,
        base::BindOnce(&DeviceDescriptionFetcherTest::OnSuccess,
                       base::Unretained(this)),
        base::BindOnce(&DeviceDescriptionFetcherTest::OnError,
                       base::Unretained(this)),
        &loader_factory_);
    description_fetcher_->Start();
    base::RunLoop().RunUntilIdle();
  }

 protected:
  base::test::ScopedTaskEnvironment environment_;
  const GURL url_;
  network::TestURLLoaderFactory loader_factory_;
  base::OnceCallback<void(const DialDeviceDescriptionData&)> success_cb_;
  base::OnceCallback<void(const std::string&)> error_cb_;
  std::unique_ptr<TestDeviceDescriptionFetcher> description_fetcher_;
  GURL expected_app_url_;
  std::string expected_description_;
  std::string expected_error_;

 private:
  MOCK_METHOD0(DoOnSuccess, void());
  MOCK_METHOD0(DoOnError, void());

  void OnSuccess(const DialDeviceDescriptionData& description) {
    EXPECT_EQ(expected_app_url_, description.app_url);
    EXPECT_EQ(expected_description_, description.device_description);
    DoOnSuccess();
    description_fetcher_.reset();
  }

  void OnError(const std::string& message) {
    EXPECT_TRUE(message.find(expected_error_) != std::string::npos)
        << "[" << expected_error_ << "] not found in message [" << message
        << "]";
    DoOnError();
    description_fetcher_.reset();
  }

  DISALLOW_COPY_AND_ASSIGN(DeviceDescriptionFetcherTest);
};

TEST_F(DeviceDescriptionFetcherTest, FetchSuccessful) {
  std::string body("<xml>description</xml>");
  ExpectSuccess(GURL("http://127.0.0.1/apps"), body);
  network::ResourceResponseHead head;
  head.headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  head.headers->AddHeader("Application-URL: http://127.0.0.1/apps");
  network::URLLoaderCompletionStatus status;
  status.decoded_body_length = body.size();
  loader_factory_.AddResponse(url_, head, body, status);
  StartRequest();
}

TEST_F(DeviceDescriptionFetcherTest, FetchSuccessfulAppUrlWithTrailingSlash) {
  std::string body("<xml>description</xml>");
  ExpectSuccess(GURL("http://127.0.0.1/apps"), body);
  network::ResourceResponseHead head;
  head.headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  head.headers->AddHeader("Application-URL: http://127.0.0.1/apps/");
  network::URLLoaderCompletionStatus status;
  status.decoded_body_length = body.size();
  loader_factory_.AddResponse(url_, head, body, status);
  StartRequest();
}

TEST_F(DeviceDescriptionFetcherTest, FetchFailsOnMissingDescription) {
  ExpectError("404");
  loader_factory_.AddResponse(
      url_, network::ResourceResponseHead(), "",
      network::URLLoaderCompletionStatus(net::HTTP_NOT_FOUND));
  StartRequest();
}

TEST_F(DeviceDescriptionFetcherTest, FetchFailsOnMissingAppUrl) {
  std::string body("<xml>description</xml>");
  ExpectError("Missing or empty Application-URL:");
  network::URLLoaderCompletionStatus status;
  status.decoded_body_length = body.size();
  loader_factory_.AddResponse(url_, network::ResourceResponseHead(), body,
                              status);
  StartRequest();
}

TEST_F(DeviceDescriptionFetcherTest, FetchFailsOnEmptyAppUrl) {
  ExpectError("Missing or empty Application-URL:");
  std::string body("<xml>description</xml>");
  network::ResourceResponseHead head;
  head.headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  head.headers->AddHeader("Application-URL:");
  network::URLLoaderCompletionStatus status;
  status.decoded_body_length = body.size();
  loader_factory_.AddResponse(url_, head, body, status);
  StartRequest();
}

TEST_F(DeviceDescriptionFetcherTest, FetchFailsOnInvalidAppUrl) {
  ExpectError("Invalid Application-URL:");
  std::string body("<xml>description</xml>");
  network::ResourceResponseHead head;
  head.headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  head.headers->AddHeader("Application-URL: http://www.example.com");
  network::URLLoaderCompletionStatus status;
  status.decoded_body_length = body.size();
  loader_factory_.AddResponse(url_, head, body, status);
  StartRequest();
}

TEST_F(DeviceDescriptionFetcherTest, FetchFailsOnEmptyDescription) {
  ExpectError("Missing or empty response");
  network::ResourceResponseHead head;
  head.headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  head.headers->AddHeader("Application-URL: http://127.0.0.1/apps");

  loader_factory_.AddResponse(url_, head, "",
                              network::URLLoaderCompletionStatus());
  StartRequest();
}

TEST_F(DeviceDescriptionFetcherTest, FetchFailsOnBadDescription) {
  ExpectError("Invalid response encoding");
  std::string body("\xfc\x9c\xbf\x80\xbf\x80");
  network::ResourceResponseHead head;
  head.headers = base::MakeRefCounted<net::HttpResponseHeaders>("");
  head.headers->AddHeader("Application-URL: http://127.0.0.1/apps");
  network::URLLoaderCompletionStatus status;
  status.decoded_body_length = body.size();
  loader_factory_.AddResponse(url_, head, body, status);
  StartRequest();
}

}  // namespace media_router
