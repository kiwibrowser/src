// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_archiver.h"

#include "base/strings/utf_string_conversions.h"

#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "components/offline_pages/core/model/offline_page_model_taskified.h"
#include "components/offline_pages/core/model/offline_page_model_utils.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "components/offline_pages/core/system_download_manager.h"

namespace {

// Helper function to do the move and register synchronously. Make sure this is
// called from a background thread.
void MoveAndRegisterArchive(
    const offline_pages::OfflinePageItem& offline_page,
    const base::FilePath& publish_directory,
    offline_pages::SystemDownloadManager* download_manager,
    offline_pages::PublishArchiveResult* archive_result) {
  // Calculate the new file name.
  base::FilePath new_file_path =
      offline_pages::model_utils::GenerateUniqueFilenameForOfflinePage(
          offline_page.title, offline_page.url, publish_directory);

  // Move the file.
  bool moved = base::Move(offline_page.file_path, new_file_path);
  if (!moved) {
    archive_result->move_result =
        offline_pages::SavePageResult::FILE_MOVE_FAILED;
    return;
  }

  // Tell the download manager about our file, get back an id.
  if (!download_manager->IsDownloadManagerInstalled()) {
    archive_result->move_result =
        offline_pages::SavePageResult::ADD_TO_DOWNLOAD_MANAGER_FAILED;
    return;
  }

  // TODO(petewil): Handle empty page title.
  std::string page_title = base::UTF16ToUTF8(offline_page.title);
  // We use the title for a description, since the add to the download manager
  // fails without a description, and we don't have anything better to use.
  int64_t download_id = download_manager->AddCompletedDownload(
      page_title, page_title,
      offline_pages::store_utils::ToDatabaseFilePath(new_file_path),
      offline_page.file_size, offline_page.url.spec(), std::string());
  if (download_id == 0LL) {
    archive_result->move_result =
        offline_pages::SavePageResult::ADD_TO_DOWNLOAD_MANAGER_FAILED;
    return;
  }

  // Put results into the result object.
  archive_result->move_result = offline_pages::SavePageResult::SUCCESS;
  archive_result->new_file_path = new_file_path;
  archive_result->download_id = download_id;

  return;
}

}  // namespace

namespace offline_pages {

void OfflinePageArchiver::PublishArchive(
    const OfflinePageItem& offline_page,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    const base::FilePath& publish_directory,
    SystemDownloadManager* download_manager,
    PublishArchiveDoneCallback archive_done_callback) {
  PublishArchiveResult* archive_results = new PublishArchiveResult();
  background_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&MoveAndRegisterArchive, offline_page, publish_directory,
                     download_manager, base::Unretained(archive_results)),
      base::BindOnce(std::move(archive_done_callback), offline_page,
                     base::Owned(archive_results)));
}

}  // namespace offline_pages
