// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feed/core/feed_image_database.h"

#include <map>

#include "base/test/scoped_task_environment.h"
#include "components/feed/core/proto/cached_image.pb.h"
#include "components/feed/core/time_serialization.h"
#include "components/leveldb_proto/testing/fake_db.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using leveldb_proto::test::FakeDB;
using testing::Mock;
using testing::_;

namespace feed {

namespace {
const std::string kImageURL = "http://pie.com/";
const std::string kImageData = "pie image";
}  // namespace

class FeedImageDatabaseTest : public testing::Test {
 public:
  FeedImageDatabaseTest() : image_db_(nullptr) {}

  void CreateDatabase() {
    // The FakeDBs are owned by |feed_db_|, so clear our pointers before
    // resetting |feed_db_| itself.
    image_db_ = nullptr;
    // Explicitly destroy any existing database before creating a new one.
    feed_db_.reset();

    auto image_db =
        std::make_unique<FakeDB<CachedImageProto>>(&image_db_storage_);

    image_db_ = image_db.get();
    feed_db_ = std::make_unique<FeedImageDatabase>(base::FilePath(),
                                                   std::move(image_db));
  }

  int64_t GetImageLastUsedTime(const std::string& url) {
    return image_db_storage_[kImageURL].last_used_time();
  }

  void InjectImageProto(const std::string& url,
                        const std::string& data,
                        base::Time time) {
    CachedImageProto image_proto;
    image_proto.set_url(url);
    image_proto.set_data(data);
    image_proto.set_last_used_time(ToDatabaseTime(time));
    image_db_storage_[url] = image_proto;
  }

  FakeDB<CachedImageProto>* image_db() { return image_db_; }

  FeedImageDatabase* db() { return feed_db_.get(); }

  void RunUntilIdle() { scoped_task_environment_.RunUntilIdle(); }

  MOCK_METHOD1(OnImageLoaded, void(std::string));
  MOCK_METHOD1(OnGarbageCollected, void(bool));

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::map<std::string, CachedImageProto> image_db_storage_;

  // Owned by |feed_db_|.
  FakeDB<CachedImageProto>* image_db_;

  std::unique_ptr<FeedImageDatabase> feed_db_;

  DISALLOW_COPY_AND_ASSIGN(FeedImageDatabaseTest);
};

TEST_F(FeedImageDatabaseTest, Init) {
  ASSERT_FALSE(db());

  CreateDatabase();
  EXPECT_FALSE(db()->IsInitialized());

  image_db()->InitCallback(true);

  EXPECT_TRUE(db()->IsInitialized());
}

TEST_F(FeedImageDatabaseTest, LoadBeforeInitSuccess) {
  CreateDatabase();
  EXPECT_FALSE(db()->IsInitialized());

  // Start an image load before the database is initialized.
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));

  EXPECT_CALL(*this, OnImageLoaded(_));

  image_db()->InitCallback(true);
  EXPECT_TRUE(db()->IsInitialized());
  image_db()->GetCallback(true);
}

TEST_F(FeedImageDatabaseTest, LoadBeforeInitFailed) {
  CreateDatabase();
  EXPECT_FALSE(db()->IsInitialized());

  // Start an image load before the database is initialized.
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));

  EXPECT_CALL(*this, OnImageLoaded(_));

  image_db()->InitCallback(false);
  EXPECT_FALSE(db()->IsInitialized());
  RunUntilIdle();
}

TEST_F(FeedImageDatabaseTest, LoadAfterInitSuccess) {
  CreateDatabase();
  EXPECT_FALSE(db()->IsInitialized());

  EXPECT_CALL(*this, OnImageLoaded(_)).Times(0);

  image_db()->InitCallback(true);
  EXPECT_TRUE(db()->IsInitialized());

  Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(*this, OnImageLoaded(_));
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));
  image_db()->GetCallback(true);
}

TEST_F(FeedImageDatabaseTest, LoadAfterInitFailed) {
  CreateDatabase();
  EXPECT_FALSE(db()->IsInitialized());

  EXPECT_CALL(*this, OnImageLoaded(_)).Times(0);

  image_db()->InitCallback(false);
  EXPECT_FALSE(db()->IsInitialized());

  Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(*this, OnImageLoaded(_));
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));
  RunUntilIdle();
}

