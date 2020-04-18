// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_taskified.h"

#include <stdint.h>
#include <memory>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind_test_util.h"
#include "base/test/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_mock_time_task_runner.h"
#include "build/build_config.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/model/clear_storage_task.h"
#include "components/offline_pages/core/model/offline_page_item_generator.h"
#include "components/offline_pages/core/model/offline_page_model_utils.h"
#include "components/offline_pages/core/model/offline_page_test_utils.h"
#include "components/offline_pages/core/model/persistent_page_consistency_check_task.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_test_archiver.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "components/offline_pages/core/stub_system_download_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::_;
using testing::A;
using testing::An;
using testing::ElementsAre;
using testing::Eq;
using testing::IsEmpty;
using testing::Pointee;
using testing::SaveArg;
using testing::UnorderedElementsAre;

namespace offline_pages {

using ArchiverResult = OfflinePageArchiver::ArchiverResult;
using ClearStorageResult = ClearStorageTask::ClearStorageResult;

namespace {
const GURL kTestUrl("http://example.com");
const GURL kTestUrl2("http://other.page.com");
const GURL kTestUrlWithFragment("http://example.com#frag");
const GURL kTestUrl2WithFragment("http://other.page.com#frag");
const GURL kOtherUrl("http://foo");
const GURL kFileUrl("file:///foo");
const ClientId kTestClientId1(kDefaultNamespace, "1234");
const ClientId kTestClientId2(kDefaultNamespace, "5678");
const ClientId kTestUserRequestedClientId(kDownloadNamespace, "714");
const ClientId kTestBrowserActionsClientId(kBrowserActionsNamespace, "999");
const int64_t kTestFileSize = 876543LL;
const base::string16 kTestTitle = base::UTF8ToUTF16("a title");
const std::string kTestRequestOrigin("abc.xyz");
const std::string kEmptyRequestOrigin;
const std::string kTestDigest("test digest");
const int64_t kDownloadId = 42LL;

}  // namespace

class OfflinePageModelTaskifiedTest : public testing::Test,
                                      public OfflinePageModel::Observer,
                                      public OfflinePageTestArchiver::Observer {
 public:
  OfflinePageModelTaskifiedTest();
  ~OfflinePageModelTaskifiedTest() override;

  void SetUp() override;
  void TearDown() override;

  // Runs until all of the tasks that are not delayed are gone from the task
  // queue.
  void PumpLoop() { task_runner_->RunUntilIdle(); }
  void BuildStore();
  void BuildModel();
  void ResetModel();
  void ResetResults();

  // OfflinePageModel::Observer implementation.
  void OfflinePageModelLoaded(OfflinePageModel* model) override;
  void OfflinePageAdded(OfflinePageModel* model,
                        const OfflinePageItem& added_page) override;
  void OfflinePageDeleted(
      const OfflinePageModel::DeletedPageInfo& page_info) override;
  MOCK_METHOD2(ThumbnailAdded,
               void(OfflinePageModel* model,
                    const OfflinePageThumbnail& added_thumbnail));

  // OfflinePageTestArchiver::Observer implementation.
  void SetLastPathCreatedByArchiver(const base::FilePath& file_path) override;

  // Saves a page which will create the file and insert the corresponding
  // metadata into store. It relies on the implementation of
  // OfflinePageModel::SavePage.
  void SavePageWithCallback(const GURL& url,
                            const ClientId& client_id,
                            const GURL& original_url,
                            const std::string& request_origin,
                            std::unique_ptr<OfflinePageArchiver> archiver,
                            const SavePageCallback& callback);
  int64_t SavePageWithExpectedResult(
      const GURL& url,
      const ClientId& client_id,
      const GURL& original_url,
      const std::string& request_origin,
      std::unique_ptr<OfflinePageArchiver> archiver,
      SavePageResult expected_result);
  // Insert an offline page in to store, it does not rely on the model
  // implementation.
  void InsertPageIntoStore(const OfflinePageItem& offline_page);
  std::unique_ptr<OfflinePageTestArchiver> BuildArchiver(const GURL& url,
                                                         ArchiverResult result);
  void CheckTaskQueueIdle();

  // Getters for private fields.
  base::TestMockTimeTaskRunner* task_runner() { return task_runner_.get(); }
  OfflinePageModelTaskified* model() { return model_.get(); }
  OfflinePageMetadataStoreSQL* store() { return store_test_util_.store(); }
  OfflinePageMetadataStoreTestUtil* store_test_util() {
    return &store_test_util_;
  }
  StubSystemDownloadManager* download_manager_stub() {
    return download_manager_stub_;
  }
  OfflinePageItemGenerator* page_generator() { return &generator_; }
  TaskQueue* task_queue() { return &model_->task_queue_; }
  base::HistogramTester* histogram_tester() { return histogram_tester_.get(); }
  const base::FilePath& temporary_dir_path() {
    return temporary_dir_.GetPath();
  }
  const base::FilePath& private_archive_dir_path() {
    return private_archive_dir_.GetPath();
  }
  const base::FilePath& public_archive_dir_path() {
    return public_archive_dir_.GetPath();
  }

  const base::FilePath& last_path_created_by_archiver() {
    return last_path_created_by_archiver_;
  }
  bool observer_add_page_called() { return observer_add_page_called_; }
  const OfflinePageItem& last_added_page() { return last_added_page_; }
  bool observer_delete_page_called() { return observer_delete_page_called_; }
  const OfflinePageModel::DeletedPageInfo& last_deleted_page_info() {
    return last_deleted_page_info_;
  }
  base::Time last_maintenance_tasks_schedule_time() {
    return model_->last_maintenance_tasks_schedule_time_;
  }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  std::unique_ptr<OfflinePageModelTaskified> model_;
  OfflinePageMetadataStoreTestUtil store_test_util_;
  StubSystemDownloadManager* download_manager_stub_;
  OfflinePageItemGenerator generator_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  base::ScopedTempDir temporary_dir_;
  base::ScopedTempDir private_archive_dir_;
  base::ScopedTempDir public_archive_dir_;

  base::FilePath last_path_created_by_archiver_;
  bool observer_add_page_called_;
  OfflinePageItem last_added_page_;
  bool observer_delete_page_called_;
  OfflinePageModel::DeletedPageInfo last_deleted_page_info_;
};

OfflinePageModelTaskifiedTest::OfflinePageModelTaskifiedTest()
    : task_runner_(new base::TestMockTimeTaskRunner(base::Time::Now(),
                                                    base::TimeTicks::Now())),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_) {}

OfflinePageModelTaskifiedTest::~OfflinePageModelTaskifiedTest() {}

void OfflinePageModelTaskifiedTest::SetUp() {
  BuildStore();
  ASSERT_TRUE(temporary_dir_.CreateUniqueTempDir());
  ASSERT_TRUE(private_archive_dir_.CreateUniqueTempDir());
  ASSERT_TRUE(public_archive_dir_.CreateUniqueTempDir());
  BuildModel();
  PumpLoop();
  CheckTaskQueueIdle();
  histogram_tester_ = std::make_unique<base::HistogramTester>();
}

