// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_delegate_chromeos.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/process/process_handle.h"
#include "chrome/browser/resource_coordinator/test_lifecycle_unit.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chromeos/dbus/fake_debug_daemon_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {
namespace {

class TabManagerDelegateTest : public testing::Test {
 public:
  TabManagerDelegateTest() {}
  ~TabManagerDelegateTest() override {}

 private:
  content::TestBrowserThreadBundle thread_bundle_;
};

constexpr bool kIsFocused = true;
constexpr bool kNotFocused = false;

}  // namespace

TEST_F(TabManagerDelegateTest, CandidatesSorted) {
  std::vector<arc::ArcProcess> arc_processes;
  arc_processes.emplace_back(1, 10, "focused", arc::mojom::ProcessState::TOP,
                             kIsFocused, 100);
  arc_processes.emplace_back(2, 20, "visible1", arc::mojom::ProcessState::TOP,
                             kNotFocused, 200);
  arc_processes.emplace_back(
      3, 30, "service", arc::mojom::ProcessState::SERVICE, kNotFocused, 500);
  arc_processes.emplace_back(4, 40, "visible2", arc::mojom::ProcessState::TOP,
                             kNotFocused, 150);

  TestLifecycleUnit focused_lifecycle_unit(base::TimeTicks::Max());
  TestLifecycleUnit protected_lifecycle_unit(
      base::TimeTicks() + base::TimeDelta::FromSeconds(5), 0, false);
  TestLifecycleUnit non_focused_lifecycle_unit(base::TimeTicks() +
                                               base::TimeDelta::FromSeconds(1));
  TestLifecycleUnit other_non_focused_lifecycle_unit(
      base::TimeTicks() + base::TimeDelta::FromSeconds(2));
  LifecycleUnitVector lifecycle_units{
      &focused_lifecycle_unit, &protected_lifecycle_unit,
      &non_focused_lifecycle_unit, &other_non_focused_lifecycle_unit};

  std::vector<TabManagerDelegate::Candidate> candidates;

  candidates =
      TabManagerDelegate::GetSortedCandidates(lifecycle_units, arc_processes);
  ASSERT_EQ(8U, candidates.size());

  // focused LifecycleUnit
  EXPECT_EQ(candidates[0].lifecycle_unit(), &focused_lifecycle_unit);
  // focused app.
  ASSERT_TRUE(candidates[1].app());
  EXPECT_EQ("focused", candidates[1].app()->process_name());
  // visible app 1, last_activity_time larger than visible app 2.
  ASSERT_TRUE(candidates[2].app());
  EXPECT_EQ("visible1", candidates[2].app()->process_name());
  // visible app 2, last_activity_time less than visible app 1.
  ASSERT_TRUE(candidates[3].app());
  EXPECT_EQ("visible2", candidates[3].app()->process_name());
  // background service.
  ASSERT_TRUE(candidates[4].app());
  EXPECT_EQ("service", candidates[4].app()->process_name());
  // protected LifecycleUnit
  EXPECT_EQ(candidates[5].lifecycle_unit(), &protected_lifecycle_unit);
  // non-focused LifecycleUnits, sorted by last focused time.
  EXPECT_EQ(candidates[6].lifecycle_unit(), &other_non_focused_lifecycle_unit);
  EXPECT_EQ(candidates[7].lifecycle_unit(), &non_focused_lifecycle_unit);
}

// Occasionally, Chrome sees both FOCUSED_TAB and FOCUSED_APP at the same time.
// Test that Chrome treats the former as a more important process.
TEST_F(TabManagerDelegateTest, CandidatesSortedWithFocusedAppAndTab) {
  std::vector<arc::ArcProcess> arc_processes;
  arc_processes.emplace_back(1, 10, "focused", arc::mojom::ProcessState::TOP,
                             kIsFocused, 100);

  TestLifecycleUnit focused_lifecycle_unit(base::TimeTicks::Max());
  LifecycleUnitVector lifecycle_units{&focused_lifecycle_unit};

  const std::vector<TabManagerDelegate::Candidate> candidates =
      TabManagerDelegate::GetSortedCandidates(lifecycle_units, arc_processes);
  ASSERT_EQ(2U, candidates.size());
  // FOCUSED_TAB should be the first one.
  EXPECT_EQ(&focused_lifecycle_unit, candidates[0].lifecycle_unit());
  ASSERT_TRUE(candidates[1].app());
  EXPECT_EQ("focused", candidates[1].app()->process_name());
}

class MockTabManagerDelegate : public TabManagerDelegate {
 public:
  MockTabManagerDelegate()
      : TabManagerDelegate(nullptr),
        always_return_true_from_is_recently_killed_(false) {}

