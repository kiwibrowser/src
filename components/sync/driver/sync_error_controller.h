// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_ERROR_CONTROLLER_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_ERROR_CONTROLLER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/sync/driver/sync_service_observer.h"

namespace syncer {

class SyncService;

// Keep track of sync errors and expose them to observers in the UI.
class SyncErrorController : public SyncServiceObserver {
 public:
  // The observer class for SyncErrorController lets the controller notify
  // observers when an error arises or changes.
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void OnErrorChanged() = 0;
  };

  explicit SyncErrorController(SyncService* service);
  ~SyncErrorController() override;

  // True if there exists an error worth elevating to the user.
  bool HasError();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // SyncServiceObserver:
  void OnStateChanged(syncer::SyncService* sync) override;

 private:
  SyncService* service_;
  base::ObserverList<Observer, true> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(SyncErrorController);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_ERROR_CONTROLLER_H_