void OfflinePageModelTaskifiedTest::TearDown() {
  SCOPED_TRACE("in TearDown");
  CheckTaskQueueIdle();
  store_test_util_.DeleteStore();
  if (temporary_dir_.IsValid()) {
    if (!temporary_dir_.Delete())
      DLOG(ERROR) << "temporary_dir_ not created";
  }
  if (private_archive_dir_.IsValid()) {
    if (!private_archive_dir_.Delete())
      DLOG(ERROR) << "private_persistent_dir not created";
  }
  if (public_archive_dir_.IsValid()) {
    if (!public_archive_dir_.Delete())
      DLOG(ERROR) << "public_archive_dir not created";
  }
  EXPECT_EQ(0UL, model_->pending_archivers_.size());
  model_->RemoveObserver(this);
  model_.reset();
  PumpLoop();
}

void OfflinePageModelTaskifiedTest::BuildStore() {
  store_test_util()->BuildStore();
}

void OfflinePageModelTaskifiedTest::BuildModel() {
  ASSERT_TRUE(store_test_util_.store());
  // Keep a copy of the system download manager stub to test against.
  download_manager_stub_ = new StubSystemDownloadManager(kDownloadId, true);
  auto archive_manager = std::make_unique<ArchiveManager>(
      temporary_dir_path(), private_archive_dir_path(),
      public_archive_dir_path(), base::ThreadTaskRunnerHandle::Get());
  std::unique_ptr<SystemDownloadManager> download_manager(
      download_manager_stub_);
  model_ = std::make_unique<OfflinePageModelTaskified>(
      store_test_util()->ReleaseStore(), std::move(archive_manager),
      std::move(download_manager), base::ThreadTaskRunnerHandle::Get(),
      task_runner_->GetMockClock());
  model_->AddObserver(this);
  histogram_tester_ = std::make_unique<base::HistogramTester>();
  ResetResults();
  EXPECT_EQ(0UL, model_->pending_archivers_.size());
}

void OfflinePageModelTaskifiedTest::ResetModel() {
  model_.reset();
  PumpLoop();
}

void OfflinePageModelTaskifiedTest::ResetResults() {
  last_path_created_by_archiver_.clear();
  observer_add_page_called_ = false;
  observer_delete_page_called_ = false;
}

void OfflinePageModelTaskifiedTest::OfflinePageModelLoaded(
    OfflinePageModel* model) {}

void OfflinePageModelTaskifiedTest::OfflinePageAdded(
    OfflinePageModel* model,
    const OfflinePageItem& added_page) {
  observer_add_page_called_ = true;
  last_added_page_ = added_page;
}

void OfflinePageModelTaskifiedTest::OfflinePageDeleted(
    const OfflinePageModel::DeletedPageInfo& page_info) {
  observer_delete_page_called_ = true;
  last_deleted_page_info_ = page_info;
}

void OfflinePageModelTaskifiedTest::SetLastPathCreatedByArchiver(
    const base::FilePath& file_path) {
  last_path_created_by_archiver_ = file_path;
}

void OfflinePageModelTaskifiedTest::SavePageWithCallback(
    const GURL& url,
    const ClientId& client_id,
    const GURL& original_url,
    const std::string& request_origin,
    std::unique_ptr<OfflinePageArchiver> archiver,
    const SavePageCallback& callback) {
  OfflinePageModel::SavePageParams save_page_params;
  save_page_params.url = url;
  save_page_params.client_id = client_id;
  save_page_params.original_url = original_url;
  save_page_params.request_origin = request_origin;
  save_page_params.is_background = false;
  model()->SavePage(save_page_params, std::move(archiver), nullptr, callback);
  PumpLoop();
}

int64_t OfflinePageModelTaskifiedTest::SavePageWithExpectedResult(
    const GURL& url,
    const ClientId& client_id,
    const GURL& original_url,
    const std::string& request_origin,
    std::unique_ptr<OfflinePageArchiver> archiver,
    SavePageResult expected_result) {
  int64_t offline_id = OfflinePageModel::kInvalidOfflineId;
  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(expected_result), A<int64_t>()))
      .WillOnce(SaveArg<1>(&offline_id));
  SavePageWithCallback(url, client_id, original_url, request_origin,
                       std::move(archiver), callback.Get());
  if (expected_result == SavePageResult::SUCCESS) {
    EXPECT_NE(OfflinePageModel::kInvalidOfflineId, offline_id);
  }
  return offline_id;
}

void OfflinePageModelTaskifiedTest::InsertPageIntoStore(
    const OfflinePageItem& offline_page) {
  store_test_util()->InsertItem(offline_page);
}

std::unique_ptr<OfflinePageTestArchiver>
OfflinePageModelTaskifiedTest::BuildArchiver(const GURL& url,
                                             ArchiverResult result) {
  return std::make_unique<OfflinePageTestArchiver>(
      this, url, result, kTestTitle, kTestFileSize, kTestDigest,
      base::ThreadTaskRunnerHandle::Get());
}

void OfflinePageModelTaskifiedTest::CheckTaskQueueIdle() {
  EXPECT_FALSE(task_queue()->HasPendingTasks());
  EXPECT_FALSE(task_queue()->HasRunningTask());
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageSuccessful) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t offline_id = SavePageWithExpectedResult(
      kTestUrl, kTestClientId1, kTestUrl2, kEmptyRequestOrigin,
      std::move(archiver), SavePageResult::SUCCESS);

  EXPECT_EQ(1UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  auto saved_page_ptr = store_test_util()->GetPageByOfflineId(offline_id);
  ASSERT_TRUE(saved_page_ptr);

  EXPECT_EQ(kTestUrl, saved_page_ptr->url);
  EXPECT_EQ(kTestClientId1.id, saved_page_ptr->client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, saved_page_ptr->client_id.name_space);
  EXPECT_EQ(last_path_created_by_archiver(), saved_page_ptr->file_path);
  EXPECT_EQ(kTestFileSize, saved_page_ptr->file_size);
  EXPECT_EQ(0, saved_page_ptr->access_count);
  EXPECT_EQ(0, saved_page_ptr->flags);
  EXPECT_EQ(kTestTitle, saved_page_ptr->title);
  EXPECT_EQ(kTestUrl2, saved_page_ptr->original_url);
  EXPECT_EQ("", saved_page_ptr->request_origin);
  EXPECT_EQ(kTestDigest, saved_page_ptr->digest);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(
          model_utils::ToNamespaceEnum(saved_page_ptr->client_id.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::SUCCESS), 1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      kTestFileSize / 1024, 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      1);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.StorageInfo.InternalFreeSpaceMiB", 1);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.StorageInfo.ExternalFreeSpaceMiB", 1);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.StorageInfo.InternalUsagePercentage", 1);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.StorageInfo.ExternalUsagePercentage", 1);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.StorageInfo.InternalArchiveSizeMiB", 1);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.StorageInfo.ExternalArchiveSizeMiB", 1);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageSuccessfulWithSameOriginalUrl) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);

  // Pass the original URL same as the final URL.
  int64_t offline_id = SavePageWithExpectedResult(
      kTestUrl, kTestClientId1, kTestUrl, kEmptyRequestOrigin,
      std::move(archiver), SavePageResult::SUCCESS);

  EXPECT_EQ(1UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  auto saved_page_ptr = store_test_util()->GetPageByOfflineId(offline_id);
  ASSERT_TRUE(saved_page_ptr);

  EXPECT_EQ(kTestUrl, saved_page_ptr->url);
  // The original URL should be empty.
  EXPECT_TRUE(saved_page_ptr->original_url.is_empty());

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(
          model_utils::ToNamespaceEnum(saved_page_ptr->client_id.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::SUCCESS), 1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      kTestFileSize / 1024, 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      1);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageSuccessfulWithRequestOrigin) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t offline_id = SavePageWithExpectedResult(
      kTestUrl, kTestClientId1, kTestUrl2, kTestRequestOrigin,
      std::move(archiver), SavePageResult::SUCCESS);

  EXPECT_EQ(1UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  auto saved_page_ptr = store_test_util()->GetPageByOfflineId(offline_id);
  ASSERT_TRUE(saved_page_ptr);

  EXPECT_EQ(kTestUrl, saved_page_ptr->url);
  EXPECT_EQ(kTestClientId1.id, saved_page_ptr->client_id.id);
  EXPECT_EQ(kTestClientId1.name_space, saved_page_ptr->client_id.name_space);
  EXPECT_EQ(last_path_created_by_archiver(), saved_page_ptr->file_path);
  EXPECT_EQ(kTestFileSize, saved_page_ptr->file_size);
  EXPECT_EQ(0, saved_page_ptr->access_count);
  EXPECT_EQ(0, saved_page_ptr->flags);
  EXPECT_EQ(kTestTitle, saved_page_ptr->title);
  EXPECT_EQ(kTestUrl2, saved_page_ptr->original_url);
  EXPECT_EQ(kTestRequestOrigin, saved_page_ptr->request_origin);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(
          model_utils::ToNamespaceEnum(saved_page_ptr->client_id.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::SUCCESS), 1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      kTestFileSize / 1024, 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      1);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverCancelled) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::ERROR_CANCELED);
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::CANCELLED);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(kTestClientId1.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::CANCELLED), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      0);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      0);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverDeviceFull) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::ERROR_DEVICE_FULL);
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::DEVICE_FULL);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(kTestClientId1.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::DEVICE_FULL), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      0);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      0);
}

