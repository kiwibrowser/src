// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/test_lifecycle_unit.h"

namespace resource_coordinator {

TestLifecycleUnit::TestLifecycleUnit(base::TimeTicks last_focused_time,
                                     base::ProcessHandle process_handle,
                                     bool can_discard)
    : LifecycleUnitBase(content::Visibility::VISIBLE),
      last_focused_time_(last_focused_time),
      process_handle_(process_handle),
      can_discard_(can_discard) {}

TestLifecycleUnit::~TestLifecycleUnit() {
  OnLifecycleUnitDestroyed();
}

TabLifecycleUnitExternal* TestLifecycleUnit::AsTabLifecycleUnitExternal() {
  return nullptr;
}

base::string16 TestLifecycleUnit::GetTitle() const {
  return base::string16();
}

base::TimeTicks TestLifecycleUnit::GetLastFocusedTime() const {
  return last_focused_time_;
}

base::ProcessHandle TestLifecycleUnit::GetProcessHandle() const {
  return process_handle_;
}

LifecycleUnit::SortKey TestLifecycleUnit::GetSortKey() const {
  return SortKey(last_focused_time_);
}

content::Visibility TestLifecycleUnit::GetVisibility() const {
  return content::Visibility::VISIBLE;
}

bool TestLifecycleUnit::Freeze() {
  return false;
}

int TestLifecycleUnit::GetEstimatedMemoryFreedOnDiscardKB() const {
  return 0;
}

bool TestLifecycleUnit::CanPurge() const {
  return false;
}

bool TestLifecycleUnit::CanFreeze() const {
  return false;
}

bool TestLifecycleUnit::CanDiscard(DiscardReason reason) const {
  return can_discard_;
}

bool TestLifecycleUnit::Discard(DiscardReason discard_reason) {
  return false;
}

}  // namespace resource_coordinator
