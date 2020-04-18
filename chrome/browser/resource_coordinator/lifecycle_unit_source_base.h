// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SOURCE_BASE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SOURCE_BASE_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_source.h"

namespace resource_coordinator {

class LifecycleUnit;

// Base class for a class that creates and destroys LifecycleUnits.
class LifecycleUnitSourceBase : public LifecycleUnitSource {
 public:
  LifecycleUnitSourceBase();
  ~LifecycleUnitSourceBase() override;

  // LifecycleUnitSource:
  void AddObserver(LifecycleUnitSourceObserver* observer) override;
  void RemoveObserver(LifecycleUnitSourceObserver* observer) override;

 protected:
  // Notifies observers that a LifecycleUnit was created.
  void NotifyLifecycleUnitCreated(LifecycleUnit* lifecycle_unit);

 private:
  // Observers notified when a LifecycleUnit is created.
  base::ObserverList<LifecycleUnitSourceObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(LifecycleUnitSourceBase);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_UNIT_SOURCE_BASE_H_