  explicit MockTabManagerDelegate(TabManagerDelegate::MemoryStat* mem_stat)
      : TabManagerDelegate(nullptr, mem_stat),
        always_return_true_from_is_recently_killed_(false) {}

  // unit test.
  std::vector<int> GetKilledArcProcesses() { return killed_arc_processes_; }

  // unit test.
  LifecycleUnitVector GetKilledTabs() { return killed_tabs_; }

  // unit test.
  void Clear() {
    killed_arc_processes_.clear();
    killed_tabs_.clear();
  }

  // unit test.
  void set_always_return_true_from_is_recently_killed(
      bool always_return_true_from_is_recently_killed) {
    always_return_true_from_is_recently_killed_ =
        always_return_true_from_is_recently_killed;
  }

  void AddLifecycleUnit(LifecycleUnit* lifecycle_unit) {
    lifecycle_units_.push_back(lifecycle_unit);
  }

  bool IsRecentlyKilledArcProcess(const std::string& process_name,
                                  const base::TimeTicks& now) override {
    if (always_return_true_from_is_recently_killed_)
      return true;
    return TabManagerDelegate::IsRecentlyKilledArcProcess(process_name, now);
  }

 protected:
  bool KillArcProcess(const int nspid) override {
    killed_arc_processes_.push_back(nspid);
    return true;
  }

  bool KillTab(LifecycleUnit* lifecycle_unit, DiscardReason reason) override {
    killed_tabs_.push_back(lifecycle_unit);
    return true;
  }

  LifecycleUnitVector GetLifecycleUnits() override { return lifecycle_units_; }

  chromeos::DebugDaemonClient* GetDebugDaemonClient() override {
    return &debugd_client_;
  }

 private:
  LifecycleUnitVector lifecycle_units_;
  chromeos::FakeDebugDaemonClient debugd_client_;
  std::vector<int> killed_arc_processes_;
  LifecycleUnitVector killed_tabs_;
  bool always_return_true_from_is_recently_killed_;
};

class MockMemoryStat : public TabManagerDelegate::MemoryStat {
 public:
  MockMemoryStat() {}
  ~MockMemoryStat() override {}

  int TargetMemoryToFreeKB() override { return target_memory_to_free_kb_; }

  int EstimatedMemoryFreedKB(base::ProcessHandle pid) override {
    return process_pss_[pid];
  }

  // unittest.
  void SetTargetMemoryToFreeKB(const int target) {
    target_memory_to_free_kb_ = target;
  }

  // unittest.
  void SetProcessPss(base::ProcessHandle pid, int pss) {
    process_pss_[pid] = pss;
  }

 private:
  int target_memory_to_free_kb_;
  std::map<base::ProcessHandle, int> process_pss_;
};

TEST_F(TabManagerDelegateTest, SetOomScoreAdj) {
  MockTabManagerDelegate tab_manager_delegate;

  std::vector<arc::ArcProcess> arc_processes;
  arc_processes.emplace_back(1, 10, "focused", arc::mojom::ProcessState::TOP,
                             kIsFocused, 100);
  arc_processes.emplace_back(2, 20, "visible1", arc::mojom::ProcessState::TOP,
                             kNotFocused, 200);
  arc_processes.emplace_back(
      3, 30, "service", arc::mojom::ProcessState::SERVICE, kNotFocused, 500);
  arc_processes.emplace_back(4, 40, "visible2", arc::mojom::ProcessState::TOP,
                             kNotFocused, 150);
  arc_processes.emplace_back(5, 50, "persistent",
                             arc::mojom::ProcessState::PERSISTENT, kNotFocused,
                             600);
  arc_processes.emplace_back(6, 60, "persistent_ui",
                             arc::mojom::ProcessState::PERSISTENT_UI,
                             kNotFocused, 700);

  TestLifecycleUnit tab1(base::TimeTicks() + base::TimeDelta::FromSeconds(3),
                         11);
  tab_manager_delegate.AddLifecycleUnit(&tab1);
  TestLifecycleUnit tab2(base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                         11);
  tab_manager_delegate.AddLifecycleUnit(&tab2);
  TestLifecycleUnit tab3(base::TimeTicks() + base::TimeDelta::FromSeconds(5),
                         12);
  tab_manager_delegate.AddLifecycleUnit(&tab3);
  TestLifecycleUnit tab4(base::TimeTicks() + base::TimeDelta::FromSeconds(4),
                         12);
  tab_manager_delegate.AddLifecycleUnit(&tab4);
  TestLifecycleUnit tab5(base::TimeTicks() + base::TimeDelta::FromSeconds(2),
                         12);
  tab_manager_delegate.AddLifecycleUnit(&tab5);

  // Sorted order (by GetSortedCandidates):
  // app "focused"       pid: 10
  // app "persistent"    pid: 50
  // app "persistent_ui" pid: 60
  // app "visible1"      pid: 20
  // app "visible2"      pid: 40
  // app "service"       pid: 30
  // tab3                pid: 12
  // tab4                pid: 12
  // tab1                pid: 11
  // tab5                pid: 12
  // tab2                pid: 11
  tab_manager_delegate.AdjustOomPrioritiesImpl(std::move(arc_processes));
  auto& oom_score_map = tab_manager_delegate.oom_score_map_;

  // 6 PIDs for apps + 2 PIDs for tabs.
  EXPECT_EQ(6U + 2U, oom_score_map.size());

  // Non-killable part. AdjustOomPrioritiesImpl() does make a focused app/tab
  // kernel-killable, but does not do that for PERSISTENT and PERSISTENT_UI
  // apps.
  EXPECT_EQ(TabManagerDelegate::kLowestOomScore, oom_score_map[50]);
  EXPECT_EQ(TabManagerDelegate::kLowestOomScore, oom_score_map[60]);

  // Higher priority part.
  EXPECT_EQ(300, oom_score_map[10]);
  EXPECT_EQ(388, oom_score_map[20]);
  EXPECT_EQ(475, oom_score_map[40]);
  EXPECT_EQ(563, oom_score_map[30]);

  // Lower priority part.
  EXPECT_EQ(650, oom_score_map[12]);
  EXPECT_EQ(720, oom_score_map[11]);
}

