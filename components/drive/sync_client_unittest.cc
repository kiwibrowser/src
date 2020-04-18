// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/sync_client.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/chromeos/about_resource_loader.h"
#include "components/drive/chromeos/about_resource_root_folder_id_loader.h"
#include "components/drive/chromeos/change_list_loader.h"
#include "components/drive/chromeos/drive_test_util.h"
#include "components/drive/chromeos/fake_free_disk_space_getter.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/chromeos/file_system/move_operation.h"
#include "components/drive/chromeos/file_system/operation_delegate.h"
#include "components/drive/chromeos/file_system/remove_operation.h"
#include "components/drive/chromeos/loader_controller.h"
#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/chromeos/start_page_token_loader.h"
#include "components/drive/drive.pb.h"
#include "components/drive/event_logger.h"
#include "components/drive/file_change.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/job_scheduler.h"
#include "components/drive/resource_entry_conversion.h"
#include "components/drive/service/fake_drive_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

namespace {

// The content of files initially stored in the cache.
const char kLocalContent[] = "Hello!";

// The content of files stored in the service.
const char kRemoteContent[] = "World!";

// SyncClientTestDriveService will return DRIVE_CANCELLED when a request is
// made with the specified resource ID.
class SyncClientTestDriveService : public ::drive::FakeDriveService {
 public:
  SyncClientTestDriveService() : download_file_count_(0) {}

  // FakeDriveService override:
  google_apis::CancelCallback DownloadFile(
      const base::FilePath& local_cache_path,
      const std::string& resource_id,
      const google_apis::DownloadActionCallback& download_action_callback,
      const google_apis::GetContentCallback& get_content_callback,
      const google_apis::ProgressCallback& progress_callback) override {
    ++download_file_count_;
    if (resource_id == resource_id_to_be_cancelled_) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::Bind(download_action_callback,
                     google_apis::DRIVE_CANCELLED,
                     base::FilePath()));
      return google_apis::CancelCallback();
    }
    if (resource_id == resource_id_to_be_paused_) {
      paused_action_ = base::Bind(download_action_callback,
                                  google_apis::DRIVE_OTHER_ERROR,
                                  base::FilePath());
      return google_apis::CancelCallback();
    }
    return FakeDriveService::DownloadFile(local_cache_path,
                                          resource_id,
                                          download_action_callback,
                                          get_content_callback,
                                          progress_callback);
  }

  int download_file_count() const { return download_file_count_; }

  void set_resource_id_to_be_cancelled(const std::string& resource_id) {
    resource_id_to_be_cancelled_ = resource_id;
  }

  void set_resource_id_to_be_paused(const std::string& resource_id) {
    resource_id_to_be_paused_ = resource_id;
  }

  const base::Closure& paused_action() const { return paused_action_; }

 private:
  int download_file_count_;
  std::string resource_id_to_be_cancelled_;
  std::string resource_id_to_be_paused_;
  base::Closure paused_action_;
};

}  // namespace

class SyncClientTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    pref_service_.reset(new TestingPrefServiceSimple);
    test_util::RegisterDrivePrefs(pref_service_->registry());

    fake_network_change_notifier_.reset(
        new test_util::FakeNetworkChangeNotifier);

    logger_.reset(new EventLogger);

    drive_service_.reset(new SyncClientTestDriveService);

    scheduler_.reset(new JobScheduler(
        pref_service_.get(), logger_.get(), drive_service_.get(),
        base::ThreadTaskRunnerHandle::Get().get(), nullptr));

    metadata_storage_.reset(new ResourceMetadataStorage(
        temp_dir_.GetPath(), base::ThreadTaskRunnerHandle::Get().get()));
    ASSERT_TRUE(metadata_storage_->Initialize());

    cache_.reset(new FileCache(metadata_storage_.get(), temp_dir_.GetPath(),
                               base::ThreadTaskRunnerHandle::Get().get(),
                               NULL /* free_disk_space_getter */));
    ASSERT_TRUE(cache_->Initialize());

    metadata_.reset(new internal::ResourceMetadata(
        metadata_storage_.get(), cache_.get(),
        base::ThreadTaskRunnerHandle::Get()));
    ASSERT_EQ(FILE_ERROR_OK, metadata_->Initialize());

    about_resource_loader_.reset(new AboutResourceLoader(scheduler_.get()));
    root_folder_id_loader_ = std::make_unique<AboutResourceRootFolderIdLoader>(
        about_resource_loader_.get());

    start_page_token_loader_.reset(new StartPageTokenLoader(
        drive::util::kTeamDriveIdDefaultCorpus, scheduler_.get()));
    loader_controller_.reset(new LoaderController);
    change_list_loader_.reset(new ChangeListLoader(
        logger_.get(), base::ThreadTaskRunnerHandle::Get().get(),
        metadata_.get(), scheduler_.get(), root_folder_id_loader_.get(),
        start_page_token_loader_.get(), loader_controller_.get()));
    ASSERT_NO_FATAL_FAILURE(SetUpTestData());

    sync_client_.reset(
        new SyncClient(base::ThreadTaskRunnerHandle::Get().get(), &delegate_,
                       scheduler_.get(), metadata_.get(), cache_.get(),
                       loader_controller_.get(), temp_dir_.GetPath()));

    // Disable delaying so that DoSyncLoop() starts immediately.
    sync_client_->set_delay_for_testing(base::TimeDelta::FromSeconds(0));
  }

  // Adds a file to the service root and |resource_ids_|.
  void AddFileEntry(const std::string& title) {
    google_apis::DriveApiErrorCode error = google_apis::DRIVE_FILE_ERROR;
    std::unique_ptr<google_apis::FileResource> entry;
    drive_service_->AddNewFile(
        "text/plain",
        kRemoteContent,
        drive_service_->GetRootResourceId(),
        title,
        false,  // shared_with_me
        google_apis::test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(google_apis::HTTP_CREATED, error);
    ASSERT_TRUE(entry);
    resource_ids_[title] = entry->file_id();
  }

  // Sets up data for tests.
  void SetUpTestData() {
    // Prepare a temp file.
    base::FilePath temp_file;
    EXPECT_TRUE(
        base::CreateTemporaryFileInDir(temp_dir_.GetPath(), &temp_file));
    ASSERT_TRUE(google_apis::test_util::WriteStringToFile(temp_file,
                                                          kLocalContent));

    // Add file entries to the service.
    ASSERT_NO_FATAL_FAILURE(AddFileEntry("foo"));
    ASSERT_NO_FATAL_FAILURE(AddFileEntry("bar"));
    ASSERT_NO_FATAL_FAILURE(AddFileEntry("baz"));
    ASSERT_NO_FATAL_FAILURE(AddFileEntry("fetched"));
    ASSERT_NO_FATAL_FAILURE(AddFileEntry("dirty"));
    ASSERT_NO_FATAL_FAILURE(AddFileEntry("removed"));
    ASSERT_NO_FATAL_FAILURE(AddFileEntry("moved"));

    // Load data from the service to the metadata.
    FileError error = FILE_ERROR_FAILED;
    change_list_loader_->LoadIfNeeded(
        google_apis::test_util::CreateCopyResultCallback(&error));
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(FILE_ERROR_OK, error);

    // Prepare 3 pinned-but-not-present files.
    EXPECT_EQ(FILE_ERROR_OK, cache_->Pin(GetLocalId("foo")));
    EXPECT_EQ(FILE_ERROR_OK, cache_->Pin(GetLocalId("bar")));
    EXPECT_EQ(FILE_ERROR_OK, cache_->Pin(GetLocalId("baz")));

    // Prepare a pinned-and-fetched file.
    const std::string md5_fetched = "md5";
    EXPECT_EQ(FILE_ERROR_OK,
              cache_->Store(GetLocalId("fetched"), md5_fetched,
                            temp_file, FileCache::FILE_OPERATION_COPY));
    EXPECT_EQ(FILE_ERROR_OK, cache_->Pin(GetLocalId("fetched")));

    // Prepare a pinned-and-fetched-and-dirty file.
    EXPECT_EQ(FILE_ERROR_OK,
              cache_->Store(GetLocalId("dirty"), std::string(),
                            temp_file, FileCache::FILE_OPERATION_COPY));
    EXPECT_EQ(FILE_ERROR_OK, cache_->Pin(GetLocalId("dirty")));

    // Prepare a removed file.
    file_system::RemoveOperation remove_operation(
        base::ThreadTaskRunnerHandle::Get().get(), &delegate_, metadata_.get(),
        cache_.get());
    remove_operation.Remove(
        util::GetDriveMyDriveRootPath().AppendASCII("removed"),
        false,  // is_recursive
        google_apis::test_util::CreateCopyResultCallback(&error));
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(FILE_ERROR_OK, error);

    // Prepare a moved file.
    file_system::MoveOperation move_operation(
        base::ThreadTaskRunnerHandle::Get().get(), &delegate_, metadata_.get());
    move_operation.Move(
        util::GetDriveMyDriveRootPath().AppendASCII("moved"),
        util::GetDriveMyDriveRootPath().AppendASCII("moved_new_title"),
        google_apis::test_util::CreateCopyResultCallback(&error));
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(FILE_ERROR_OK, error);
  }

 protected:
  std::string GetLocalId(const std::string& title) {
    EXPECT_EQ(1U, resource_ids_.count(title));
    std::string local_id;
    EXPECT_EQ(FILE_ERROR_OK,
              metadata_->GetIdByResourceId(resource_ids_[title], &local_id));
    return local_id;
  }

  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<test_util::FakeNetworkChangeNotifier>
      fake_network_change_notifier_;
  std::unique_ptr<EventLogger> logger_;
  std::unique_ptr<SyncClientTestDriveService> drive_service_;
  file_system::OperationDelegate delegate_;
  std::unique_ptr<JobScheduler> scheduler_;
  std::unique_ptr<ResourceMetadataStorage, test_util::DestroyHelperForTests>
      metadata_storage_;
  std::unique_ptr<FileCache, test_util::DestroyHelperForTests> cache_;
  std::unique_ptr<ResourceMetadata, test_util::DestroyHelperForTests> metadata_;
  std::unique_ptr<AboutResourceLoader> about_resource_loader_;
  std::unique_ptr<StartPageTokenLoader> start_page_token_loader_;
  std::unique_ptr<LoaderController> loader_controller_;
  std::unique_ptr<ChangeListLoader> change_list_loader_;
  std::unique_ptr<SyncClient> sync_client_;
  std::unique_ptr<AboutResourceRootFolderIdLoader> root_folder_id_loader_;

  std::map<std::string, std::string> resource_ids_;  // Name-to-id map.
};

