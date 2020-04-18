// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/thumbnail_source.h"

#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class ThumbnailSourceTest : public testing::Test {
 protected:
  ThumbnailSourceTest() {}
  ~ThumbnailSourceTest() override {}

  void SetUp() override {
    profile_manager_.reset(
        new TestingProfileManager(TestingBrowserProcess::GetGlobal()));
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("testing-profile");
  }

  Profile* profile() { return profile_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  TestingProfile* profile_;  // Owned by TestingProfileManager.
};

TEST_F(ThumbnailSourceTest, ExtractPageAndThumbnailUrlsValidCase) {
  ThumbnailSource thumbnail_source(profile(), false);

  std::string path = "http://www.abc.com/?fb=http://www.xyz.com/";

  GURL thumbnail_url_1, thumbnail_url_2;
  thumbnail_source.ExtractPageAndThumbnailUrls(path, &thumbnail_url_1,
                                               &thumbnail_url_2);

  EXPECT_EQ("http://www.abc.com/", thumbnail_url_1.spec());
  EXPECT_TRUE(thumbnail_url_1.is_valid());
  EXPECT_EQ("http://www.xyz.com/", thumbnail_url_2.spec());
  EXPECT_TRUE(thumbnail_url_2.is_valid());
}

TEST_F(ThumbnailSourceTest, ExtractPageAndThumbnailUrlsNoDelimiter) {
  ThumbnailSource thumbnail_source(profile(), false);

  std::string path = "http://www.abc.com/http://www.xyz.com/";

  GURL thumbnail_url_1, thumbnail_url_2;
  thumbnail_source.ExtractPageAndThumbnailUrls(path, &thumbnail_url_1,
                                               &thumbnail_url_2);

  EXPECT_EQ(path, thumbnail_url_1.spec());
  EXPECT_TRUE(thumbnail_url_2.is_empty());
}

TEST_F(ThumbnailSourceTest, ExtractPageAndThumbnailUrlsTwoDelimiters) {
  ThumbnailSource thumbnail_source(profile(), false);

  std::string path = "http://www.abc.com/?fb=?fb=http://www.xyz.com/";

  GURL thumbnail_url_1, thumbnail_url_2;
  thumbnail_source.ExtractPageAndThumbnailUrls(path, &thumbnail_url_1,
                                               &thumbnail_url_2);

  EXPECT_EQ("http://www.abc.com/", thumbnail_url_1.spec());
  EXPECT_TRUE(thumbnail_url_1.is_valid());
  EXPECT_FALSE(thumbnail_url_2.is_valid());
}

TEST_F(ThumbnailSourceTest, ExtractPageAndThumbnailUrlsInvalidFirstUrl) {
  ThumbnailSource thumbnail_source(profile(), false);

  std::string path = "http://!@#$%^&*()_+/?fb=http://www.xyz.com/";

  GURL thumbnail_url_1, thumbnail_url_2;
  thumbnail_source.ExtractPageAndThumbnailUrls(path, &thumbnail_url_1,
                                               &thumbnail_url_2);

  EXPECT_FALSE(thumbnail_url_1.is_valid());
  EXPECT_EQ("http://www.xyz.com/", thumbnail_url_2.spec());
  EXPECT_TRUE(thumbnail_url_2.is_valid());
}

TEST_F(ThumbnailSourceTest, ExtractPageAndThumbnailUrlsInvalidSecondUrl) {
  ThumbnailSource thumbnail_source(profile(), false);

  std::string path = "http://www.abc.com/?fb=http://!@#$%^&*()_+/";

  GURL thumbnail_url_1, thumbnail_url_2;
  thumbnail_source.ExtractPageAndThumbnailUrls(path, &thumbnail_url_1,
                                               &thumbnail_url_2);

  EXPECT_EQ("http://www.abc.com/", thumbnail_url_1.spec());
  EXPECT_TRUE(thumbnail_url_1.is_valid());
  EXPECT_FALSE(thumbnail_url_2.is_valid());
}
