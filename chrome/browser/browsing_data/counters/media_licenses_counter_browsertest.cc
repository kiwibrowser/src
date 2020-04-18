// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/counters/media_licenses_counter.h"

#include "base/run_loop.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "ppapi/shared_impl/ppapi_constants.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/common/fileapi/file_system_util.h"
#include "url/gurl.h"

namespace {

const char kTestOrigin[] = "https://host1/";
const GURL kOrigin(kTestOrigin);
const char kClearKeyCdmPluginId[] = "application_x-ppapi-clearkey-cdm";

class MediaLicensesCounterTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override { SetMediaLicenseDeletionPref(true); }

  void SetMediaLicenseDeletionPref(bool value) {
    browser()->profile()->GetPrefs()->SetBoolean(
        browsing_data::prefs::kDeleteMediaLicenses, value);
  }

  // Create some test data for origin |kOrigin|.
  void CreateMediaLicenseTestData() {
    storage::FileSystemContext* filesystem_context =
        content::BrowserContext::GetDefaultStoragePartition(
            browser()->profile())
            ->GetFileSystemContext();
    std::string clearkey_fsid =
        CreateFileSystem(filesystem_context, kClearKeyCdmPluginId, kOrigin);
    CreateFile(filesystem_context, kOrigin, clearkey_fsid, "foo");
  }

  // Start running and allow the asynchronous IO operations to complete.
  void RunAndWaitForResult() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

  // Callback from the counter.
  void CountingCallback(
      std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    callback_called_ = true;
    finished_ = result->Finished();
    if (finished_) {
      MediaLicensesCounter::MediaLicenseResult* media_result =
          static_cast<MediaLicensesCounter::MediaLicenseResult*>(result.get());
      count_ = media_result->Value();
      origin_ = media_result->GetOneOrigin();
    }

    if (run_loop_ && finished_)
      run_loop_->Quit();
  }

  bool CallbackCalled() { return callback_called_; }

  browsing_data::BrowsingDataCounter::ResultInt GetCount() {
    DCHECK(finished_);
    return count_;
  }

  const std::string& GetOrigin() {
    DCHECK(finished_);
    return origin_;
  }

 private:
  // Creates a PluginPrivateFileSystem for the |plugin_name| and |origin|
  // provided. Returns the file system ID for the created
  // PluginPrivateFileSystem.
  std::string CreateFileSystem(storage::FileSystemContext* filesystem_context,
                               const std::string& plugin_name,
                               const GURL& origin) {
    std::string fsid = storage::IsolatedContext::GetInstance()
                           ->RegisterFileSystemForVirtualPath(
                               storage::kFileSystemTypePluginPrivate,
                               ppapi::kPluginPrivateRootName, base::FilePath());
    EXPECT_TRUE(storage::ValidateIsolatedFileSystemId(fsid));
    filesystem_context->OpenPluginPrivateFileSystem(
        origin, storage::kFileSystemTypePluginPrivate, fsid, plugin_name,
        storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
        base::Bind(&MediaLicensesCounterTest::OnFileSystemOpened,
                   base::Unretained(this)));
    RunAndWaitForResult();
    return fsid;
  }

  // Creates a file named |file_name| in the PluginPrivateFileSystem identified
  // by |origin| and |fsid|.
  void CreateFile(storage::FileSystemContext* filesystem_context,
                  const GURL& origin,
                  const std::string& fsid,
                  const std::string& file_name) {
    std::string root = storage::GetIsolatedFileSystemRootURIString(
        origin, fsid, ppapi::kPluginPrivateRootName);
    storage::FileSystemURL file_url =
        filesystem_context->CrackURL(GURL(root + file_name));
    storage::AsyncFileUtil* file_util = filesystem_context->GetAsyncFileUtil(
        storage::kFileSystemTypePluginPrivate);
    std::unique_ptr<storage::FileSystemOperationContext> operation_context =
        std::make_unique<storage::FileSystemOperationContext>(
            filesystem_context);
    operation_context->set_allowed_bytes_growth(
        storage::QuotaManager::kNoLimit);
    file_util->EnsureFileExists(
        std::move(operation_context), file_url,
        base::Bind(&MediaLicensesCounterTest::OnFileCreated,
                   base::Unretained(this)));
    RunAndWaitForResult();
  }

  void OnFileSystemOpened(base::File::Error result) {
    EXPECT_EQ(base::File::FILE_OK, result) << base::File::ErrorToString(result);
    if (run_loop_)
      run_loop_->Quit();
  }

  void OnFileCreated(base::File::Error result, bool created) {
    EXPECT_EQ(base::File::FILE_OK, result) << base::File::ErrorToString(result);
    EXPECT_TRUE(created);
    if (run_loop_)
      run_loop_->Quit();
  }

  bool callback_called_ = false;
  bool finished_ = false;
  browsing_data::BrowsingDataCounter::ResultInt count_;
  std::string origin_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

// Tests that for the empty file system, the result is zero.
IN_PROC_BROWSER_TEST_F(MediaLicensesCounterTest, Empty) {
  Profile* profile = browser()->profile();
  std::unique_ptr<MediaLicensesCounter> counter =
      MediaLicensesCounter::Create(profile);
  counter->Init(profile->GetPrefs(),
                browsing_data::ClearBrowsingDataTab::ADVANCED,
                base::Bind(&MediaLicensesCounterTest::CountingCallback,
                           base::Unretained(this)));
  counter->Restart();

  RunAndWaitForResult();

  EXPECT_TRUE(CallbackCalled());
  EXPECT_EQ(0u, GetCount());
  EXPECT_TRUE(GetOrigin().empty());
}

// Tests that for a non-empty file system, the result is nonzero.
IN_PROC_BROWSER_TEST_F(MediaLicensesCounterTest, NonEmpty) {
  CreateMediaLicenseTestData();

  Profile* profile = browser()->profile();
  std::unique_ptr<MediaLicensesCounter> counter =
      MediaLicensesCounter::Create(profile);
  counter->Init(profile->GetPrefs(),
                browsing_data::ClearBrowsingDataTab::ADVANCED,
                base::Bind(&MediaLicensesCounterTest::CountingCallback,
                           base::Unretained(this)));
  counter->Restart();

  RunAndWaitForResult();

  EXPECT_TRUE(CallbackCalled());
  EXPECT_EQ(1u, GetCount());
  EXPECT_EQ(kOrigin.host(), GetOrigin());
}

// Tests that the counter starts counting automatically when the deletion
// pref changes to true.
IN_PROC_BROWSER_TEST_F(MediaLicensesCounterTest, PrefChanged) {
  SetMediaLicenseDeletionPref(false);

  Profile* profile = browser()->profile();
  std::unique_ptr<MediaLicensesCounter> counter =
      MediaLicensesCounter::Create(profile);
  counter->Init(profile->GetPrefs(),
                browsing_data::ClearBrowsingDataTab::ADVANCED,
                base::Bind(&MediaLicensesCounterTest::CountingCallback,
                           base::Unretained(this)));
  SetMediaLicenseDeletionPref(true);

  RunAndWaitForResult();

  EXPECT_TRUE(CallbackCalled());
  EXPECT_EQ(0u, GetCount());
  EXPECT_TRUE(GetOrigin().empty());
}

}  // namespace
