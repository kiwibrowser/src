// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feed/core/feed_image_manager.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

using testing::_;

namespace feed {

namespace {
const char kImageURL[] = "http://pie.com/";
const char kImageURL2[] = "http://cake.com/";
const char kImageData[] = "pie image";
const char kImageData2[] = "cake image";

class FakeImageDecoder : public image_fetcher::ImageDecoder {
 public:
  void DecodeImage(
      const std::string& image_data,
      const gfx::Size& desired_image_frame_size,
      const image_fetcher::ImageDecodedCallback& callback) override {
    gfx::Image image;
    if (valid_ && !image_data.empty()) {
      ASSERT_EQ(image_data_, image_data);
      image = gfx::test::CreateImage();
    }
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindRepeating(callback, image));
  }
  void SetDecodingValid(bool valid) { valid_ = valid; }
  void SetExpectedData(std::string data) { image_data_ = data; }

 private:
  bool valid_ = true;
  std::string image_data_;
};

}  // namespace

class FeedImageManagerTest : public testing::Test {
 public:
  FeedImageManagerTest() : fake_url_fetcher_factory_(nullptr) {}

  ~FeedImageManagerTest() override {
    feed_image_manager_.reset();
    // We need to run until idle after deleting the database, because
    // ProtoDatabaseImpl deletes the actual LevelDB asynchronously.
    RunUntilIdle();
  }

  void SetUp() override {
    ASSERT_TRUE(database_dir_.CreateUniqueTempDir());

    std::unique_ptr<FeedImageDatabase> image_database =
        std::make_unique<FeedImageDatabase>(database_dir_.GetPath());
    image_database_ = image_database.get();

    request_context_getter_ = scoped_refptr<net::TestURLRequestContextGetter>(
        new net::TestURLRequestContextGetter(
            scoped_task_environment_.GetMainThreadTaskRunner()));

    auto decoder = std::make_unique<FakeImageDecoder>();
    decoder->SetExpectedData(kImageData);
    fake_image_decoder_ = decoder.get();

    feed_image_manager_ = std::make_unique<FeedImageManager>(
        std::make_unique<image_fetcher::ImageFetcherImpl>(
            std::move(decoder), request_context_getter_.get()),
        std::move(image_database));

    RunUntilIdle();

    ASSERT_TRUE(image_database_->IsInitialized());
  }

  void RunUntilIdle() { scoped_task_environment_.RunUntilIdle(); }

  FeedImageDatabase* image_database() { return image_database_; }

  FeedImageManager* feed_image_manager() { return feed_image_manager_.get(); }

  base::OneShotTimer& garbage_collection_timer() {
    return feed_image_manager_->garbage_collection_timer_;
  }

  void StartGarbageCollection() {
    feed_image_manager_->DoGarbageCollectionIfNeeded();
  }

  void StopGarbageCollection() { feed_image_manager_->StopGarbageCollection(); }

  FakeImageDecoder* fake_image_decoder() { return fake_image_decoder_; }

  net::FakeURLFetcherFactory* fake_url_fetcher_factory() {
    return &fake_url_fetcher_factory_;
  }

  MOCK_METHOD1(OnImageLoaded, void(std::string));

 private:
  std::unique_ptr<FeedImageManager> feed_image_manager_;
  FeedImageDatabase* image_database_;
  base::ScopedTempDir database_dir_;
  FakeImageDecoder* fake_image_decoder_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  net::FakeURLFetcherFactory fake_url_fetcher_factory_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(FeedImageManagerTest);
};

TEST_F(FeedImageManagerTest, FetchEmptyUrlVector) {
  base::MockCallback<ImageFetchedCallback> image_callback;

  // Make sure an empty image passed to callback.
  EXPECT_CALL(image_callback,
              Run(testing::Property(&gfx::Image::IsEmpty, testing::Eq(true))));
  feed_image_manager()->FetchImage(std::vector<std::string>(),
                                   image_callback.Get());

  RunUntilIdle();
}

TEST_F(FeedImageManagerTest, FetchImageFromCache) {
  // Save the image in the database.
  image_database()->SaveImage(kImageURL, kImageData);
  RunUntilIdle();

  base::MockCallback<ImageFetchedCallback> image_callback;
  EXPECT_CALL(image_callback,
              Run(testing::Property(&gfx::Image::IsEmpty, testing::Eq(false))));
  feed_image_manager()->FetchImage(std::vector<std::string>({kImageURL}),
                                   image_callback.Get());

  RunUntilIdle();
}

