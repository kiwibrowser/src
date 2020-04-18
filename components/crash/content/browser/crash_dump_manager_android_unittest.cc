// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/browser/crash_dump_manager_android.h"

#include <utility>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_restrictions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace breakpad {

class CrashDumpManagerTest : public testing::Test {
 public:
  CrashDumpManagerTest()
      : scoped_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}
  ~CrashDumpManagerTest() override {}

  void SetUp() override {
    // Initialize the manager.
    ASSERT_TRUE(CrashDumpManager::GetInstance());
  }

  // TODO(csharrison): This test harness is not robust enough. Namely, it does
  // not support actually processing a non-empty crash dump, due to JNI calls.
  static void CreateAndProcessEmptyMinidump(
      const CrashDumpObserver::TerminationInfo& info) {
    base::ScopedFD fd =
        CrashDumpManager::GetInstance()->CreateMinidumpFileForChild(
            info.process_host_id);
    EXPECT_TRUE(fd.is_valid());
    base::ScopedTempDir dump_dir;
    EXPECT_TRUE(dump_dir.CreateUniqueTempDir());
    CrashDumpManager::GetInstance()->ProcessMinidumpFileFromChild(
        dump_dir.GetPath(), info);
  }

 private:
  base::ShadowingAtExitManager at_exit_;
  base::test::ScopedTaskEnvironment scoped_environment_;
  DISALLOW_COPY_AND_ASSIGN(CrashDumpManagerTest);
};

class CrashDumpManagerObserver : public CrashDumpManager::Observer {
 public:
  CrashDumpManagerObserver() {}
  ~CrashDumpManagerObserver() {}

  // CrashDumpManager::Observer:
  void OnCrashDumpProcessed(
      const CrashDumpManager::CrashDumpDetails& details) override {
    last_details_ = details;
    if (!wait_closure_.is_null())
      std::move(wait_closure_).Run();
  }

  const CrashDumpManager::CrashDumpDetails& last_details() const {
    return last_details_.value();
  }

  void WaitForProcessed() {
    base::RunLoop run_loop;
    wait_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

 private:
  base::OnceClosure wait_closure_;
  base::Optional<CrashDumpManager::CrashDumpDetails> last_details_;
  DISALLOW_COPY_AND_ASSIGN(CrashDumpManagerObserver);
};

TEST_F(CrashDumpManagerTest, SimpleOOM) {
  base::HistogramTester histogram_tester;
  CrashDumpManager* manager = CrashDumpManager::GetInstance();

  CrashDumpManagerObserver crash_dump_observer;
  manager->AddObserver(&crash_dump_observer);

  CrashDumpObserver::TerminationInfo termination_info;
  termination_info.process_host_id = 1;
  termination_info.pid = base::kNullProcessHandle;
  termination_info.process_type = content::PROCESS_TYPE_RENDERER;
  termination_info.app_state =
      base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES;
  termination_info.normal_termination = false;
  termination_info.has_oom_protection_bindings = true;
  termination_info.was_killed_intentionally_by_browser = false;
  termination_info.was_oom_protected_status = true;
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&CrashDumpManagerTest::CreateAndProcessEmptyMinidump,
                 termination_info));
  crash_dump_observer.WaitForProcessed();

  const CrashDumpManager::CrashDumpDetails& details =
      crash_dump_observer.last_details();
  EXPECT_EQ(termination_info.process_host_id, details.process_host_id);
  EXPECT_EQ(content::PROCESS_TYPE_RENDERER, details.process_type);
  EXPECT_TRUE(details.was_oom_protected_status);
  EXPECT_EQ(base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES,
            details.app_state);
  EXPECT_EQ(0, details.file_size);
  EXPECT_EQ(CrashDumpManager::CrashDumpStatus::kEmptyDump, details.status);

  histogram_tester.ExpectUniqueSample(
      "Tab.RendererDetailedExitStatus",
      CrashDumpManager::EMPTY_MINIDUMP_WHILE_RUNNING, 1);
}

TEST_F(CrashDumpManagerTest, NoDumpCreated) {
  base::HistogramTester histogram_tester;
  CrashDumpManager* manager = CrashDumpManager::GetInstance();

  CrashDumpManagerObserver crash_dump_observer;
  manager->AddObserver(&crash_dump_observer);

  CrashDumpObserver::TerminationInfo termination_info;
  termination_info.process_host_id = 1;
  termination_info.pid = base::kNullProcessHandle;
  termination_info.process_type = content::PROCESS_TYPE_RENDERER;
  termination_info.app_state =
      base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES;
  termination_info.normal_termination = false;
  termination_info.has_oom_protection_bindings = true;
  termination_info.was_killed_intentionally_by_browser = false;
  termination_info.was_oom_protected_status = true;
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&CrashDumpManager::ProcessMinidumpFileFromChild,
                 base::Unretained(manager), base::FilePath(),
                 termination_info));
  crash_dump_observer.WaitForProcessed();

  const CrashDumpManager::CrashDumpDetails& details =
      crash_dump_observer.last_details();
  EXPECT_EQ(termination_info.process_host_id, details.process_host_id);
  EXPECT_EQ(content::PROCESS_TYPE_RENDERER, details.process_type);
  EXPECT_TRUE(details.was_oom_protected_status);
  EXPECT_EQ(base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES,
            details.app_state);
  EXPECT_EQ(0, details.file_size);
  EXPECT_EQ(CrashDumpManager::CrashDumpStatus::kNoDump, details.status);

  histogram_tester.ExpectTotalCount("Tab.RendererDetailedExitStatus", 0);
}

}  // namespace breakpad
