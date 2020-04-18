// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/file_manager/path_util.h"
#include "chrome/browser/chromeos/fileapi/recent_download_source.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "chrome/browser/chromeos/fileapi/recent_source.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/browser/test/test_file_system_context.h"
#include "storage/common/fileapi/file_system_mount_option.h"
#include "storage/common/fileapi/file_system_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chromeos {

class RecentDownloadSourceTest : public testing::Test {
 public:
  RecentDownloadSourceTest() : origin_("https://example.com/") {}

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    file_system_context_ = content::CreateFileSystemContextForTesting(
        nullptr, temp_dir_.GetPath());

    RegisterFakeDownloadsFileSystem();

    source_ = std::make_unique<RecentDownloadSource>(profile_.get());
  }

 protected:
  void RegisterFakeDownloadsFileSystem() const {
    storage::ExternalMountPoints* mount_points =
        storage::ExternalMountPoints::GetSystemInstance();
    std::string mount_point_name =
        file_manager::util::GetDownloadsMountPointName(profile_.get());

    mount_points->RevokeFileSystem(mount_point_name);
    ASSERT_TRUE(mount_points->RegisterFileSystem(
        mount_point_name, storage::kFileSystemTypeTest,
        storage::FileSystemMountOption(), base::FilePath()));
  }

  bool CreateEmptyFile(const std::string& filename, const base::Time& time) {
    base::File file(temp_dir_.GetPath().Append(filename),
                    base::File::FLAG_CREATE | base::File::FLAG_WRITE);
    if (!file.IsValid())
      return false;

    return file.SetTimes(time, time);
  }

  std::vector<RecentFile> GetRecentFiles(size_t max_files,
                                         const base::Time& cutoff_time) {
    std::vector<RecentFile> files;

    base::RunLoop run_loop;

    source_->GetRecentFiles(RecentSource::Params(
        file_system_context_.get(), origin_, max_files, cutoff_time,
        base::BindOnce(
            [](base::RunLoop* run_loop, std::vector<RecentFile>* out_files,
               std::vector<RecentFile> files) {
              run_loop->Quit();
              *out_files = std::move(files);
            },
            &run_loop, &files)));

    run_loop.Run();

    return files;
  }

  content::TestBrowserThreadBundle thread_bundle_;
  const GURL origin_;
  std::unique_ptr<TestingProfile> profile_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;
  std::unique_ptr<RecentDownloadSource> source_;
  base::Time base_time_;
};

TEST_F(RecentDownloadSourceTest, GetRecentFiles) {
  // Oldest
  ASSERT_TRUE(CreateEmptyFile("1.jpg", base::Time::FromJavaTime(1000)));
  ASSERT_TRUE(CreateEmptyFile("2.jpg", base::Time::FromJavaTime(2000)));
  ASSERT_TRUE(CreateEmptyFile("3.jpg", base::Time::FromJavaTime(3000)));
  ASSERT_TRUE(CreateEmptyFile("4.jpg", base::Time::FromJavaTime(4000)));
  // Newest

  std::vector<RecentFile> files = GetRecentFiles(3, base::Time());

  std::sort(files.begin(), files.end(), RecentFileComparator());

  ASSERT_EQ(3u, files.size());
  EXPECT_EQ("4.jpg", files[0].url().path().BaseName().value());
  EXPECT_EQ(base::Time::FromJavaTime(4000), files[0].last_modified());
  EXPECT_EQ("3.jpg", files[1].url().path().BaseName().value());
  EXPECT_EQ(base::Time::FromJavaTime(3000), files[1].last_modified());
  EXPECT_EQ("2.jpg", files[2].url().path().BaseName().value());
  EXPECT_EQ(base::Time::FromJavaTime(2000), files[2].last_modified());
}

TEST_F(RecentDownloadSourceTest, GetRecentFiles_CutoffTime) {
  // Oldest
  ASSERT_TRUE(CreateEmptyFile("1.jpg", base::Time::FromJavaTime(1000)));
  ASSERT_TRUE(CreateEmptyFile("2.jpg", base::Time::FromJavaTime(2000)));
  ASSERT_TRUE(CreateEmptyFile("3.jpg", base::Time::FromJavaTime(3000)));
  ASSERT_TRUE(CreateEmptyFile("4.jpg", base::Time::FromJavaTime(4000)));
  // Newest

  std::vector<RecentFile> files =
      GetRecentFiles(3, base::Time::FromJavaTime(2500));

  std::sort(files.begin(), files.end(), RecentFileComparator());

  ASSERT_EQ(2u, files.size());
  EXPECT_EQ("4.jpg", files[0].url().path().BaseName().value());
  EXPECT_EQ(base::Time::FromJavaTime(4000), files[0].last_modified());
  EXPECT_EQ("3.jpg", files[1].url().path().BaseName().value());
  EXPECT_EQ(base::Time::FromJavaTime(3000), files[1].last_modified());
}

TEST_F(RecentDownloadSourceTest, GetRecentFiles_UmaStats) {
  base::HistogramTester histogram_tester;

  GetRecentFiles(3, base::Time());

  histogram_tester.ExpectTotalCount(RecentDownloadSource::kLoadHistogramName,
                                    1);
}

}  // namespace chromeos