TEST_F(SyncClientTest, StartProcessingBacklog) {
  sync_client_->StartProcessingBacklog();
  base::RunLoop().RunUntilIdle();

  ResourceEntry entry;
  // Pinned files get downloaded.
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("foo"), &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_present());

  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("bar"), &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_present());

  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("baz"), &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_present());

  // Dirty file gets uploaded.
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("dirty"), &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_dirty());

  // Removed entry is not found.
  google_apis::DriveApiErrorCode status = google_apis::DRIVE_OTHER_ERROR;
  std::unique_ptr<google_apis::FileResource> server_entry;
  drive_service_->GetFileResource(
      resource_ids_["removed"],
      google_apis::test_util::CreateCopyResultCallback(&status, &server_entry));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, status);
  ASSERT_TRUE(server_entry);
  EXPECT_TRUE(server_entry->labels().is_trashed());

  // Moved entry was moved.
  status = google_apis::DRIVE_OTHER_ERROR;
  drive_service_->GetFileResource(
      resource_ids_["moved"],
      google_apis::test_util::CreateCopyResultCallback(&status, &server_entry));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(google_apis::HTTP_SUCCESS, status);
  ASSERT_TRUE(server_entry);
  EXPECT_EQ("moved_new_title", server_entry->title());
}

TEST_F(SyncClientTest, AddFetchTask) {
  sync_client_->AddFetchTask(GetLocalId("foo"));
  base::RunLoop().RunUntilIdle();

  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("foo"), &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_present());
}

