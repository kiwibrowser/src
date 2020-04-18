// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/file_write_watcher.h"

#include <set>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

namespace {

class TestObserver {
 public:
  // After all the resource_ids in |expected_upload| are notified for the
  // need of uploading, runs |quit_closure|. Also checks that each id is
  // notified only once.
  TestObserver(const std::set<std::string>& expected_upload,
               const base::Closure& quit_closure)
      : expected_upload_(expected_upload),
        quit_closure_(quit_closure) {
  }

  void OnWrite(const std::string& id) {
    EXPECT_EQ(1U, expected_upload_.count(id)) << id;
    expected_upload_.erase(id);
    if (expected_upload_.empty())
      quit_closure_.Run();
  }

 private:
  std::set<std::string> expected_upload_;
  base::Closure quit_closure_;
};

// Writes something on the file at |path|.
void WriteSomethingAfterStartWatch(const base::FilePath& path,
                                   bool watch_success) {
  EXPECT_TRUE(watch_success) << path.value();

  const char kDummy[] = "hello";
  ASSERT_EQ(static_cast<int>(arraysize(kDummy)),
            base::WriteFile(path, kDummy, arraysize(kDummy)));
}

class FileWriteWatcherTest : public testing::Test {
 protected:
  // The test requires UI thread (FileWriteWatcher DCHECKs that its public
  // interface is running on UI thread) and FILE thread (Linux version of
  // base::FilePathWatcher needs to live on an IOAllowed thread with TYPE_IO,
  // which is FILE thread in the production environment).
  //
  // By using the IO_MAINLOOP test thread bundle, the main thread is used
  // both as UI and FILE thread, with TYPE_IO message loop.
  FileWriteWatcherTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
  }

  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }

  base::FilePath GetTempPath(const std::string& name) {
    return temp_dir_.GetPath().Append(name);
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;
};

}  // namespace

TEST_F(FileWriteWatcherTest, WatchThreeFiles) {
  std::set<std::string> expected;
  expected.insert("1");
  expected.insert("2");
  expected.insert("3");

  base::RunLoop loop;
  TestObserver observer(expected, loop.QuitClosure());

  // Set up the watcher.
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()});
  FileWriteWatcher watcher(task_runner.get());
  watcher.DisableDelayForTesting();

  // Start watching and running.
  base::FilePath path1 = GetTempPath("foo.txt");
  base::FilePath path2 = GetTempPath("bar.png");
  base::FilePath path3 = GetTempPath("buz.doc");
  base::FilePath path4 = GetTempPath("mya.mp3");
  watcher.StartWatch(
      path1,
      base::Bind(&WriteSomethingAfterStartWatch, path1),
      base::Bind(&TestObserver::OnWrite, base::Unretained(&observer), "1"));
  watcher.StartWatch(
      path2,
      base::Bind(&WriteSomethingAfterStartWatch, path2),
      base::Bind(&TestObserver::OnWrite, base::Unretained(&observer), "2"));
  watcher.StartWatch(
      path3,
      base::Bind(&WriteSomethingAfterStartWatch, path3),
      base::Bind(&TestObserver::OnWrite, base::Unretained(&observer), "3"));

  // Unwatched write. It shouldn't be notified.
  WriteSomethingAfterStartWatch(path4, true);

  // The loop should quit if all the three paths are notified to be written.
  loop.Run();
}

}  // namespace internal
}  // namespace drive