TEST_F(TabManagerDelegateTest, IsRecentlyKilledArcProcess) {
  constexpr char kProcessName1[] = "org.chromium.arc.test1";
  constexpr char kProcessName2[] = "org.chromium.arc.test2";

  // Not owned.
  MockMemoryStat* memory_stat = new MockMemoryStat();
  // Instantiate the mock instance.
  MockTabManagerDelegate tab_manager_delegate(memory_stat);

  // When the process name is not in the map, IsRecentlyKilledArcProcess should
  // return false.
  const base::TimeTicks now = NowTicks();
  EXPECT_FALSE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName1, now));
  EXPECT_FALSE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName2, now));

  // Update the map to tell the manager that the process was killed very
  // recently.
  tab_manager_delegate.recently_killed_arc_processes_[kProcessName1] = now;
  EXPECT_TRUE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName1, now));
  EXPECT_FALSE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName2, now));
  tab_manager_delegate.recently_killed_arc_processes_[kProcessName1] =
      now - base::TimeDelta::FromMicroseconds(1);
  EXPECT_TRUE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName1, now));
  EXPECT_FALSE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName2, now));
  tab_manager_delegate.recently_killed_arc_processes_[kProcessName1] =
      now - TabManagerDelegate::GetArcRespawnKillDelay();
  EXPECT_TRUE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName1, now));
  EXPECT_FALSE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName2, now));

  // Update the map to tell the manager that the process was killed
  // (GetArcRespawnKillDelay() + 1) seconds ago. In this case,
  // IsRecentlyKilledArcProcess(kProcessName1) should return false.
  tab_manager_delegate.recently_killed_arc_processes_[kProcessName1] =
      now - TabManagerDelegate::GetArcRespawnKillDelay() -
      base::TimeDelta::FromSeconds(1);
  EXPECT_FALSE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName1, now));
  EXPECT_FALSE(
      tab_manager_delegate.IsRecentlyKilledArcProcess(kProcessName2, now));
}

TEST_F(TabManagerDelegateTest, DoNotKillRecentlyKilledArcProcesses) {
  // Not owned.
  MockMemoryStat* memory_stat = new MockMemoryStat();

  // Instantiate the mock instance.
  MockTabManagerDelegate tab_manager_delegate(memory_stat);
  tab_manager_delegate.set_always_return_true_from_is_recently_killed(true);

  std::vector<arc::ArcProcess> arc_processes;
  arc_processes.emplace_back(
      1, 10, "service", arc::mojom::ProcessState::SERVICE, kNotFocused, 500);

  memory_stat->SetTargetMemoryToFreeKB(250000);
  memory_stat->SetProcessPss(30, 10000);
  tab_manager_delegate.LowMemoryKillImpl(DiscardReason::kUrgent,
                                         std::move(arc_processes));

  auto killed_arc_processes = tab_manager_delegate.GetKilledArcProcesses();
  EXPECT_EQ(0U, killed_arc_processes.size());
}