TEST_F(OfflinePageModelTaskifiedTest,
       SavePageOfflineArchiverContentUnavailable) {
  auto archiver =
      BuildArchiver(kTestUrl, ArchiverResult::ERROR_CONTENT_UNAVAILABLE);
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::CONTENT_UNAVAILABLE);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(kTestClientId1.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::CONTENT_UNAVAILABLE), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      0);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      0);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineCreationFailed) {
  auto archiver =
      BuildArchiver(kTestUrl, ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED);
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::ARCHIVE_CREATION_FAILED);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(kTestClientId1.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::ARCHIVE_CREATION_FAILED), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      0);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      0);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverReturnedWrongUrl) {
  auto archiver = BuildArchiver(GURL("http://other.random.url.com"),
                                ArchiverResult::SUCCESSFULLY_CREATED);
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::ARCHIVE_CREATION_FAILED);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(kTestClientId1.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::ARCHIVE_CREATION_FAILED), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      0);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      0);
}

// This test is disabled since it's lacking the ability of mocking store failure
// in store_test_utils. https://crbug.com/781023
// TODO(romax): reenable the test once the above issue is resolved.
TEST_F(OfflinePageModelTaskifiedTest,
       DISABLED_SavePageOfflineCreationStoreWriteFailure) {}

TEST_F(OfflinePageModelTaskifiedTest, SavePageLocalFileFailed) {
  SavePageWithExpectedResult(
      kFileUrl, kTestClientId1, kTestUrl2, kEmptyRequestOrigin,
      std::unique_ptr<OfflinePageTestArchiver>(), SavePageResult::SKIPPED);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(kTestClientId1.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::SKIPPED), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      0);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      0);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOfflineArchiverTwoPages) {
  // This test case is for the scenario that there are two save page requests
  // but the first one is slower during archive creation (set_delayed in the
  // test case) so the second request will finish first.
  // offline_id1 will be id of the first completed request.
  // offline_id2 will be id of the second completed request.
  int64_t offline_id1;
  int64_t offline_id2;

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()))
      .Times(2)
      .WillOnce(SaveArg<1>(&offline_id1))
      .WillOnce(SaveArg<1>(&offline_id2));

  // delayed_archiver_ptr will be valid until after first PumpLoop() call after
  // CompleteCreateArchive() is called. Keeping the raw pointer because the
  // ownership is transferring to the model.
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  OfflinePageTestArchiver* delayed_archiver_ptr = archiver.get();
  delayed_archiver_ptr->set_delayed(true);
  SavePageWithCallback(kTestUrl, kTestClientId1, GURL(), kEmptyRequestOrigin,
                       std::move(archiver), callback.Get());

  // Request to save another page, with request origin.
  archiver = BuildArchiver(kTestUrl2, ArchiverResult::SUCCESSFULLY_CREATED);
  SavePageWithCallback(kTestUrl2, kTestClientId2, GURL(), kTestRequestOrigin,
                       std::move(archiver), callback.Get());

  EXPECT_EQ(1UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  base::FilePath saved_file_path1 = last_path_created_by_archiver();

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(kTestClientId2.name_space)),
      1);

  ResetResults();

  delayed_archiver_ptr->CompleteCreateArchive();
  // Pump loop so the first request can finish saving.
  PumpLoop();

  // Check that offline_id1 refers to the second save page request.
  EXPECT_EQ(2UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(2LL, store_test_util()->GetPageCount());
  base::FilePath saved_file_path2 = last_path_created_by_archiver();

  auto saved_page_ptr1 = store_test_util()->GetPageByOfflineId(offline_id1);
  auto saved_page_ptr2 = store_test_util()->GetPageByOfflineId(offline_id2);
  ASSERT_TRUE(saved_page_ptr1);
  ASSERT_TRUE(saved_page_ptr2);

  EXPECT_EQ(kTestUrl2, saved_page_ptr1->url);
  EXPECT_EQ(kTestClientId2, saved_page_ptr1->client_id);
  EXPECT_EQ(saved_file_path1, saved_page_ptr1->file_path);
  EXPECT_EQ(kTestFileSize, saved_page_ptr1->file_size);
  EXPECT_EQ(kTestRequestOrigin, saved_page_ptr1->request_origin);

  EXPECT_EQ(kTestUrl, saved_page_ptr2->url);
  EXPECT_EQ(kTestClientId1, saved_page_ptr2->client_id);
  EXPECT_EQ(saved_file_path2, saved_page_ptr2->file_path);
  EXPECT_EQ(kTestFileSize, saved_page_ptr2->file_size);

  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.SavePageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(kTestClientId1.name_space)),
      2);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::SUCCESS), 2);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageSize"),
      kTestFileSize / 1024, 2);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageTime"),
      2);
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageOnBackground) {
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  OfflinePageTestArchiver* archiver_ptr = archiver.get();

  OfflinePageModel::SavePageParams save_page_params;
  save_page_params.url = kTestUrl;
  save_page_params.client_id = kTestClientId1;
  save_page_params.original_url = kTestUrl2;
  save_page_params.is_background = true;
  save_page_params.use_page_problem_detectors = false;

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::SUCCESS), A<int64_t>()));
  model()->SavePage(save_page_params, std::move(archiver), nullptr,
                    callback.Get());
  EXPECT_TRUE(archiver_ptr->create_archive_called());
  // |remove_popup_overlay| should be turned on on background mode.
  EXPECT_TRUE(archiver_ptr->create_archive_params().remove_popup_overlay);

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, SavePageWithNullArchiver) {
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, GURL(),
                             kEmptyRequestOrigin, nullptr,
                             SavePageResult::CONTENT_UNAVAILABLE);
  histogram_tester()->ExpectUniqueSample(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.SavePageResult"),
      static_cast<int>(SavePageResult::CONTENT_UNAVAILABLE), 1);
}

