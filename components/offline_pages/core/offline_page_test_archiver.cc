// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_test_archiver.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace offline_pages {

OfflinePageTestArchiver::OfflinePageTestArchiver(
    Observer* observer,
    const GURL& url,
    ArchiverResult result,
    const base::string16& result_title,
    int64_t size_to_report,
    const std::string& digest_to_report,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : observer_(observer),
      url_(url),
      result_(result),
      size_to_report_(size_to_report),
      create_archive_called_(false),
      publish_archive_called_(false),
      delayed_(false),
      result_title_(result_title),
      digest_to_report_(digest_to_report),
      task_runner_(task_runner) {}

OfflinePageTestArchiver::~OfflinePageTestArchiver() {
  EXPECT_TRUE(create_archive_called_ || publish_archive_called_);
}

void OfflinePageTestArchiver::CreateArchive(
    const base::FilePath& archives_dir,
    const CreateArchiveParams& create_archive_params,
    content::WebContents* web_contents,
    const CreateArchiveCallback& callback) {
  create_archive_called_ = true;
  callback_ = callback;
  archives_dir_ = archives_dir;
  create_archive_params_ = create_archive_params;
  if (!delayed_)
    CompleteCreateArchive();
}

void OfflinePageTestArchiver::PublishArchive(
    const OfflinePageItem& offline_page,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    const base::FilePath& new_file_path,
    SystemDownloadManager* download_manager,
    PublishArchiveDoneCallback publish_done_callback) {
  publish_archive_called_ = true;
  publish_archive_result_.move_result = SavePageResult::SUCCESS;
  publish_archive_result_.new_file_path = offline_page.file_path;
  publish_archive_result_.download_id = 0;
  std::move(publish_done_callback).Run(offline_page, &publish_archive_result_);
}

void OfflinePageTestArchiver::CompleteCreateArchive() {
  DCHECK(!callback_.is_null());
  base::FilePath archive_path;
  if (filename_.empty()) {
    ASSERT_TRUE(base::CreateTemporaryFileInDir(archives_dir_, &archive_path));
  } else {
    archive_path = archives_dir_.Append(filename_);
    // This step ensures the file is created and closed immediately.
    base::File file(archive_path, base::File::FLAG_OPEN_ALWAYS);
  }
  observer_->SetLastPathCreatedByArchiver(archive_path);
  task_runner_->PostTask(
      FROM_HERE, base::Bind(callback_, this, result_, url_, archive_path,
                            result_title_, size_to_report_, digest_to_report_));
}

}  // namespace offline_pages