TEST_F(FeedImageDatabaseTest, Save) {
  CreateDatabase();
  image_db()->InitCallback(true);
  ASSERT_TRUE(db()->IsInitialized());

  // Store an image.
  db()->SaveImage(kImageURL, kImageData);
  image_db()->UpdateCallback(true);

  // Make sure they're there.
  EXPECT_CALL(*this, OnImageLoaded(kImageData));
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));
  image_db()->GetCallback(true);
}

TEST_F(FeedImageDatabaseTest, SavePersist) {
  CreateDatabase();
  image_db()->InitCallback(true);
  ASSERT_TRUE(db()->IsInitialized());

  // Store an image.
  db()->SaveImage(kImageURL, kImageData);
  image_db()->UpdateCallback(true);

  // They should still exist after recreating the database.
  CreateDatabase();
  image_db()->InitCallback(true);
  ASSERT_TRUE(db()->IsInitialized());

  EXPECT_CALL(*this, OnImageLoaded(kImageData));
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));
  image_db()->GetCallback(true);
}

TEST_F(FeedImageDatabaseTest, LoadUpdatesTime) {
  CreateDatabase();
  image_db()->InitCallback(true);
  ASSERT_TRUE(db()->IsInitialized());

  // Store an image.
  InjectImageProto(kImageURL, kImageData, base::Time::UnixEpoch());

  int64_t old_time = GetImageLastUsedTime(kImageURL);
  // Make sure they're there.
  EXPECT_CALL(*this, OnImageLoaded(kImageData));
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));
  image_db()->GetCallback(true);
  image_db()->UpdateCallback(true);
  EXPECT_TRUE(old_time != GetImageLastUsedTime(kImageURL));
}

TEST_F(FeedImageDatabaseTest, Delete) {
  CreateDatabase();
  image_db()->InitCallback(true);
  ASSERT_TRUE(db()->IsInitialized());

  // Store the image.
  db()->SaveImage(kImageURL, kImageData);
  image_db()->UpdateCallback(true);

  // Make sure the image is there.
  EXPECT_CALL(*this, OnImageLoaded(kImageData));
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));
  image_db()->GetCallback(true);

  Mock::VerifyAndClearExpectations(this);

  // Delete the image.
  db()->DeleteImage(kImageURL);
  image_db()->UpdateCallback(true);

  // Make sure the image is gone.
  EXPECT_CALL(*this, OnImageLoaded(std::string()));
  db()->LoadImage(kImageURL,
                  base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                 base::Unretained(this)));
  image_db()->GetCallback(true);
}

TEST_F(FeedImageDatabaseTest, GarbageCollectImagesTest) {
  CreateDatabase();
  image_db()->InitCallback(true);
  ASSERT_TRUE(db()->IsInitialized());

  base::Time now = base::Time::Now();
  base::Time expired_time = now - base::TimeDelta::FromDays(30);
  base::Time very_old_time = now - base::TimeDelta::FromDays(100);

  // Store images.
  InjectImageProto("url1", "data1", very_old_time);
  InjectImageProto("url2", "data2", now);
  InjectImageProto("url3", "data3", very_old_time);

  // Garbage collect all except the second.
  EXPECT_CALL(*this, OnGarbageCollected(true));
  db()->GarbageCollectImages(
      expired_time, base::BindOnce(&FeedImageDatabaseTest::OnGarbageCollected,
                                   base::Unretained(this)));
  // This will first load all images, then delete the expired ones.
  image_db()->LoadCallback(true);
  image_db()->UpdateCallback(true);
  RunUntilIdle();

  // Make sure the images are gone.
  EXPECT_CALL(*this, OnImageLoaded(std::string()));
  db()->LoadImage("url1", base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                         base::Unretained(this)));
  image_db()->GetCallback(true);

  EXPECT_CALL(*this, OnImageLoaded(std::string()));
  db()->LoadImage("url3", base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                         base::Unretained(this)));
  image_db()->GetCallback(true);

  // Make sure the second still exists.
  EXPECT_CALL(*this, OnImageLoaded("data2"));
  db()->LoadImage("url2", base::BindOnce(&FeedImageDatabaseTest::OnImageLoaded,
                                         base::Unretained(this)));
  image_db()->GetCallback(true);
}

}  // namespace feed