TEST_F(OfflinePageModelTaskifiedTest, AddPage) {
  // Creates a fresh page.
  page_generator()->SetArchiveDirectory(temporary_dir_path());
  OfflinePageItem page = page_generator()->CreateItemWithTempFile();

  base::MockCallback<AddPageCallback> callback;
  EXPECT_CALL(callback, Run(An<AddPageResult>(), Eq(page.offline_id)));

  model()->AddPage(page, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
  EXPECT_TRUE(observer_add_page_called());
  EXPECT_EQ(last_added_page(), page);
}

TEST_F(OfflinePageModelTaskifiedTest, MarkPageAccessed) {
  OfflinePageItem page = page_generator()->CreateItem();
  InsertPageIntoStore(page);

  model()->MarkPageAccessed(page.offline_id);
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();

  auto accessed_page_ptr =
      store_test_util()->GetPageByOfflineId(page.offline_id);
  ASSERT_TRUE(accessed_page_ptr);
  EXPECT_EQ(1LL, accessed_page_ptr->access_count);
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.AccessPageCount",
      static_cast<int>(model_utils::ToNamespaceEnum(page.client_id.name_space)),
      1);
}

TEST_F(OfflinePageModelTaskifiedTest, GetAllPagesWhenStoreEmpty) {
  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(IsEmpty()));

  model()->GetAllPages(callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();

  histogram_tester()->ExpectUniqueSample("OfflinePages.SavedPageCountUponQuery",
                                         0, 1);
}

// TODO(romax): remove these 'indicators for newly added tests' when migration
// is done.
// This test case is covered by DeletePageTaskTest::DeletePagesBy*.
TEST_F(OfflinePageModelTaskifiedTest, DISABLED_DeletePageSuccessful) {}

// This test case is covered by DeletePageTaskTest::DeletePagesByUrlPredicate.
TEST_F(OfflinePageModelTaskifiedTest,
       DISABLED_DeleteCachedPageByPredicateUserRequested) {}

// This test case is renamed to DeletePagesByUrlPredicate.
TEST_F(OfflinePageModelTaskifiedTest, DISABLED_DeleteCachedPageByPredicate) {}

// This test case is covered by DeletePageTaskTest::DeletePagesBy*NotFound.
TEST_F(OfflinePageModelTaskifiedTest, DISABLED_DeletePageNotFound) {}

// This test case is covered by
// DeletePageTaskTest::DeletePagesStoreFailureOnRemove.
TEST_F(OfflinePageModelTaskifiedTest, DISABLED_DeletePageStoreFailureOnRemove) {
}

// This test case is covered by DeletePageTaskTest::DeletePagesBy*.
TEST_F(OfflinePageModelTaskifiedTest, DISABLED_DeleteMultiplePages) {}

// These newly added tests are testing the API instead of results, which
// should be covered in DeletePagesTaskTest.

TEST_F(OfflinePageModelTaskifiedTest, DeletePagesByOfflineId) {
  page_generator()->SetArchiveDirectory(temporary_dir_path());
  page_generator()->SetNamespace(kDefaultNamespace);
  OfflinePageItem page1 = page_generator()->CreateItemWithTempFile();
  OfflinePageItem page2 = page_generator()->CreateItemWithTempFile();
  page1.system_download_id = kDownloadId;
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);
  EXPECT_EQ(2UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(2LL, store_test_util()->GetPageCount());

  base::MockCallback<DeletePageCallback> callback;
  EXPECT_CALL(callback, Run(A<DeletePageResult>()));
  CheckTaskQueueIdle();

  model()->DeletePagesByOfflineId({page1.offline_id}, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
  EXPECT_TRUE(observer_delete_page_called());
  EXPECT_EQ(last_deleted_page_info().offline_id, page1.offline_id);
  EXPECT_EQ(1UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  EXPECT_EQ(page1.system_download_id,
            download_manager_stub()->last_removed_id());
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.DeletePageCount",
      static_cast<int>(
          model_utils::ToNamespaceEnum(page1.client_id.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.DeletePageResult",
      static_cast<int>(DeletePageResult::SUCCESS), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(page1.client_id.name_space,
                                      "OfflinePages.PageLifetime"),
      1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(page1.client_id.name_space,
                                      "OfflinePages.AccessCount"),
      1);
}

TEST_F(OfflinePageModelTaskifiedTest, DeletePagesByUrlPredicate) {
  page_generator()->SetArchiveDirectory(temporary_dir_path());
  page_generator()->SetNamespace(kDefaultNamespace);
  page_generator()->SetUrl(kTestUrl);
  OfflinePageItem page1 = page_generator()->CreateItemWithTempFile();
  page_generator()->SetUrl(kTestUrl2);
  OfflinePageItem page2 = page_generator()->CreateItemWithTempFile();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);
  EXPECT_EQ(2UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(2LL, store_test_util()->GetPageCount());

  base::MockCallback<DeletePageCallback> callback;
  EXPECT_CALL(callback, Run(testing::A<DeletePageResult>()));
  CheckTaskQueueIdle();

  UrlPredicate predicate =
      base::Bind([](const GURL& expected_url,
                    const GURL& url) -> bool { return url == expected_url; },
                 kTestUrl);

  model()->DeleteCachedPagesByURLPredicate(predicate, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
  EXPECT_TRUE(observer_delete_page_called());
  EXPECT_EQ(last_deleted_page_info().offline_id, page1.offline_id);
  EXPECT_EQ(1UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.DeletePageCount",
      static_cast<int>(
          model_utils::ToNamespaceEnum(page1.client_id.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.DeletePageResult",
      static_cast<int>(DeletePageResult::SUCCESS), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(page1.client_id.name_space,
                                      "OfflinePages.PageLifetime"),
      1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(page1.client_id.name_space,
                                      "OfflinePages.AccessCount"),
      1);
}

TEST_F(OfflinePageModelTaskifiedTest, GetPageByOfflineId) {
  page_generator()->SetNamespace(kDefaultNamespace);
  page_generator()->SetUrl(kTestUrl);
  OfflinePageItem page = page_generator()->CreateItem();
  InsertPageIntoStore(page);

  base::MockCallback<SingleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(Pointee(Eq(page))));

  model()->GetPageByOfflineId(page.offline_id, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, GetPagesByUrl_FinalUrl) {
  page_generator()->SetUrl(kTestUrl);
  OfflinePageItem page1 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  page_generator()->SetUrl(kTestUrl2);
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page2);

  // Search by kTestUrl.
  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(ElementsAre(page1)));
  model()->GetPagesByURL(kTestUrl, URLSearchMode::SEARCH_BY_FINAL_URL_ONLY,
                         callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());
  PumpLoop();

  // Search by kTestUrl2.
  EXPECT_CALL(callback, Run(ElementsAre(page2)));
  model()->GetPagesByURL(kTestUrl2, URLSearchMode::SEARCH_BY_FINAL_URL_ONLY,
                         callback.Get());
  PumpLoop();

  // Search by random url, which should return no pages.
  EXPECT_CALL(callback, Run(IsEmpty()));
  model()->GetPagesByURL(kOtherUrl, URLSearchMode::SEARCH_BY_FINAL_URL_ONLY,
                         callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());
  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest,
       GetPagesByUrl_FinalUrlWithFragmentStripped) {
  page_generator()->SetUrl(kTestUrl);
  OfflinePageItem page1 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  page_generator()->SetUrl(kTestUrl2WithFragment);
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page2);

  // Search by kTestUrlWithFragment.
  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(ElementsAre(page1)));
  model()->GetPagesByURL(kTestUrlWithFragment,
                         URLSearchMode::SEARCH_BY_FINAL_URL_ONLY,
                         callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());
  PumpLoop();

  // Search by kTestUrl2.
  EXPECT_CALL(callback, Run(ElementsAre(page2)));
  model()->GetPagesByURL(kTestUrl2, URLSearchMode::SEARCH_BY_FINAL_URL_ONLY,
                         callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());
  PumpLoop();

  // Search by kTestUrl2WithFragment.
  EXPECT_CALL(callback, Run(ElementsAre(page2)));
  model()->GetPagesByURL(kTestUrl2WithFragment,
                         URLSearchMode::SEARCH_BY_FINAL_URL_ONLY,
                         callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());
  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, GetPagesByUrl_AllUrls) {
  page_generator()->SetUrl(kTestUrl);
  page_generator()->SetOriginalUrl(kTestUrl2);
  OfflinePageItem page1 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  page_generator()->SetUrl(kTestUrl2);
  page_generator()->SetOriginalUrl(GURL());
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page2);

  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(UnorderedElementsAre(page1, page2)));
  model()->GetPagesByURL(kTestUrl2, URLSearchMode::SEARCH_BY_ALL_URLS,
                         callback.Get());
  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, CanSaveURL) {
  EXPECT_TRUE(OfflinePageModel::CanSaveURL(GURL("http://foo")));
  EXPECT_TRUE(OfflinePageModel::CanSaveURL(GURL("https://foo")));
  EXPECT_FALSE(OfflinePageModel::CanSaveURL(GURL("file:///foo")));
  EXPECT_FALSE(OfflinePageModel::CanSaveURL(GURL("data:image/png;base64,ab")));
  EXPECT_FALSE(OfflinePageModel::CanSaveURL(GURL("chrome://version")));
  EXPECT_FALSE(OfflinePageModel::CanSaveURL(GURL("chrome-native://newtab/")));
  EXPECT_FALSE(OfflinePageModel::CanSaveURL(GURL("/invalid/url.mhtml")));
}

