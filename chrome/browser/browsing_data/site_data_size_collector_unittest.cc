// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/site_data_size_collector.h"

#include "base/files/file_util.h"
#include "base/run_loop.h"
#include "chrome/browser/browsing_data/mock_browsing_data_appcache_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_cache_storage_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_channel_id_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_cookie_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_database_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_file_system_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_flash_lso_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_indexed_db_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_local_storage_helper.h"
#include "chrome/browser/browsing_data/mock_browsing_data_service_worker_helper.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/common/content_constants.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kCookieFileData[] = "cookie_file_data";
const char kChannelIdFileData[] = "channel_id_file_data";
const base::FilePath::CharType kFlashDataFilename0[] =
    FILE_PATH_LITERAL("flash_data_filename_0");
const base::FilePath::CharType kFlashDataFilename1[] =
    FILE_PATH_LITERAL("flash_data_filename_1");
const char kFlashData0[] = "flash_data_zero";
const char kFlashData1[] = "flash_data_one";

class SiteDataSizeCollectorTest : public testing::Test {
 public:
  ~SiteDataSizeCollectorTest() override {
    profile_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void SetUp() override {
    profile_.reset(new TestingProfile());
    mock_browsing_data_cookie_helper_ =
        new MockBrowsingDataCookieHelper(profile_->GetRequestContext());
    mock_browsing_data_database_helper_ =
        new MockBrowsingDataDatabaseHelper(profile_.get());
    mock_browsing_data_local_storage_helper_ =
        new MockBrowsingDataLocalStorageHelper(profile_.get());
    mock_browsing_data_appcache_helper_ =
        new MockBrowsingDataAppCacheHelper(profile_.get());
    mock_browsing_data_indexed_db_helper_ =
        new MockBrowsingDataIndexedDBHelper(profile_.get());
    mock_browsing_data_file_system_helper_ =
        new MockBrowsingDataFileSystemHelper(profile_.get());
    mock_browsing_data_channel_id_helper_ =
        new MockBrowsingDataChannelIDHelper();
    mock_browsing_data_service_worker_helper_ =
        new MockBrowsingDataServiceWorkerHelper(profile_.get());
    mock_browsing_data_cache_storage_helper_ =
        new MockBrowsingDataCacheStorageHelper(profile_.get());
    mock_browsing_data_flash_lso_helper_ =
        new MockBrowsingDataFlashLSOHelper(profile_.get());

    base::WriteFile(profile_->GetPath().Append(chrome::kCookieFilename),
                    kCookieFileData, arraysize(kCookieFileData));
    base::WriteFile(profile_->GetPath().Append(chrome::kChannelIDFilename),
                    kChannelIdFileData, arraysize(kChannelIdFileData));
    const base::FilePath flash_data_dir = profile_->GetPath().Append(
        content::kPepperDataDirname);
    base::CreateDirectory(flash_data_dir);
    base::WriteFile(flash_data_dir.Append(kFlashDataFilename0),
                    kFlashData0, arraysize(kFlashData0));
    base::WriteFile(flash_data_dir.Append(kFlashDataFilename1),
                    kFlashData1, arraysize(kFlashData1));

    fetched_size_ = -1;
  }

  void TearDown() override {
    mock_browsing_data_service_worker_helper_ = nullptr;
    mock_browsing_data_cache_storage_helper_ = nullptr;
    mock_browsing_data_channel_id_helper_ = nullptr;
    mock_browsing_data_file_system_helper_ = nullptr;
    mock_browsing_data_indexed_db_helper_ = nullptr;
    mock_browsing_data_appcache_helper_ = nullptr;
    mock_browsing_data_session_storage_helper_ = nullptr;
    mock_browsing_data_local_storage_helper_ = nullptr;
    mock_browsing_data_database_helper_ = nullptr;
    mock_browsing_data_flash_lso_helper_ = nullptr;
    base::RunLoop().RunUntilIdle();
  }

  void FetchCallback(int64_t size) {
    fetched_size_ = size;
  }

 protected:
  int64_t fetched_size_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
  scoped_refptr<MockBrowsingDataCookieHelper>
      mock_browsing_data_cookie_helper_;
  scoped_refptr<MockBrowsingDataDatabaseHelper>
      mock_browsing_data_database_helper_;
  scoped_refptr<MockBrowsingDataLocalStorageHelper>
      mock_browsing_data_local_storage_helper_;
  scoped_refptr<MockBrowsingDataLocalStorageHelper>
      mock_browsing_data_session_storage_helper_;
  scoped_refptr<MockBrowsingDataAppCacheHelper>
      mock_browsing_data_appcache_helper_;
  scoped_refptr<MockBrowsingDataIndexedDBHelper>
      mock_browsing_data_indexed_db_helper_;
  scoped_refptr<MockBrowsingDataFileSystemHelper>
      mock_browsing_data_file_system_helper_;
  scoped_refptr<MockBrowsingDataChannelIDHelper>
      mock_browsing_data_channel_id_helper_;
  scoped_refptr<MockBrowsingDataServiceWorkerHelper>
      mock_browsing_data_service_worker_helper_;
  scoped_refptr<MockBrowsingDataCacheStorageHelper>
      mock_browsing_data_cache_storage_helper_;
  scoped_refptr<MockBrowsingDataFlashLSOHelper>
      mock_browsing_data_flash_lso_helper_;
};

TEST_F(SiteDataSizeCollectorTest, FetchCookie) {
  SiteDataSizeCollector collector(
      profile_->GetPath(),
      mock_browsing_data_cookie_helper_.get(),
      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  // AddCookieSample() actually doesn't write the cookie to the file, only
  // triggers the condition to take the file into account.
  mock_browsing_data_cookie_helper_->AddCookieSamples(
      GURL("http://foo1"), "A=1");
  mock_browsing_data_cookie_helper_->Notify();
  // Wait until reading files on blocking pool finishes.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(static_cast<int64_t>(arraysize(kCookieFileData)), fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchCookieWithoutEntry) {
  SiteDataSizeCollector collector(
      profile_->GetPath(),
      mock_browsing_data_cookie_helper_.get(),
      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr);

  // Fetched size should be 0 if there are no cookies.
  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_cookie_helper_->Notify();
  EXPECT_EQ(0, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchDatabase) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr,
      mock_browsing_data_database_helper_.get(),
      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_database_helper_->AddDatabaseSamples();
  mock_browsing_data_database_helper_->Notify();
  EXPECT_EQ(3, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchLocalStorage) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr,
      mock_browsing_data_local_storage_helper_.get(),
      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_local_storage_helper_->AddLocalStorageSamples();
  mock_browsing_data_local_storage_helper_->Notify();
  EXPECT_EQ(3, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchAppCache) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr,
      mock_browsing_data_appcache_helper_.get(),
      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_appcache_helper_->AddAppCacheSamples();
  mock_browsing_data_appcache_helper_->Notify();
  EXPECT_EQ(6, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchIndexedDB) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr,
      mock_browsing_data_indexed_db_helper_.get(),
      nullptr, nullptr, nullptr, nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_indexed_db_helper_->AddIndexedDBSamples();
  mock_browsing_data_indexed_db_helper_->Notify();
  EXPECT_EQ(3, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchFileSystem) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr, nullptr,
      mock_browsing_data_file_system_helper_.get(),
      nullptr, nullptr, nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_file_system_helper_->AddFileSystemSamples();
  mock_browsing_data_file_system_helper_->Notify();
  EXPECT_EQ(14, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchChannelId) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      mock_browsing_data_channel_id_helper_.get(),
      nullptr, nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  // AddChannelIDSample() actually doesn't write channel ID to the file, only
  // triggers the condition to take the file into account.
  mock_browsing_data_channel_id_helper_->AddChannelIDSample("sbc1");
  mock_browsing_data_channel_id_helper_->Notify();
  // Wait until reading files on blocking pool finishes.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(static_cast<int64_t>(arraysize(kChannelIdFileData)), fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchChannelIdWithoutEntry) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      mock_browsing_data_channel_id_helper_.get(),
      nullptr, nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_channel_id_helper_->Notify();
  EXPECT_EQ(0, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchServiceWorker) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr,
      mock_browsing_data_service_worker_helper_.get(),
      nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_service_worker_helper_->AddServiceWorkerSamples();
  mock_browsing_data_service_worker_helper_->Notify();
  EXPECT_EQ(3, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchCacheStorage) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr,
      mock_browsing_data_cache_storage_helper_.get(),
      nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_cache_storage_helper_->AddCacheStorageSamples();
  mock_browsing_data_cache_storage_helper_->Notify();
  EXPECT_EQ(3, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchFlashLSO) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr,
      mock_browsing_data_flash_lso_helper_.get());

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  // AddFlashLSODomain() actually doesn't write flash data to the file, only
  // triggers the condition to take the file into account.
  mock_browsing_data_flash_lso_helper_->AddFlashLSODomain("example.com");
  mock_browsing_data_flash_lso_helper_->Notify();
  // Wait until reading files on blocking pool finishes.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(
      static_cast<int64_t>(arraysize(kFlashData0) + arraysize(kFlashData1)),
      fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchFlashLSOWithoutEntry) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr,
      mock_browsing_data_flash_lso_helper_.get());

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));
  mock_browsing_data_flash_lso_helper_->Notify();
  EXPECT_EQ(0, fetched_size_);
}

TEST_F(SiteDataSizeCollectorTest, FetchMultiple) {
  SiteDataSizeCollector collector(
      profile_->GetPath(), nullptr, nullptr, nullptr, nullptr,
      mock_browsing_data_indexed_db_helper_.get(), nullptr, nullptr,
      mock_browsing_data_service_worker_helper_.get(), nullptr, nullptr);

  collector.Fetch(base::Bind(&SiteDataSizeCollectorTest::FetchCallback,
                             base::Unretained(this)));

  mock_browsing_data_indexed_db_helper_->AddIndexedDBSamples();
  mock_browsing_data_indexed_db_helper_->Notify();
  // The callback for Fetch() shouldn't be called at this point.
  EXPECT_EQ(-1, fetched_size_);

  mock_browsing_data_service_worker_helper_->AddServiceWorkerSamples();
  mock_browsing_data_service_worker_helper_->Notify();
  EXPECT_EQ(3 + 3, fetched_size_);
}

}  // namespace
