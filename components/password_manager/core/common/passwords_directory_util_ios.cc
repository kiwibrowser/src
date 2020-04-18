// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/common/passwords_directory_util_ios.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_restrictions.h"

namespace password_manager {

namespace {
// Synchronously deletes passwords directoy.
void DeletePasswordsDirectorySync() {
  base::AssertBlockingAllowed();
  base::FilePath downloads_directory;
  if (GetPasswordsDirectory(&downloads_directory)) {
    // It is assumed that deleting the directory always succeeds.
    DeleteFile(downloads_directory, /*recursive=*/true);
  }
}
}  // namespace

bool GetPasswordsDirectory(base::FilePath* directory_path) {
  if (!GetTempDir(directory_path)) {
    return false;
  }
  *directory_path = directory_path->Append(FILE_PATH_LITERAL("passwords"));
  return true;
}

void DeletePasswordsDirectory() {
  base::PostTaskWithTraits(FROM_HERE,
                           {base::MayBlock(), base::TaskPriority::BACKGROUND,
                            base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
                           base::BindOnce(&DeletePasswordsDirectorySync));
}

}  // namespace password_manager
