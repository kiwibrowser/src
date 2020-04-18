// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_image_retainer.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/hash.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/common/chrome_paths.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

using base::FilePath;

namespace {

constexpr base::FilePath::CharType kImageRoot[] =
    FILE_PATH_LITERAL("Notification Resources");

// How long to keep the temp files before deleting them. The formula for picking
// the delay is t * (n + 1), where t is the default on-screen display time for
// an Action Center notification (6 seconds) and n is the number of
// notifications that can be shown on-screen at once (1).
constexpr base::TimeDelta kDeletionDelay = base::TimeDelta::FromSeconds(12);

// Writes |data| to a new temporary file and returns the path to the new file,
// or an empty path if the function fails.
FilePath WriteDataToTmpFile(const FilePath& file_path,
                            const base::string16& subdirectory,
                            const scoped_refptr<base::RefCountedMemory>& data) {
  int data_len = data->size();
  if (data_len == 0)
    return FilePath();

  FilePath new_temp = file_path.Append(subdirectory);
  if (!base::CreateDirectoryAndGetError(new_temp, nullptr))
    return FilePath();

  FilePath temp_file;
  if (!base::CreateTemporaryFileInDir(new_temp, &temp_file))
    return FilePath();
  if (base::WriteFile(temp_file, data->front_as<char>(), data_len) != data_len)
    return FilePath();
  return temp_file;
}

}  // namespace

bool NotificationImageRetainer::override_file_destruction_ = false;

NotificationImageRetainer::NotificationImageRetainer(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : task_runner_(task_runner) {}

NotificationImageRetainer::~NotificationImageRetainer() {
  if (!image_directory_.empty())
    base::DeleteFile(image_directory_, true);
}

FilePath NotificationImageRetainer::RegisterTemporaryImage(
    const gfx::Image& image,
    const std::string& profile_id,
    const GURL& origin) {
  base::AssertBlockingAllowed();
  if (!initialized_) {
    image_directory_ = DetermineImageDirectory();
    // Delete the old image directory.
    DeleteFile(image_directory_, /*recursive=*/true);
    // Recreate the image directory.
    if (!base::CreateDirectoryAndGetError(image_directory_, nullptr))
      return FilePath();
    initialized_ = true;
  }

  // To minimize the risk of collisions, separate each request by subdirectory
  // generated from hashes of the profile and the origin. Each file within the
  // subdirectory will also be given a unique filename.
  base::string16 directory = base::UintToString16(base::Hash(
      base::UTF8ToUTF16(profile_id) + base::UTF8ToUTF16(origin.spec())));
  FilePath temp_file =
      WriteDataToTmpFile(image_directory_, directory, image.As1xPNGBytes());

  // Add a future task to try to delete the file. It is OK to fail, the file
  // will get deleted later.
  base::TimeDelta delay = override_file_destruction_
                              ? base::TimeDelta::FromSeconds(0)
                              : kDeletionDelay;
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&base::DeleteFile), temp_file,
                 /*recursive=*/true),
      delay);

  return temp_file;
}

// static
void NotificationImageRetainer::OverrideTempFileLifespanForTesting(
    bool override) {
  override_file_destruction_ = override;
}

FilePath NotificationImageRetainer::DetermineImageDirectory() {
  FilePath data_dir;
  bool success = base::PathService::Get(chrome::DIR_USER_DATA, &data_dir);
  DCHECK(success);
  return data_dir.Append(kImageRoot);
}
