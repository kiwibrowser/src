// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/sync/test/integration/bookmarks_helper.h"
#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/test/integration/updated_progress_marker_checker.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/syncable/directory.h"
#include "content/public/browser/browser_thread.h"
#include "sql/test/test_helpers.h"
#include "url/gurl.h"

using content::BrowserThread;
using base::FileEnumerator;
using base::FilePath;

namespace {

// USS ModelTypeStore uses the same folder as the Directory. However, all of its
// content is in a sub-folder. By not asking for recursive files, this function
// will avoid seeing any of those, and return iff Directory database files still
// exist.
bool FolderContainsFiles(const FilePath& folder) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  if (base::DirectoryExists(folder)) {
    return !FileEnumerator(folder, false, FileEnumerator::FILES).Next().empty();
  } else {
    return false;
  }
}

class SingleClientDirectorySyncTest : public SyncTest {
 public:
  SingleClientDirectorySyncTest() : SyncTest(SINGLE_CLIENT) {}
  ~SingleClientDirectorySyncTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleClientDirectorySyncTest);
};

void SignalEvent(base::WaitableEvent* e) {
  e->Signal();
}

bool WaitForExistingTasksOnLoop(base::MessageLoop* loop) {
  base::WaitableEvent e(base::WaitableEvent::ResetPolicy::MANUAL,
                        base::WaitableEvent::InitialState::NOT_SIGNALED);
  loop->task_runner()->PostTask(FROM_HERE, base::BindOnce(&SignalEvent, &e));
  // Timeout stolen from StatusChangeChecker::GetTimeoutDuration().
  return e.TimedWait(base::TimeDelta::FromSeconds(45));
}

// A status change checker that waits for an unrecoverable sync error to occur.
class SyncUnrecoverableErrorChecker : public SingleClientStatusChangeChecker {
 public:
  explicit SyncUnrecoverableErrorChecker(
      browser_sync::ProfileSyncService* service)
      : SingleClientStatusChangeChecker(service) {}

  bool IsExitConditionSatisfied() override {
    return service()->HasUnrecoverableError();
  }

  std::string GetDebugMessage() const override {
    return "Sync Unrecoverable Error";
  }
};

IN_PROC_BROWSER_TEST_F(SingleClientDirectorySyncTest,
                       StopThenDisableDeletesDirectory) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  browser_sync::ProfileSyncService* sync_service = GetSyncService(0);
  FilePath directory_path = sync_service->GetDirectoryPathForTest();
  ASSERT_TRUE(FolderContainsFiles(directory_path));
  sync_service->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);

  // Wait for StartupController::StartUp()'s tasks to finish.
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                run_loop.QuitClosure());
  run_loop.Run();
  // Wait for the directory deletion to finish.
  base::MessageLoop* sync_loop = sync_service->GetSyncLoopForTest();
  ASSERT_TRUE(WaitForExistingTasksOnLoop(sync_loop));
  ASSERT_FALSE(FolderContainsFiles(directory_path));
}

// Verify that when the sync directory's backing store becomes corrupted, we
// trigger an unrecoverable error and delete the database.
//
// If this test fails, see the definition of kNumEntriesRequiredForCorruption
// for one possible cause.
IN_PROC_BROWSER_TEST_F(SingleClientDirectorySyncTest,
                       DeleteDirectoryWhenCorrupted) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";
  // Sync and wait for syncing to complete.
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(UpdatedProgressMarkerChecker(GetSyncService(0)).Wait());
  ASSERT_TRUE(bookmarks_helper::ModelMatchesVerifier(0));

  // Flush the directory to the backing store and wait until the flush
  // completes.
  browser_sync::ProfileSyncService* sync_service = GetSyncService(0);
  sync_service->FlushDirectory();
  base::MessageLoop* sync_loop = sync_service->GetSyncLoopForTest();
  ASSERT_TRUE(WaitForExistingTasksOnLoop(sync_loop));

  // Now corrupt the database.
  const FilePath directory_path(sync_service->GetDirectoryPathForTest());
  const FilePath sync_db(directory_path.Append(
      syncer::syncable::Directory::kSyncDatabaseFilename));
  ASSERT_TRUE(sql::test::CorruptSizeInHeaderWithLock(sync_db));

  // Write some bookmarks and flush the directory to force sync to
  // notice the corruption.
  const GURL url("https://www.example.com");
  const bookmarks::BookmarkNode* top = bookmarks_helper::AddFolder(
      0, bookmarks_helper::GetOtherNode(0), 0, "top");
  for (int i = 0; i < 100; ++i) {
    ASSERT_TRUE(
        bookmarks_helper::AddURL(0, top, 0, base::Int64ToString(i), url));
  }
  sync_service->FlushDirectory();

  // Wait for an unrecoverable error to occur.
  ASSERT_TRUE(SyncUnrecoverableErrorChecker(sync_service).Wait());
  ASSERT_TRUE(sync_service->HasUnrecoverableError());

  // Wait until the sync loop has processed any existing tasks and see that the
  // directory no longer exists.
  ASSERT_TRUE(WaitForExistingTasksOnLoop(sync_loop));
  ASSERT_FALSE(FolderContainsFiles(directory_path));
}

}  // namespace