TEST_F(SyncClientTest, AddFetchTaskAndCancelled) {
  // Trigger fetching of a file which results in cancellation.
  drive_service_->set_resource_id_to_be_cancelled(resource_ids_["foo"]);
  sync_client_->AddFetchTask(GetLocalId("foo"));
  base::RunLoop().RunUntilIdle();

  // The file should be unpinned if the user wants the download to be cancelled.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("foo"), &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_pinned());
}

TEST_F(SyncClientTest, RemoveFetchTask) {
  sync_client_->AddFetchTask(GetLocalId("foo"));
  sync_client_->AddFetchTask(GetLocalId("bar"));
  sync_client_->AddFetchTask(GetLocalId("baz"));

  sync_client_->RemoveFetchTask(GetLocalId("foo"));
  sync_client_->RemoveFetchTask(GetLocalId("baz"));
  base::RunLoop().RunUntilIdle();

  // Only "bar" should be fetched.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("foo"), &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_present());

  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("bar"), &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_present());

  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("baz"), &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_present());

}

TEST_F(SyncClientTest, ExistingPinnedFiles) {
  // Start checking the existing pinned files. This will collect the resource
  // IDs of pinned files, with stale local cache files.
  sync_client_->StartCheckingExistingPinnedFiles();
  base::RunLoop().RunUntilIdle();

  // "fetched" and "dirty" are the existing pinned files.
  // The non-dirty one should be synced, but the dirty one should not.
  base::FilePath cache_file;
  std::string content;
  EXPECT_EQ(FILE_ERROR_OK, cache_->GetFile(GetLocalId("fetched"), &cache_file));
  EXPECT_TRUE(base::ReadFileToString(cache_file, &content));
  EXPECT_EQ(kRemoteContent, content);
  content.clear();

  EXPECT_EQ(FILE_ERROR_OK, cache_->GetFile(GetLocalId("dirty"), &cache_file));
  EXPECT_TRUE(base::ReadFileToString(cache_file, &content));
  EXPECT_EQ(kLocalContent, content);
}

TEST_F(SyncClientTest, RetryOnDisconnection) {
  // Let the service go down.
  drive_service_->set_offline(true);
  // Change the network connection state after some delay, to test that
  // FILE_ERROR_NO_CONNECTION is handled by SyncClient correctly.
  // Without this delay, JobScheduler will keep the jobs unrun and SyncClient
  // will receive no error.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&test_util::FakeNetworkChangeNotifier::SetConnectionType,
                 base::Unretained(fake_network_change_notifier_.get()),
                 net::NetworkChangeNotifier::CONNECTION_NONE),
      TestTimeouts::tiny_timeout());

  // Try fetch and upload.
  sync_client_->AddFetchTask(GetLocalId("foo"));
  sync_client_->AddUpdateTask(ClientContext(USER_INITIATED),
                              GetLocalId("dirty"));
  base::RunLoop().RunUntilIdle();

  // Not yet fetched nor uploaded.
  ResourceEntry entry;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("foo"), &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_present());
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("dirty"), &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_dirty());

  // Switch to online.
  fake_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  drive_service_->set_offline(false);
  base::RunLoop().RunUntilIdle();

  // Fetched and uploaded.
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("foo"), &entry));
  EXPECT_TRUE(entry.file_specific_info().cache_state().is_present());
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryById(GetLocalId("dirty"), &entry));
  EXPECT_FALSE(entry.file_specific_info().cache_state().is_dirty());
}

