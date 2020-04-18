// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/download/download_directory_util.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"

namespace {
// Synchronously deletes downloads directory.
void DeleteDownloadsDirectorySync() {
  base::FilePath downloads_directory;
  if (GetDownloadsDirectory(&downloads_directory)) {
    DeleteFile(downloads_directory, /*recursive=*/true);
  }
}
}  // namespace

bool GetDownloadsDirectory(base::FilePath* directory_path) {
  if (!GetTempDir(directory_path)) {
    return false;
  }
  *directory_path = directory_path->Append("downloads");
  return true;
}

void DeleteDownloadsDirectory() {
  base::PostTaskWithTraits(FROM_HERE,
                           {base::MayBlock(), base::TaskPriority::BACKGROUND},
                           base::BindOnce(&DeleteDownloadsDirectorySync));
}