TEST_F(OfflinePageModelTaskifiedTest, GetOfflineIdsForClientId) {
  page_generator()->SetNamespace(kTestClientId1.name_space);
  page_generator()->SetId(kTestClientId1.id);
  OfflinePageItem page1 = page_generator()->CreateItem();
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);

  base::MockCallback<MultipleOfflineIdCallback> callback;
  EXPECT_CALL(callback,
              Run(UnorderedElementsAre(page1.offline_id, page2.offline_id)));

  model()->GetOfflineIdsForClientId(kTestClientId1, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, GetPagesByClientIds) {
  page_generator()->SetNamespace(kTestClientId1.name_space);
  page_generator()->SetId(kTestClientId1.id);
  OfflinePageItem page1 = page_generator()->CreateItem();
  OfflinePageItem page2 = page_generator()->CreateItem();
  page_generator()->SetNamespace(kTestUserRequestedClientId.name_space);
  OfflinePageItem page3 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);
  InsertPageIntoStore(page3);

  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(UnorderedElementsAre(page1, page2)));

  model()->GetPagesByClientIds({kTestClientId1}, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, GetPagesByRequestOrigin) {
  page_generator()->SetRequestOrigin(kTestRequestOrigin);
  OfflinePageItem page1 = page_generator()->CreateItem();
  page_generator()->SetRequestOrigin(kEmptyRequestOrigin);
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);

  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(ElementsAre(page1)));

  model()->GetPagesByRequestOrigin(kTestRequestOrigin, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, GetPageBySizeAndDigest) {
  static const int64_t kFileSize1 = 123LL;
  static const int64_t kFileSize2 = 999999LL;
  static const char kDigest1[] = "digest 1";
  page_generator()->SetFileSize(kFileSize1);
  page_generator()->SetDigest(kDigest1);
  OfflinePageItem page1 = page_generator()->CreateItem();
  page_generator()->SetFileSize(kFileSize2);
  page_generator()->SetDigest(kDigest1);
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);

  base::MockCallback<SingleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(Pointee(Eq(page2))));

  model()->GetPageBySizeAndDigest(kFileSize2, kDigest1, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, DeletePagesByClientIds) {
  page_generator()->SetArchiveDirectory(temporary_dir_path());
  page_generator()->SetNamespace(kTestClientId1.name_space);
  page_generator()->SetId(kTestClientId1.id);
  OfflinePageItem page1 = page_generator()->CreateItemWithTempFile();
  page_generator()->SetId(kTestClientId2.id);
  OfflinePageItem page2 = page_generator()->CreateItemWithTempFile();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);
  EXPECT_EQ(2UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(2LL, store_test_util()->GetPageCount());

  base::MockCallback<DeletePageCallback> callback;
  EXPECT_CALL(callback, Run(testing::A<DeletePageResult>()));
  CheckTaskQueueIdle();

  model()->DeletePagesByClientIds({page1.client_id}, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
  EXPECT_TRUE(observer_delete_page_called());
  EXPECT_EQ(last_deleted_page_info().client_id, page1.client_id);
  EXPECT_EQ(1UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.DeletePageCount",
      static_cast<int>(
          model_utils::ToNamespaceEnum(page1.client_id.name_space)),
      1);
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.DeletePageResult",
      static_cast<int>(DeletePageResult::SUCCESS), 1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.PageLifetime"),
      1);
  histogram_tester()->ExpectTotalCount(
      model_utils::AddHistogramSuffix(kTestClientId1.name_space,
                                      "OfflinePages.AccessCount"),
      1);
}

TEST_F(OfflinePageModelTaskifiedTest, GetPagesByNamespace) {
  page_generator()->SetNamespace(kDefaultNamespace);
  OfflinePageItem page1 = page_generator()->CreateItem();
  OfflinePageItem page2 = page_generator()->CreateItem();
  page_generator()->SetNamespace(kDownloadNamespace);
  OfflinePageItem page3 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);
  InsertPageIntoStore(page3);

  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(UnorderedElementsAre(page1, page2)));

  model()->GetPagesByNamespace(kDefaultNamespace, callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, GetPagesRemovedOnCacheReset) {
  page_generator()->SetNamespace(kDefaultNamespace);
  OfflinePageItem page1 = page_generator()->CreateItem();
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);
  page_generator()->SetNamespace(kDownloadNamespace);
  OfflinePageItem page3 = page_generator()->CreateItem();
  InsertPageIntoStore(page3);

  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(UnorderedElementsAre(page1, page2)));

  model()->GetPagesRemovedOnCacheReset(callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
}

TEST_F(OfflinePageModelTaskifiedTest, GetPagesSupportedByDownloads) {
  page_generator()->SetNamespace(kDownloadNamespace);
  OfflinePageItem page1 = page_generator()->CreateItem();
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);
  page_generator()->SetNamespace(kDefaultNamespace);
  OfflinePageItem page3 = page_generator()->CreateItem();
  InsertPageIntoStore(page3);

  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(UnorderedElementsAre(page1, page2)));

  model()->GetPagesSupportedByDownloads(callback.Get());
  EXPECT_TRUE(task_queue()->HasRunningTask());

  PumpLoop();
}

