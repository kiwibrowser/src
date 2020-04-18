// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TEST_LIFECYCLE_UNIT_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TEST_LIFECYCLE_UNIT_H_

#include "base/macros.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_base.h"

namespace resource_coordinator {

class TestLifecycleUnit : public LifecycleUnitBase {
 public:
  using LifecycleUnitBase::OnLifecycleUnitVisibilityChanged;
  using LifecycleUnitBase::SetState;

  TestLifecycleUnit(base::TimeTicks last_focused_time = base::TimeTicks(),
                    base::ProcessHandle process_handle = base::ProcessHandle(),
                    bool can_discard = true);
  ~TestLifecycleUnit() override;

  void SetLastFocusedTime(base::TimeTicks last_focused_time) {
    last_focused_time_ = last_focused_time;
  }

  // LifecycleUnit:
  TabLifecycleUnitExternal* AsTabLifecycleUnitExternal() override;
  base::string16 GetTitle() const override;
  base::TimeTicks GetLastFocusedTime() const override;
  base::ProcessHandle GetProcessHandle() const override;
  SortKey GetSortKey() const override;
  content::Visibility GetVisibility() const override;
  bool Freeze() override;
  int GetEstimatedMemoryFreedOnDiscardKB() const override;
  bool CanPurge() const override;
  bool CanFreeze() const override;
  bool CanDiscard(DiscardReason reason) const override;
  bool Discard(DiscardReason discard_reason) override;

 private:
  base::TimeTicks last_focused_time_;
  base::ProcessHandle process_handle_;
  bool can_discard_;

  DISALLOW_COPY_AND_ASSIGN(TestLifecycleUnit);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TEST_LIFECYCLE_UNIT_H_
