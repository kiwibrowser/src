// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/lifecycle_unit_base.h"

#include "chrome/browser/resource_coordinator/lifecycle_unit_observer.h"
#include "chrome/browser/resource_coordinator/time.h"

namespace resource_coordinator {

LifecycleUnitBase::LifecycleUnitBase(content::Visibility visibility)
    : last_visible_time_(visibility == content::Visibility::VISIBLE
                             ? base::TimeTicks::Max()
                             : base::TimeTicks()) {}

LifecycleUnitBase::~LifecycleUnitBase() = default;

int32_t LifecycleUnitBase::GetID() const {
  return id_;
}

LifecycleState LifecycleUnitBase::GetState() const {
  return state_;
}

base::TimeTicks LifecycleUnitBase::GetLastVisibleTime() const {
  return last_visible_time_;
}

void LifecycleUnitBase::AddObserver(LifecycleUnitObserver* observer) {
  observers_.AddObserver(observer);
}

void LifecycleUnitBase::RemoveObserver(LifecycleUnitObserver* observer) {
  observers_.RemoveObserver(observer);
}

void LifecycleUnitBase::SetState(LifecycleState state) {
  if (state == state_)
    return;
  LifecycleState last_state = state_;
  state_ = state;
  for (auto& observer : observers_)
    observer.OnLifecycleUnitStateChanged(this, last_state);
}

void LifecycleUnitBase::OnLifecycleUnitVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::VISIBLE)
    last_visible_time_ = base::TimeTicks::Max();
  else if (last_visible_time_.is_max())
    last_visible_time_ = NowTicks();

  for (auto& observer : observers_)
    observer.OnLifecycleUnitVisibilityChanged(this, visibility);
}

void LifecycleUnitBase::OnLifecycleUnitDestroyed() {
  for (auto& observer : observers_)
    observer.OnLifecycleUnitDestroyed(this);
}

int32_t LifecycleUnitBase::next_id_ = 0;

}  // namespace resource_coordinator
