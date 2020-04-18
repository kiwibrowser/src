// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_USER_EVENTS_TRIAL_RECORDER_H_
#define COMPONENTS_SYNC_USER_EVENTS_TRIAL_RECORDER_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "components/sync/protocol/user_event_specifics.pb.h"
#include "components/sync/user_events/user_event_service.h"
#include "components/variations/variations_associated_data.h"
#include "components/variations/variations_id_collection.h"

namespace syncer {

// Watches finalization of trails and records FieldTrial events through its
// UserEventService on construction, on change, and every so often.
class TrialRecorder {
 public:
  explicit TrialRecorder(UserEventService* user_event_service);
  ~TrialRecorder();

 private:
  // Construct and record a field trial event if applicable.
  void RecordFieldTrials();

  // Simply drops the |id| param and calls RecordFieldTrials().
  void OnNewVariationId(variations::VariationID id);

  // Non-owning pointer to interface of how events are actually recorded.
  UserEventService* user_event_service_;

  // Tracks all the variation ids that we we care about.
  variations::VariationsIdCollection variations_;

  // Timer used to record a field trial event every given interval.
  base::OneShotTimer field_trial_timer_;

  DISALLOW_COPY_AND_ASSIGN(TrialRecorder);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_USER_EVENTS_TRIAL_RECORDER_H_
