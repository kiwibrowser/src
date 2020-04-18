// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/webrtc_logging/browser/log_cleanup.h"

#include <stddef.h>

#include <string>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "components/webrtc_logging/browser/log_list.h"

namespace webrtc_logging {

namespace {

const int kDaysToKeepLogs = 5;

// Remove any empty entries from the log list. One line is one log entry, see
// WebRtcLogUploader::AddLocallyStoredLogInfoToUploadListFile for more
// information about the format.
void RemoveEmptyEntriesFromLogList(std::string* log_list) {
  // TODO(crbug.com/826253): Make this more robust to errors; corrupt entries
  // should also be removed. (Better to move away from a .csv altogether.)
  static const char kEmptyLineStart[] = ",,,";  // And a timestamp after it.
  size_t pos = 0;
  do {
    pos = log_list->find(kEmptyLineStart, pos);
    if (pos == std::string::npos)
      break;
    const size_t line_end = log_list->find("\n", pos);
    DCHECK(line_end == std::string::npos || pos < line_end);
    const size_t delete_len =
        line_end == std::string::npos ? std::string::npos : line_end - pos + 1;
    log_list->erase(pos, delete_len);
  } while (pos < log_list->size());
}

}  // namespace

void DeleteOldWebRtcLogFiles(const base::FilePath& log_dir) {
  DeleteOldAndRecentWebRtcLogFiles(log_dir, base::Time::Max());
}

void DeleteOldAndRecentWebRtcLogFiles(const base::FilePath& log_dir,
                                      const base::Time& delete_begin_time) {
  base::AssertBlockingAllowed();

  if (!base::PathExists(log_dir)) {
    // This will happen if no logs have been stored or uploaded.
    DVLOG(3) << "Could not find directory: " << log_dir.value();
    return;
  }

  const base::Time now = base::Time::Now();
  const base::TimeDelta time_to_keep_logs =
      base::TimeDelta::FromDays(kDaysToKeepLogs);

  base::FilePath log_list_path =
      LogList::GetWebRtcLogListFileForDirectory(log_dir);
  std::string log_list;
  const bool update_log_list = base::PathExists(log_list_path);
  if (update_log_list) {
    constexpr size_t kMaxIndexSizeBytes = 1000000;  // Intentional overshot.
    const bool read_ok = base::ReadFileToStringWithMaxSize(
        log_list_path, &log_list, kMaxIndexSizeBytes);
    if (!read_ok) {
      // If the maximum size was exceeded, updating it will corrupt it. However,
      // the size would not be exceeded unless the user edits it manually.
      LOG(ERROR) << "Couldn't read WebRTC textual logs list (" << log_list_path
                 << ").";
    }
  }

  // Delete relevant logs files (and their associated entries in the index).
  base::FileEnumerator log_files(log_dir, false, base::FileEnumerator::FILES);
  bool delete_ok = true;
  for (base::FilePath name = log_files.Next(); !name.empty();
       name = log_files.Next()) {
    if (name == log_list_path)
      continue;
    base::FileEnumerator::FileInfo file_info(log_files.GetInfo());
    // TODO(crbug.com/827167): Handle mismatch between timestamps of the .gz
    // file and the .meta file, as well as with the index.
    base::TimeDelta file_age = now - file_info.GetLastModifiedTime();
    if (file_age > time_to_keep_logs ||
        (!delete_begin_time.is_max() &&
         file_info.GetLastModifiedTime() > delete_begin_time)) {
      if (!base::DeleteFile(name, false))
        delete_ok = false;

      // Remove the local ID from the log list file. The ID is guaranteed to be
      // unique.
      std::string id = file_info.GetName().RemoveExtension().MaybeAsASCII();
      size_t id_pos = log_list.find(id);
      if (id_pos == std::string::npos)
        continue;
      log_list.erase(id_pos, id.size());
    }
  }

  if (!delete_ok)
    LOG(WARNING) << "Could not delete all old WebRTC logs.";

  // TODO(crbug.com/826254): Purge index file separately, too, to ensure
  // entries for logs whose files were manually removed, are also subject
  // to expiry and browsing data clearing.

  RemoveEmptyEntriesFromLogList(&log_list);

  if (update_log_list) {
    int written = base::WriteFile(log_list_path, &log_list[0], log_list.size());
    DPCHECK(written == static_cast<int>(log_list.size()));
  }
}

}  // namespace webrtc_logging
