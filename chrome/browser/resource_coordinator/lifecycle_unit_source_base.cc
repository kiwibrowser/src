// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/lifecycle_unit_source_base.h"

#include "chrome/browser/resource_coordinator/lifecycle_unit_source_observer.h"

namespace resource_coordinator {

LifecycleUnitSourceBase::LifecycleUnitSourceBase() = default;
LifecycleUnitSourceBase::~LifecycleUnitSourceBase() = default;

void LifecycleUnitSourceBase::AddObserver(
    LifecycleUnitSourceObserver* observer) {
  observers_.AddObserver(observer);
}

void LifecycleUnitSourceBase::RemoveObserver(
    LifecycleUnitSourceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void LifecycleUnitSourceBase::NotifyLifecycleUnitCreated(
    LifecycleUnit* lifecycle_unit) {
  for (LifecycleUnitSourceObserver& observer : observers_)
    observer.OnLifecycleUnitCreated(lifecycle_unit);
}

}  // namespace resource_coordinator