TEST_F(SyncClientTest, ScheduleRerun) {
  // Add a fetch task for "foo", this should result in being paused.
  drive_service_->set_resource_id_to_be_paused(resource_ids_["foo"]);
  sync_client_->AddFetchTask(GetLocalId("foo"));
  base::RunLoop().RunUntilIdle();

  // While the first task is paused, add a task again.
  // This results in scheduling rerun of the task.
  sync_client_->AddFetchTask(GetLocalId("foo"));
  base::RunLoop().RunUntilIdle();

  // Resume the paused task.
  drive_service_->set_resource_id_to_be_paused(std::string());
  ASSERT_FALSE(drive_service_->paused_action().is_null());
  drive_service_->paused_action().Run();
  base::RunLoop().RunUntilIdle();

  // Task should be run twice.
  EXPECT_EQ(2, drive_service_->download_file_count());
}

TEST_F(SyncClientTest, Dependencies) {
  // Create directories locally.
  const base::FilePath kPath1(FILE_PATH_LITERAL("drive/root/dir1"));
  const base::FilePath kPath2 = kPath1.AppendASCII("dir2");

  ResourceEntry parent;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryByPath(kPath1.DirName(), &parent));

  ResourceEntry entry1;
  entry1.set_parent_local_id(parent.local_id());
  entry1.set_title(kPath1.BaseName().AsUTF8Unsafe());
  entry1.mutable_file_info()->set_is_directory(true);
  entry1.set_metadata_edit_state(ResourceEntry::DIRTY);
  std::string local_id1;
  EXPECT_EQ(FILE_ERROR_OK, metadata_->AddEntry(entry1, &local_id1));

  ResourceEntry entry2;
  entry2.set_parent_local_id(local_id1);
  entry2.set_title(kPath2.BaseName().AsUTF8Unsafe());
  entry2.mutable_file_info()->set_is_directory(true);
  entry2.set_metadata_edit_state(ResourceEntry::DIRTY);
  std::string local_id2;
  EXPECT_EQ(FILE_ERROR_OK, metadata_->AddEntry(entry2, &local_id2));

  // Start syncing the child first.
  sync_client_->AddUpdateTask(ClientContext(USER_INITIATED), local_id2);
  // Start syncing the parent later.
  sync_client_->AddUpdateTask(ClientContext(USER_INITIATED), local_id1);
  base::RunLoop().RunUntilIdle();

  // Both entries are synced.
  EXPECT_EQ(FILE_ERROR_OK, metadata_->GetResourceEntryById(local_id1, &entry1));
  EXPECT_EQ(ResourceEntry::CLEAN, entry1.metadata_edit_state());
  EXPECT_EQ(FILE_ERROR_OK, metadata_->GetResourceEntryById(local_id2, &entry2));
  EXPECT_EQ(ResourceEntry::CLEAN, entry2.metadata_edit_state());
}

TEST_F(SyncClientTest, WaitForUpdateTaskToComplete) {
  // Create a directory locally.
  const base::FilePath kPath(FILE_PATH_LITERAL("drive/root/dir1"));

  ResourceEntry parent;
  EXPECT_EQ(FILE_ERROR_OK,
            metadata_->GetResourceEntryByPath(kPath.DirName(), &parent));

  ResourceEntry entry;
  entry.set_parent_local_id(parent.local_id());
  entry.set_title(kPath.BaseName().AsUTF8Unsafe());
  entry.mutable_file_info()->set_is_directory(true);
  entry.set_metadata_edit_state(ResourceEntry::DIRTY);
  std::string local_id;
  EXPECT_EQ(FILE_ERROR_OK, metadata_->AddEntry(entry, &local_id));

  // Sync task is not yet avialable.
  FileError error = FILE_ERROR_FAILED;
  EXPECT_FALSE(sync_client_->WaitForUpdateTaskToComplete(
      local_id, google_apis::test_util::CreateCopyResultCallback(&error)));

  // Start syncing the directory and wait for it to complete.
  sync_client_->AddUpdateTask(ClientContext(USER_INITIATED), local_id);

  EXPECT_TRUE(sync_client_->WaitForUpdateTaskToComplete(
      local_id, google_apis::test_util::CreateCopyResultCallback(&error)));

  base::RunLoop().RunUntilIdle();

  // The callback is called.
  EXPECT_EQ(FILE_ERROR_OK, error);
}

}  // namespace internal
}  // namespace drive