// This test is affected by https://crbug.com/725685, which only affects windows
// platform.
#if defined(OS_WIN)
#define MAYBE_CheckPagesSavedInSeparateDirsPrivate \
  DISABLED_CheckPagesSavedInSeparateDirsPrivate
#else
#define MAYBE_CheckPagesSavedInSeparateDirsPrivate \
  CheckPagesSavedInSeparateDirsPrivate
#endif
TEST_F(OfflinePageModelTaskifiedTest,
       MAYBE_CheckPagesSavedInSeparateDirsPrivate) {
  // Save a temporary page.
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t temporary_id = SavePageWithExpectedResult(
      kTestUrl, kTestClientId1, GURL(), kEmptyRequestOrigin,
      std::move(archiver), SavePageResult::SUCCESS);

  // Save a persistent page that will not be published
  archiver = BuildArchiver(kTestUrl2, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t persistent_id = SavePageWithExpectedResult(
      kTestUrl2, kTestBrowserActionsClientId, GURL(), kEmptyRequestOrigin,
      std::move(archiver), SavePageResult::SUCCESS);

  std::unique_ptr<OfflinePageItem> temporary_page =
      store_test_util()->GetPageByOfflineId(temporary_id);
  std::unique_ptr<OfflinePageItem> persistent_page =
      store_test_util()->GetPageByOfflineId(persistent_id);

  ASSERT_TRUE(temporary_page);
  ASSERT_TRUE(persistent_page);

  base::FilePath temporary_page_path = temporary_page->file_path;
  base::FilePath persistent_page_path = persistent_page->file_path;

  EXPECT_TRUE(temporary_dir_path().IsParent(temporary_page_path));
  // For a page in the prefetch namespace, it gets moved to the
  // a private internal directory inside chromium.
  EXPECT_TRUE(private_archive_dir_path().IsParent(persistent_page_path));
  EXPECT_NE(temporary_page_path.DirName(), persistent_page_path.DirName());
}

// This test is affected by https://crbug.com/725685, which only affects windows
// platform.
#if defined(OS_WIN)
#define MAYBE_CheckPagesSavedInSeparateDirsPublic \
  DISABLED_CheckPagesSavedInSeparateDirsPublic
#else
#define MAYBE_CheckPagesSavedInSeparateDirsPublic \
  CheckPagesSavedInSeparateDirsPublic
#endif
TEST_F(OfflinePageModelTaskifiedTest,
       MAYBE_CheckPagesSavedInSeparateDirsPublic) {
  // Save a temporary page.
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t temporary_id = SavePageWithExpectedResult(
      kTestUrl, kTestClientId1, GURL(), kEmptyRequestOrigin,
      std::move(archiver), SavePageResult::SUCCESS);

  // Save a persistent page that will be published.
  archiver = BuildArchiver(kTestUrl2, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t persistent_id = SavePageWithExpectedResult(
      kTestUrl2, kTestUserRequestedClientId, GURL(), kEmptyRequestOrigin,
      std::move(archiver), SavePageResult::SUCCESS);

  std::unique_ptr<OfflinePageItem> temporary_page =
      store_test_util()->GetPageByOfflineId(temporary_id);
  std::unique_ptr<OfflinePageItem> persistent_page =
      store_test_util()->GetPageByOfflineId(persistent_id);

  ASSERT_TRUE(temporary_page);
  ASSERT_TRUE(persistent_page);

  base::FilePath temporary_page_path = temporary_page->file_path;
  base::FilePath persistent_page_path = persistent_page->file_path;

  EXPECT_TRUE(temporary_dir_path().IsParent(temporary_page_path));
  // TODO(petewil): It might be better to replace the check below with a check
  // that the file ended up in the correct place instead of just not the wrong
  // place.
  EXPECT_NE(temporary_page_path.DirName(), persistent_page_path.DirName());
}

// This test is affected by https://crbug.com/725685, which only affects windows
// platform.
#if defined(OS_WIN)
#define MAYBE_CheckPublishInternalArchive DISABLED_CheckPublishInternalArchive
#else
#define MAYBE_CheckPublishInternalArchive CheckPublishInternalArchive
#endif
TEST_F(OfflinePageModelTaskifiedTest, MAYBE_CheckPublishInternalArchive) {
  // Save a persistent page into our internal directory that will not be
  // published. We use a "browser actions" page for this purpose.
  std::unique_ptr<OfflinePageTestArchiver> test_archiver =
      BuildArchiver(kTestUrl2, ArchiverResult::SUCCESSFULLY_CREATED);
  int64_t persistent_id = SavePageWithExpectedResult(
      kTestUrl2, kTestBrowserActionsClientId, GURL(), kEmptyRequestOrigin,
      std::move(test_archiver), SavePageResult::SUCCESS);

  std::unique_ptr<OfflinePageItem> persistent_page =
      store_test_util()->GetPageByOfflineId(persistent_id);

  ASSERT_TRUE(persistent_page);

  base::FilePath persistent_page_path = persistent_page->file_path;

  // For a page in the browser actions namespace, it gets moved to the
  // a private internal directory inside chromium.
  EXPECT_TRUE(private_archive_dir_path().IsParent(persistent_page_path));

  // Make another archiver, since SavePageWithExpectedResult deleted the first
  // one.
  test_archiver =
      BuildArchiver(kTestUrl2, ArchiverResult::SUCCESSFULLY_CREATED);

  // Publish the page from our internal store.
  base::MockCallback<PublishPageCallback> callback;
  EXPECT_CALL(callback, Run(A<const base::FilePath&>(), A<SavePageResult>()));

  model()->PublishInternalArchive(*persistent_page, std::move(test_archiver),
                                  callback.Get());
  PumpLoop();
}

// This test is disabled since it's lacking the ability of mocking store failure
// in store_test_utils. https://crbug.com/781023
// TODO(romax): reenable the test once the above issue is resolved.
TEST_F(OfflinePageModelTaskifiedTest,
       DISABLED_ClearCachedPagesTriggeredWhenSaveFailed) {
  // After a save failed, only PostClearCachedPagesTask will be triggered.
  page_generator()->SetArchiveDirectory(temporary_dir_path());
  page_generator()->SetNamespace(kDefaultNamespace);
  page_generator()->SetUrl(kTestUrl);
  OfflinePageItem page1 = page_generator()->CreateItemWithTempFile();
  OfflinePageItem page2 = page_generator()->CreateItemWithTempFile();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);

  ResetResults();

  base::MockCallback<SavePageCallback> callback;
  EXPECT_CALL(callback, Run(Eq(SavePageResult::ERROR_PAGE), A<int64_t>()));

  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  OfflinePageTestArchiver* archiver_ptr = archiver.get();

  SavePageWithCallback(kTestUrl, kTestClientId1, kTestUrl2, kEmptyRequestOrigin,
                       std::move(archiver), callback.Get());
  // The archiver will not be erased before PumpLoop().
  ASSERT_TRUE(archiver_ptr);
  EXPECT_TRUE(archiver_ptr->create_archive_called());

  PumpLoop();
  EXPECT_FALSE(observer_add_page_called());
  EXPECT_FALSE(observer_delete_page_called());
}

TEST_F(OfflinePageModelTaskifiedTest, ExtraActionTriggeredWhenSaveSuccess) {
  // After a save successfully saved, both RemovePagesWithSameUrlInSameNamespace
  // and PostClearCachedPagesTask will be triggered.
  // Add pages that have the same namespace and url directly into store, in
  // order to avoid triggering the removal.
  // The 'default' namespace has a limit of 1 per url.
  page_generator()->SetArchiveDirectory(temporary_dir_path());
  page_generator()->SetNamespace(kDefaultNamespace);
  page_generator()->SetUrl(kTestUrl);
  OfflinePageItem page1 = page_generator()->CreateItemWithTempFile();
  OfflinePageItem page2 = page_generator()->CreateItemWithTempFile();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);

  ResetResults();

  std::unique_ptr<OfflinePageTestArchiver> archiver(
      BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED));
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::SUCCESS);

  EXPECT_TRUE(observer_add_page_called());
  EXPECT_TRUE(observer_delete_page_called());
}