TEST_F(TabManagerDelegateTest, KillMultipleProcesses) {
  // Not owned.
  MockMemoryStat* memory_stat = new MockMemoryStat();

  // Instantiate the mock instance.
  MockTabManagerDelegate tab_manager_delegate(memory_stat);

  std::vector<arc::ArcProcess> arc_processes;
  arc_processes.emplace_back(1, 10, "focused", arc::mojom::ProcessState::TOP,
                             kIsFocused, 100);
  arc_processes.emplace_back(2, 20, "visible1", arc::mojom::ProcessState::TOP,
                             kNotFocused, 200);
  arc_processes.emplace_back(
      3, 30, "service", arc::mojom::ProcessState::SERVICE, kNotFocused, 500);
  arc_processes.emplace_back(4, 40, "visible2",
                             arc::mojom::ProcessState::IMPORTANT_FOREGROUND,
                             kNotFocused, 150);
  arc_processes.emplace_back(5, 50, "not-visible",
                             arc::mojom::ProcessState::IMPORTANT_BACKGROUND,
                             kNotFocused, 300);
  arc_processes.emplace_back(6, 60, "persistent",
                             arc::mojom::ProcessState::PERSISTENT, kNotFocused,
                             400);

  TestLifecycleUnit tab1(base::TimeTicks() + base::TimeDelta::FromSeconds(3),
                         11);
  tab_manager_delegate.AddLifecycleUnit(&tab1);
  TestLifecycleUnit tab2(base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                         11);
  tab_manager_delegate.AddLifecycleUnit(&tab2);
  TestLifecycleUnit tab3(base::TimeTicks() + base::TimeDelta::FromSeconds(5),
                         12);
  tab_manager_delegate.AddLifecycleUnit(&tab3);
  TestLifecycleUnit tab4(base::TimeTicks() + base::TimeDelta::FromSeconds(4),
                         12);
  tab_manager_delegate.AddLifecycleUnit(&tab4);
  TestLifecycleUnit tab5(base::TimeTicks() + base::TimeDelta::FromSeconds(2),
                         12);
  tab_manager_delegate.AddLifecycleUnit(&tab5);

  // Sorted order (by GetSortedCandidates):
  // app "focused"     pid: 10  nspid 1
  // app "persistent"  pid: 60  nspid 6
  // app "visible1"    pid: 20  nspid 2
  // app "visible2"    pid: 40  nspid 4
  // app "not-visible" pid: 50  nspid 5
  // app "service"     pid: 30  nspid 3
  // tab3              pid: 12  id 3
  // tab4              pid: 12  id 4
  // tab1              pid: 11  id 1
  // tab5              pid: 12  id 5
  // tab2              pid: 11  id 2
  memory_stat->SetTargetMemoryToFreeKB(250000);
  // Entities to be killed.
  memory_stat->SetProcessPss(11, 50000);
  memory_stat->SetProcessPss(12, 30000);
  memory_stat->SetProcessPss(30, 10000);
  memory_stat->SetProcessPss(50, 60000);
  // Should not be used.
  memory_stat->SetProcessPss(60, 500000);
  memory_stat->SetProcessPss(40, 50000);
  memory_stat->SetProcessPss(20, 30000);
  memory_stat->SetProcessPss(10, 100000);

  tab_manager_delegate.LowMemoryKillImpl(DiscardReason::kProactive,
                                         std::move(arc_processes));

  auto killed_arc_processes = tab_manager_delegate.GetKilledArcProcesses();
  auto killed_tabs = tab_manager_delegate.GetKilledTabs();

  // Killed apps and their nspid.
  ASSERT_EQ(2U, killed_arc_processes.size());
  EXPECT_EQ(3, killed_arc_processes[0]);
  EXPECT_EQ(5, killed_arc_processes[1]);
  // Killed tabs and their content id.
  // Note that process with pid 11 is counted twice and pid 12 is counted 3
  // times. But so far I don't have a good way to estimate the memory freed
  // if multiple tabs share one process.
  ASSERT_EQ(5U, killed_tabs.size());
  EXPECT_EQ(&tab2, killed_tabs[0]);
  EXPECT_EQ(&tab5, killed_tabs[1]);
  EXPECT_EQ(&tab1, killed_tabs[2]);
  EXPECT_EQ(&tab4, killed_tabs[3]);
  EXPECT_EQ(&tab3, killed_tabs[4]);

  // Check that killed apps are in the map.
  const TabManagerDelegate::KilledArcProcessesMap& processes_map =
      tab_manager_delegate.recently_killed_arc_processes_;
  EXPECT_EQ(2U, processes_map.size());
  EXPECT_EQ(1U, processes_map.count("service"));
  EXPECT_EQ(1U, processes_map.count("not-visible"));
}

}  // namespace resource_coordinator
