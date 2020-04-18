// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/trial_recorder.h"

#include <memory>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/stl_util.h"
#include "base/time/time.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/variations/active_field_trials.h"

using sync_pb::UserEventSpecifics;

namespace syncer {

namespace {

// A FieldTrial is recorded periodically, using this delay. Although upon chance
// an event is immediately recorded and we reset to using this delay for the
// next time.
base::TimeDelta GetFieldTrialDelay() {
  return base::TimeDelta::FromSeconds(base::GetFieldTrialParamByFeatureAsInt(
      switches::kSyncUserFieldTrialEvents, "field_trial_delay_seconds",
      base::TimeDelta::FromDays(1).InSeconds()));
}

}  // namespace

TrialRecorder::TrialRecorder(UserEventService* user_event_service)
    : user_event_service_(user_event_service),
      variations_(variations::CHROME_SYNC_EVENT_LOGGER,
                  base::BindRepeating(&TrialRecorder::OnNewVariationId,
                                      base::Unretained(this))) {
  DCHECK(user_event_service_);
  RecordFieldTrials();
}

TrialRecorder::~TrialRecorder() {}

void TrialRecorder::RecordFieldTrials() {
  if (!base::FeatureList::IsEnabled(switches::kSyncUserFieldTrialEvents)) {
    return;
  }

  std::set<variations::VariationID> ids = variations_.GetIds();
  if (!ids.empty()) {
    auto specifics = std::make_unique<UserEventSpecifics>();
    specifics->set_event_time_usec(
        (base::Time::Now() - base::Time()).InMicroseconds());

    for (variations::VariationID id : ids) {
      DCHECK_NE(variations::EMPTY_ID, id);
      specifics->mutable_field_trial_event()->add_variation_ids(id);
    }
    user_event_service_->RecordUserEvent(std::move(specifics));
  }

  field_trial_timer_.Start(
      FROM_HERE, GetFieldTrialDelay(),
      base::BindRepeating(&TrialRecorder::RecordFieldTrials,
                          base::Unretained(this)));
}

void TrialRecorder::OnNewVariationId(variations::VariationID id) {
  RecordFieldTrials();
}

}  // namespace syncer
