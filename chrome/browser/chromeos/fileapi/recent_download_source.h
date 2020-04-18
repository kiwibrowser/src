// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_DOWNLOAD_SOURCE_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_DOWNLOAD_SOURCE_H_

#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "chrome/browser/chromeos/fileapi/recent_model.h"
#include "chrome/browser/chromeos/fileapi/recent_source.h"
#include "storage/browser/fileapi/file_system_operation.h"

class Profile;

namespace chromeos {

// RecentSource implementation for Downloads files.
//
// All member functions must be called on the UI thread.
class RecentDownloadSource : public RecentSource {
 public:
  explicit RecentDownloadSource(Profile* profile);
  ~RecentDownloadSource() override;

  // RecentSource overrides:
  void GetRecentFiles(Params params) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(RecentDownloadSourceTest, GetRecentFiles_UmaStats);

  static const char kLoadHistogramName[];

  void ScanDirectory(const base::FilePath& path);
  void OnReadDirectory(const base::FilePath& path,
                       base::File::Error result,
                       storage::FileSystemOperation::FileEntryList entries,
                       bool has_more);
  void OnGetMetadata(const storage::FileSystemURL& url,
                     base::File::Error result,
                     const base::File::Info& info);
  void OnReadOrStatFinished();

  storage::FileSystemURL BuildDownloadsURL(const base::FilePath& path) const;

  const std::string mount_point_name_;

  // Parameters given to GetRecentFiles().
  base::Optional<Params> params_;

  // Time when the build started.
  base::TimeTicks build_start_time_;
  // Number of ReadDirectory() calls in flight.
  int inflight_readdirs_ = 0;
  // Number of GetMetadata() calls in flight.
  int inflight_stats_ = 0;
  // Most recently modified files.
  std::priority_queue<RecentFile, std::vector<RecentFile>, RecentFileComparator>
      recent_files_;

  base::WeakPtrFactory<RecentDownloadSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RecentDownloadSource);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_DOWNLOAD_SOURCE_H_