TEST_F(FeedImageManagerTest, FetchImagePopulatesCache) {
  // Expect the image to be fetched by URL.
  {
    fake_url_fetcher_factory()->SetFakeResponse(GURL(kImageURL), kImageData,
                                                net::HTTP_OK,
                                                net::URLRequestStatus::SUCCESS);
    base::MockCallback<ImageFetchedCallback> image_callback;
    EXPECT_CALL(image_callback, Run(testing::Property(&gfx::Image::IsEmpty,
                                                      testing::Eq(false))));
    feed_image_manager()->FetchImage(std::vector<std::string>({kImageURL}),
                                     image_callback.Get());

    RunUntilIdle();
  }
  // Make sure the image data is in the database.
  {
    EXPECT_CALL(*this, OnImageLoaded(kImageData));
    image_database()->LoadImage(
        kImageURL, base::BindOnce(&FeedImageManagerTest::OnImageLoaded,
                                  base::Unretained(this)));
    RunUntilIdle();
  }
  // Fetch again. The cache should be populated, no network request is needed.
  {
    fake_url_fetcher_factory()->ClearFakeResponses();
    base::MockCallback<ImageFetchedCallback> image_callback;
    EXPECT_CALL(image_callback, Run(testing::Property(&gfx::Image::IsEmpty,
                                                      testing::Eq(false))));
    feed_image_manager()->FetchImage(std::vector<std::string>({kImageURL}),
                                     image_callback.Get());

    RunUntilIdle();
  }
}

TEST_F(FeedImageManagerTest, FetchSecondImageIfFirstFailed) {
  // Expect the image to be fetched by URL.
  {
    fake_url_fetcher_factory()->SetFakeResponse(GURL(kImageURL), kImageData,
                                                net::HTTP_NOT_FOUND,
                                                net::URLRequestStatus::FAILED);
    fake_url_fetcher_factory()->SetFakeResponse(GURL(kImageURL2), kImageData2,
                                                net::HTTP_OK,
                                                net::URLRequestStatus::SUCCESS);
    base::MockCallback<ImageFetchedCallback> image_callback;
    EXPECT_CALL(image_callback, Run(testing::Property(&gfx::Image::IsEmpty,
                                                      testing::Eq(false))));
    fake_image_decoder()->SetExpectedData(kImageData2);
    feed_image_manager()->FetchImage(
        std::vector<std::string>({kImageURL, kImageURL2}),
        image_callback.Get());

    RunUntilIdle();
  }
  // Make sure the image data is in the database.
  {
    EXPECT_CALL(*this, OnImageLoaded(kImageData2));
    image_database()->LoadImage(
        kImageURL2, base::BindOnce(&FeedImageManagerTest::OnImageLoaded,
                                   base::Unretained(this)));
    RunUntilIdle();
  }
}

TEST_F(FeedImageManagerTest, DecodingErrorWillDeleteCache) {
  // Save the image in the database.
  image_database()->SaveImage(kImageURL, kImageData);
  RunUntilIdle();
  {
    fake_url_fetcher_factory()->SetFakeResponse(GURL(kImageURL), kImageData,
                                                net::HTTP_OK,
                                                net::URLRequestStatus::SUCCESS);
    // Set decoding always error.
    fake_image_decoder()->SetDecodingValid(false);
    base::MockCallback<ImageFetchedCallback> image_callback;

    EXPECT_CALL(image_callback, Run(testing::Property(&gfx::Image::IsEmpty,
                                                      testing::Eq(true))));
    feed_image_manager()->FetchImage(std::vector<std::string>({kImageURL}),
                                     image_callback.Get());

    RunUntilIdle();
  }
  // Make sure the image data was deleted from database.
  {
    EXPECT_CALL(*this, OnImageLoaded(std::string()));
    image_database()->LoadImage(
        kImageURL, base::BindOnce(&FeedImageManagerTest::OnImageLoaded,
                                  base::Unretained(this)));
    RunUntilIdle();
  }
}

TEST_F(FeedImageManagerTest, GarbageCollectionRunOnStart) {
  // Garbage Collection is scheduled by default.
  EXPECT_TRUE(garbage_collection_timer().IsRunning());

  StopGarbageCollection();
  EXPECT_FALSE(garbage_collection_timer().IsRunning());

  // StartGarbageCollection will start the garbage collection, but won't
  // schedule the next one.
  StartGarbageCollection();
  EXPECT_FALSE(garbage_collection_timer().IsRunning());

  // After finishing the garbage collection cycle, the next garbage collection
  // will be scheduled.
  RunUntilIdle();
  EXPECT_TRUE(garbage_collection_timer().IsRunning());
}

}  // namespace feed
