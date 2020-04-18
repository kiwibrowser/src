// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_METRICS_H_
#define CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_METRICS_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "content/browser/background_sync/background_sync.pb.h"
#include "content/browser/background_sync/background_sync_status.h"

namespace content {

// This class contains the logic for recording usage metrics for the Background
// Sync API. It is stateless, containing only static methods, so it can be used
// by any of the Background Sync code, without needing to be instantiated
// explicitly.
class BackgroundSyncMetrics {
 public:
  enum RegistrationCouldFire {
    REGISTRATION_COULD_NOT_FIRE,
    REGISTRATION_COULD_FIRE
  };

  enum RegistrationIsDuplicate {
    REGISTRATION_IS_NOT_DUPLICATE,
    REGISTRATION_IS_DUPLICATE
  };

  // Records the start of a sync event.
  static void RecordEventStarted(bool startedin_foreground);

  // Records the result of a single sync event firing.
  static void RecordEventResult(bool result, bool finished_in_foreground);

  // Records the result of running a batch of sync events, including the total
  // time spent, and the batch size.
  static void RecordBatchSyncEventComplete(const base::TimeDelta& time,
                                           int number_of_batched_sync_events);

  // Records the result of successfully registering a sync. |could_fire|
  // indicates whether the conditions were sufficient for the sync to fire
  // immediately at the time it was registered.
  static void CountRegisterSuccess(
      RegistrationCouldFire could_fire,
      RegistrationIsDuplicate registration_is_duplicate);

  // Records the status of a failed sync registration.
  static void CountRegisterFailure(BackgroundSyncStatus status);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BackgroundSyncMetrics);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_METRICS_H_
