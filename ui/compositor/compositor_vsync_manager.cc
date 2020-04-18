// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/compositor_vsync_manager.h"

namespace ui {

CompositorVSyncManager::CompositorVSyncManager()
    : authoritative_vsync_interval_(base::TimeDelta::FromSeconds(0)) {
}

CompositorVSyncManager::~CompositorVSyncManager() {}

void CompositorVSyncManager::SetAuthoritativeVSyncInterval(
    base::TimeDelta interval) {
  authoritative_vsync_interval_ = interval;
  last_interval_ = interval;
  NotifyObservers(last_timebase_, last_interval_);
}

void CompositorVSyncManager::UpdateVSyncParameters(base::TimeTicks timebase,
                                                   base::TimeDelta interval) {
  if (authoritative_vsync_interval_ != base::TimeDelta::FromSeconds(0))
    interval = authoritative_vsync_interval_;
  last_timebase_ = timebase;
  last_interval_ = interval;
  NotifyObservers(timebase, interval);
}

void CompositorVSyncManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
  observer->OnUpdateVSyncParameters(last_timebase_, last_interval_);
}

void CompositorVSyncManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void CompositorVSyncManager::NotifyObservers(base::TimeTicks timebase,
                                             base::TimeDelta interval) {
  for (auto& observer : observer_list_)
    observer.OnUpdateVSyncParameters(timebase, interval);
}

}  // namespace ui