TEST_F(OfflinePageModelTaskifiedTest, GetArchiveDirectory) {
  base::FilePath temporary_dir =
      model()->GetInternalArchiveDirectory(kDefaultNamespace);
  EXPECT_EQ(temporary_dir_path(), temporary_dir);
  base::FilePath persistent_dir =
      model()->GetInternalArchiveDirectory(kDownloadNamespace);
  EXPECT_EQ(private_archive_dir_path(), persistent_dir);
}

TEST_F(OfflinePageModelTaskifiedTest, GetAllPages) {
  OfflinePageItem page1 = page_generator()->CreateItem();
  OfflinePageItem page2 = page_generator()->CreateItem();
  InsertPageIntoStore(page1);
  InsertPageIntoStore(page2);

  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  EXPECT_CALL(callback, Run(UnorderedElementsAre(page1, page2)));
  model()->GetAllPages(callback.Get());
  PumpLoop();

  histogram_tester()->ExpectUniqueSample("OfflinePages.SavedPageCountUponQuery",
                                         2, 1);
}

// This test is affected by https://crbug.com/725685, which only affects windows
// platform.
#if defined(OS_WIN)
#define MAYBE_StartupMaintenanceTaskExecuted \
  DISABLED_StartupMaintenanceTaskExecuted
#else
#define MAYBE_StartupMaintenanceTaskExecuted StartupMaintenanceTaskExecuted
#endif
TEST_F(OfflinePageModelTaskifiedTest, MAYBE_StartupMaintenanceTaskExecuted) {
  // Insert temporary pages
  page_generator()->SetArchiveDirectory(temporary_dir_path());
  page_generator()->SetNamespace(kDefaultNamespace);
  // Page missing archive file in temporary directory.
  OfflinePageItem temp_page1 = page_generator()->CreateItem();
  // Page missing metadata entry in database since it's not inserted into store.
  OfflinePageItem temp_page2 = page_generator()->CreateItemWithTempFile();
  // Page in temporary namespace saved in persistent directory to simulate pages
  // saved in legacy directory.
  page_generator()->SetArchiveDirectory(private_archive_dir_path());
  OfflinePageItem temp_page3 = page_generator()->CreateItemWithTempFile();
  InsertPageIntoStore(temp_page1);
  InsertPageIntoStore(temp_page3);

  // Insert persistent pages.
  page_generator()->SetNamespace(kDownloadNamespace);
  // Page missing archive file in private directory.
  OfflinePageItem persistent_page1 = page_generator()->CreateItem();
  // Page missing metadata entry in database since it's not inserted into store.
  OfflinePageItem persistent_page2 = page_generator()->CreateItemWithTempFile();
  // Page in persistent namespace saved in private directory.
  OfflinePageItem persistent_page3 = page_generator()->CreateItemWithTempFile();
  InsertPageIntoStore(persistent_page1);
  InsertPageIntoStore(persistent_page3);

  PumpLoop();

  EXPECT_EQ(4LL, store_test_util()->GetPageCount());
  EXPECT_EQ(1UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(3UL,
            test_utils::GetFileCountInDirectory(private_archive_dir_path()));

  // Execute GetAllPages and move the clock forward to cover the delay, in order
  // to trigger StartupMaintenanceTask execution.
  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  model()->GetAllPages(callback.Get());
  task_runner()->FastForwardBy(
      OfflinePageModelTaskified::kMaintenanceTasksDelay +
      base::TimeDelta::FromMilliseconds(1));
  PumpLoop();

  EXPECT_EQ(2LL, store_test_util()->GetPageCount());
  EXPECT_EQ(0UL, test_utils::GetFileCountInDirectory(temporary_dir_path()));
  EXPECT_EQ(1UL,
            test_utils::GetFileCountInDirectory(private_archive_dir_path()));
}

TEST_F(OfflinePageModelTaskifiedTest, ClearStorage) {
  // The ClearStorage task should not be executed based on time delays after
  // launch (aka the model being built).
  task_runner()->FastForwardBy(base::TimeDelta::FromDays(1));
  PumpLoop();
  EXPECT_EQ(base::Time(), last_maintenance_tasks_schedule_time());

  // GetAllPages should schedule a delayed task that will eventually run
  // ClearStorage.
  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  model()->GetAllPages(callback.Get());
  PumpLoop();
  EXPECT_EQ(task_runner()->Now(), last_maintenance_tasks_schedule_time());
  base::Time last_scheduling_time = task_runner()->Now();
  // Confirm no runs so far.
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ClearTemporaryPages.Result", 0);

  // After the delay (plus 1 millisecond just in case) ClearStorage should be
  // enqueued and executed.
  const base::TimeDelta run_delay =
      OfflinePageModelTaskified::kMaintenanceTasksDelay +
      base::TimeDelta::FromMilliseconds(1);
  task_runner()->FastForwardBy(run_delay);
  PumpLoop();
  EXPECT_EQ(last_scheduling_time, last_maintenance_tasks_schedule_time());
  // Check that CleanupThumbnailsTask ran.
  histogram_tester()->ExpectUniqueSample("OfflinePages.CleanupThumbnails.Count",
                                         0, 1);

  // Calling GetAllPages after only half of the enforced interval between
  // ClearStorage runs should not schedule ClearStorage.
  // Note: The previous elapsed delay is discounted from the clock advance here.
  task_runner()->FastForwardBy(
      OfflinePageModelTaskified::kClearStorageInterval / 2 - run_delay);
  ASSERT_GT(task_runner()->Now(), last_scheduling_time);
  model()->GetAllPages(callback.Get());
  // And advance the delay too.
  task_runner()->FastForwardBy(run_delay);
  PumpLoop();
  EXPECT_EQ(last_scheduling_time, last_maintenance_tasks_schedule_time());
  // Confirm a single run happened so far.
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.ClearTemporaryPages.Result",
      static_cast<int>(ClearStorageResult::UNNECESSARY), 1);

  // Forwarding by the full interval (plus 1 second just in case) should allow
  // the task to be enqueued again.
  task_runner()->FastForwardBy(
      OfflinePageModelTaskified::kClearStorageInterval / 2 +
      base::TimeDelta::FromSeconds(1));
  // Saving a page should also immediately enqueue the ClearStorage task.
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::SUCCESS);
  last_scheduling_time = task_runner()->Now();
  // Advance the delay again.
  task_runner()->FastForwardBy(run_delay);
  PumpLoop();
  EXPECT_EQ(last_scheduling_time, last_maintenance_tasks_schedule_time());

  // Confirm that two runs happened.
  histogram_tester()->ExpectUniqueSample(
      "OfflinePages.ClearTemporaryPages.Result",
      static_cast<int>(ClearStorageResult::UNNECESSARY), 2);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ClearTemporaryPages.BatchSize", 0);
  // Check that CleanupThumbnailsTask ran only once.
  histogram_tester()->ExpectTotalCount("OfflinePages.CleanupThumbnails.Count",
                                       1);
}

