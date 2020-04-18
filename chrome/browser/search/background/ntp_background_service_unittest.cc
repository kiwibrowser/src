// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/background/ntp_background_service.h"

#include <utility>
#include <vector>

#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/search/background/ntp_background_data.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::Eq;

namespace {

// The options to be added to end of an image URL, specifying resolution, etc.
constexpr char kImageOptions[] = "=imageOptions";

}  // namespace

class NtpBackgroundServiceTest : public testing::Test {
 public:
  NtpBackgroundServiceTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {}

  ~NtpBackgroundServiceTest() override {}

  void SetUp() override {
    testing::Test::SetUp();

    service_ = std::make_unique<NtpBackgroundService>(
        test_shared_loader_factory_, base::nullopt, base::nullopt,
        kImageOptions);
  }

  void SetUpResponseWithData(const GURL& load_url,
                             const std::string& response) {
    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {}));
    test_url_loader_factory_.AddResponse(load_url.spec(), response);
  }

  void SetUpResponseWithNetworkError(const GURL& load_url) {
    test_url_loader_factory_.AddResponse(
        load_url, network::ResourceResponseHead(), std::string(),
        network::URLLoaderCompletionStatus(net::HTTP_NOT_FOUND));
  }

  NtpBackgroundService* service() { return service_.get(); }

 private:
  // Required to run tests from UI and threads.
  content::TestBrowserThreadBundle thread_bundle_;

  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;

  std::unique_ptr<NtpBackgroundService> service_;
};

TEST_F(NtpBackgroundServiceTest, CollectionInfoNetworkError) {
  SetUpResponseWithNetworkError(service()->GetCollectionsLoadURLForTesting());

  ASSERT_TRUE(service()->collection_info().empty());

  base::RunLoop loop;
  service()->FetchCollectionInfo();
  loop.RunUntilIdle();

  ASSERT_TRUE(service()->collection_info().empty());
}

TEST_F(NtpBackgroundServiceTest, BadCollectionsResponse) {
  SetUpResponseWithData(service()->GetCollectionsLoadURLForTesting(),
                        "bad serialized GetCollectionsResponse");

  ASSERT_TRUE(service()->collection_info().empty());

  base::RunLoop loop;
  service()->FetchCollectionInfo();
  loop.RunUntilIdle();

  EXPECT_TRUE(service()->collection_info().empty());
}

TEST_F(NtpBackgroundServiceTest, GoodCollectionsResponse) {
  ntp::background::Collection collection;
  collection.set_collection_id("shapes");
  collection.set_collection_name("Shapes");
  collection.add_preview()->set_image_url("https://wallpapers.co/some_image");
  ntp::background::GetCollectionsResponse response;
  *response.add_collections() = collection;
  std::string response_string;
  response.SerializeToString(&response_string);

  SetUpResponseWithData(service()->GetCollectionsLoadURLForTesting(),
                        response_string);

  ASSERT_TRUE(service()->collection_info().empty());

  base::RunLoop loop;
  service()->FetchCollectionInfo();
  loop.RunUntilIdle();

  CollectionInfo collection_info;
  collection_info.collection_id = collection.collection_id();
  collection_info.collection_name = collection.collection_name();
  collection_info.preview_image_url = GURL(collection.preview(0).image_url());

  EXPECT_FALSE(service()->collection_info().empty());
  EXPECT_THAT(service()->collection_info().at(0), Eq(collection_info));
}

TEST_F(NtpBackgroundServiceTest, CollectionImagesNetworkError) {
  SetUpResponseWithNetworkError(service()->GetImagesURLForTesting());

  ASSERT_TRUE(service()->collection_images().empty());

  base::RunLoop loop;
  service()->FetchCollectionImageInfo("shapes");
  loop.RunUntilIdle();

  ASSERT_TRUE(service()->collection_images().empty());
}

TEST_F(NtpBackgroundServiceTest, BadCollectionImagesResponse) {
  SetUpResponseWithData(service()->GetImagesURLForTesting(),
                        "bad serialized GetImagesInCollectionResponse");

  ASSERT_TRUE(service()->collection_images().empty());

  base::RunLoop loop;
  service()->FetchCollectionImageInfo("shapes");
  loop.RunUntilIdle();

  EXPECT_TRUE(service()->collection_images().empty());
}

TEST_F(NtpBackgroundServiceTest, GoodCollectionImagesResponse) {
  ntp::background::Image image;
  image.set_asset_id(12345);
  image.set_image_url("https://wallpapers.co/some_image");
  image.add_attribution()->set_text("attribution text");
  ntp::background::GetImagesInCollectionResponse response;
  *response.add_images() = image;
  std::string response_string;
  response.SerializeToString(&response_string);

  SetUpResponseWithData(service()->GetImagesURLForTesting(), response_string);

  ASSERT_TRUE(service()->collection_images().empty());

  base::RunLoop loop;
  service()->FetchCollectionImageInfo("shapes");
  loop.RunUntilIdle();

  CollectionImage collection_image;
  collection_image.collection_id = "shapes";
  collection_image.asset_id = image.asset_id();
  collection_image.thumbnail_image_url = GURL(image.image_url());
  collection_image.image_url = GURL(image.image_url() + kImageOptions);
  collection_image.attribution.push_back(image.attribution(0).text());

  EXPECT_FALSE(service()->collection_images().empty());
  EXPECT_THAT(service()->collection_images().at(0), Eq(collection_image));
}

TEST_F(NtpBackgroundServiceTest, MultipleRequests) {
  ntp::background::Collection collection;
  collection.set_collection_id("shapes");
  collection.set_collection_name("Shapes");
  collection.add_preview()->set_image_url("https://wallpapers.co/some_image");
  ntp::background::GetCollectionsResponse collection_response;
  *collection_response.add_collections() = collection;
  std::string collection_response_string;
  collection_response.SerializeToString(&collection_response_string);

  ntp::background::Image image;
  image.set_asset_id(12345);
  image.set_image_url("https://wallpapers.co/some_image");
  image.add_attribution()->set_text("attribution text");
  ntp::background::GetImagesInCollectionResponse image_response;
  *image_response.add_images() = image;
  std::string image_response_string;
  image_response.SerializeToString(&image_response_string);

  SetUpResponseWithData(service()->GetCollectionsLoadURLForTesting(),
                        collection_response_string);
  SetUpResponseWithData(service()->GetImagesURLForTesting(),
                        image_response_string);

  ASSERT_TRUE(service()->collection_info().empty());
  ASSERT_TRUE(service()->collection_images().empty());

  base::RunLoop loop;
  service()->FetchCollectionInfo();
  service()->FetchCollectionImageInfo("shapes");
  // Subsequent requests are ignored while the loader is in use.
  service()->FetchCollectionImageInfo("colors");
  loop.RunUntilIdle();

  CollectionInfo collection_info;
  collection_info.collection_id = collection.collection_id();
  collection_info.collection_name = collection.collection_name();
  collection_info.preview_image_url = GURL(collection.preview(0).image_url());

  CollectionImage collection_image;
  collection_image.collection_id = "shapes";
  collection_image.asset_id = image.asset_id();
  collection_image.thumbnail_image_url = GURL(image.image_url());
  collection_image.image_url = GURL(image.image_url() + kImageOptions);
  collection_image.attribution.push_back(image.attribution(0).text());

  EXPECT_FALSE(service()->collection_info().empty());
  EXPECT_THAT(service()->collection_info().at(0), Eq(collection_info));
  EXPECT_FALSE(service()->collection_images().empty());
  EXPECT_THAT(service()->collection_images().at(0), Eq(collection_image));
}