// This test is affected by https://crbug.com/725685, which only affects windows
// platform.
#if defined(OS_WIN)
#define MAYBE_PersistentPageConsistencyCheckExecuted \
  DISABLED_PersistentPageConsistencyCheckExecuted
#else
#define MAYBE_PersistentPageConsistencyCheckExecuted \
  PersistentPageConsistencyCheckExecuted
#endif
TEST_F(OfflinePageModelTaskifiedTest, PersistentPageConsistencyCheckExecuted) {
  // The PersistentPageConsistencyCheckTask should not be executed based on time
  // delays after launch (aka the model being built).
  task_runner()->FastForwardBy(base::TimeDelta::FromDays(1));
  PumpLoop();
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ConsistencyCheck.Persistent.Result", 0);

  // GetAllPages should schedule a delayed task that will eventually run
  // PersistentPageConsistencyCheck.
  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  model()->GetAllPages(callback.Get());
  PumpLoop();
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ConsistencyCheck.Persistent.Result", 0);

  // Add a persistent page with file.
  page_generator()->SetNamespace(kDownloadNamespace);
  page_generator()->SetArchiveDirectory(public_archive_dir_path());
  OfflinePageItem page = page_generator()->CreateItemWithTempFile();
  page.system_download_id = kDownloadId;
  InsertPageIntoStore(page);
  EXPECT_EQ(1UL,
            test_utils::GetFileCountInDirectory(public_archive_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());

  // After the delay (plus 1 millisecond just in case), the consistency check
  // should be enqueued and executed.
  const base::TimeDelta run_delay =
      OfflinePageModelTaskified::kMaintenanceTasksDelay +
      base::TimeDelta::FromMilliseconds(1);
  task_runner()->FastForwardBy(run_delay);
  PumpLoop();
  // But nothing should change.
  EXPECT_EQ(1UL,
            test_utils::GetFileCountInDirectory(public_archive_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ConsistencyCheck.Persistent.Result", 1);

  // Delete the file associated with |page|, so the next time when the
  // consistency check is executed, the page will be marked as hidden.
  base::DeleteFile(page.file_path, false);

  // Calling GetAllPages after only half of the enforced interval between
  // consistency check runs should not schedule the task.
  // Note: The previous elapsed delay is discounted from the clock advance here.
  task_runner()->FastForwardBy(
      OfflinePageModelTaskified::kClearStorageInterval / 2 - run_delay);
  model()->GetAllPages(callback.Get());
  // And advance the delay too.
  task_runner()->FastForwardBy(run_delay);
  PumpLoop();
  // Confirm no persistent page consistency check is executed.
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ConsistencyCheck.Persistent.Result", 1);

  // Forwarding by the full interval (plus 1 second just in case) should allow
  // the task to be enqueued again and call GetAllPages again to enqueue the
  // task.
  task_runner()->FastForwardBy(
      OfflinePageModelTaskified::kClearStorageInterval / 2 +
      base::TimeDelta::FromSeconds(1));
  model()->GetAllPages(callback.Get());
  // And advance the delay too.
  task_runner()->FastForwardBy(run_delay);
  PumpLoop();
  // Confirm persistent page consistency check is executed, and the page is
  // marked as missing file.
  EXPECT_EQ(0UL,
            test_utils::GetFileCountInDirectory(public_archive_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  auto actual_page = store_test_util()->GetPageByOfflineId(page.offline_id);
  ASSERT_TRUE(actual_page);
  EXPECT_NE(base::Time(), actual_page->file_missing_time);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ConsistencyCheck.Persistent.Result", 2);

  // Forwarding by a long time that is enough for the page with missing file to
  // get expired.
  task_runner()->FastForwardBy(base::TimeDelta::FromDays(400));
  // Saving a page should also immediately enqueue the consistency check task.
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::SUCCESS);
  // Advance the delay to activate task execution.
  task_runner()->FastForwardBy(run_delay);
  PumpLoop();
  // Confirm persistent page consistency check is executed, and the page is
  // deleted from database, also notified system download manager.
  EXPECT_EQ(0UL,
            test_utils::GetFileCountInDirectory(public_archive_dir_path()));
  EXPECT_EQ(1LL, store_test_util()->GetPageCount());
  EXPECT_EQ(page.system_download_id,
            download_manager_stub()->last_removed_id());
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ConsistencyCheck.Persistent.Result", 3);
}

TEST_F(OfflinePageModelTaskifiedTest, MaintenanceTasksAreDisabled) {
  // The maintenance tasks should not be executed when disabled by tests.
  model()->DoNotRunMaintenanceTasksForTesting();

  // With that setting GetAllPages and saving a page should not schedule
  // maintenance tasks.
  base::MockCallback<MultipleOfflinePageItemCallback> callback;
  model()->GetAllPages(callback.Get());
  auto archiver = BuildArchiver(kTestUrl, ArchiverResult::SUCCESSFULLY_CREATED);
  SavePageWithExpectedResult(kTestUrl, kTestClientId1, kTestUrl2,
                             kEmptyRequestOrigin, std::move(archiver),
                             SavePageResult::SUCCESS);
  PumpLoop();
  EXPECT_EQ(base::Time(), last_maintenance_tasks_schedule_time());

  // Advance the clock considerably and confirm no runs happened.
  task_runner()->FastForwardBy(base::TimeDelta::FromDays(1));
  PumpLoop();
  EXPECT_EQ(base::Time(), last_maintenance_tasks_schedule_time());
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ClearTemporaryPages.Result", 0);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ClearTemporaryPages.BatchSize", 0);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ConsistencyCheck.Temporary.Result", 0);
  histogram_tester()->ExpectTotalCount(
      "OfflinePages.ConsistencyCheck.Persistent.Result", 0);
  histogram_tester()->ExpectTotalCount("OfflinePages.CleanupThumbnails.Count",
                                       0);
}

TEST_F(OfflinePageModelTaskifiedTest, StoreAndCheckThumbnail) {
  // Store a thumbnail.
  OfflinePageThumbnail thumb;
  thumb.offline_id = 1;
  thumb.expiration = base::Time::Now();
  thumb.thumbnail = "abc123";
  model()->StoreThumbnail(thumb);
  EXPECT_CALL(*this, ThumbnailAdded(_, thumb));
  PumpLoop();

  // Check it exists
  bool thumbnail_exists = false;
  auto exists_callback = base::BindLambdaForTesting(
      [&](bool exists) { thumbnail_exists = exists; });
  model()->HasThumbnailForOfflineId(thumb.offline_id, exists_callback);
  PumpLoop();
  EXPECT_TRUE(thumbnail_exists);

  // Obtain its data.
  std::unique_ptr<OfflinePageThumbnail> result_thumbnail;
  auto data_callback = base::BindLambdaForTesting(
      [&](std::unique_ptr<OfflinePageThumbnail> result) {
        result_thumbnail = std::move(result);
      });
  model()->GetThumbnailByOfflineId(thumb.offline_id, data_callback);
  PumpLoop();
  EXPECT_EQ(thumb, *result_thumbnail);
}

}  // namespace offline_pages
