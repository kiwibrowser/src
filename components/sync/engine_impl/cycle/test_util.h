// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_TEST_UTIL_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_TEST_UTIL_H_

#include "components/sync/engine_impl/cycle/sync_cycle.h"
#include "components/sync/engine_impl/syncer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace test_util {

// Utils to simulate various outcomes of a sync cycle.

// Configure sync cycle successes and failures.
void SimulateGetEncryptionKeyFailed(ModelTypeSet requested_types,
                                    sync_pb::SyncEnums::GetUpdatesOrigin origin,
                                    SyncCycle* cycle);
void SimulateConfigureSuccess(ModelTypeSet requested_types,
                              sync_pb::SyncEnums::GetUpdatesOrigin origin,
                              SyncCycle* cycle);
void SimulateConfigureFailed(ModelTypeSet requested_types,
                             sync_pb::SyncEnums::GetUpdatesOrigin origin,
                             SyncCycle* cycle);
void SimulateConfigureConnectionFailure(
    ModelTypeSet requested_types,
    sync_pb::SyncEnums::GetUpdatesOrigin origin,
    SyncCycle* cycle);

// Normal mode sync cycle successes and failures.
void SimulateNormalSuccess(ModelTypeSet requested_types,
                           NudgeTracker* nudge_tracker,
                           SyncCycle* cycle);
void SimulateDownloadUpdatesFailed(ModelTypeSet requested_types,
                                   NudgeTracker* nudge_tracker,
                                   SyncCycle* cycle);
void SimulateCommitFailed(ModelTypeSet requested_types,
                          NudgeTracker* nudge_tracker,
                          SyncCycle* cycle);
void SimulateConnectionFailure(ModelTypeSet requested_types,
                               NudgeTracker* nudge_tracker,
                               SyncCycle* cycle);

// Poll successes and failures.
// TODO(tschumann): Move poll simulations into the only call site,
// sync_scheduler_impl_unittest.cc.
void SimulatePollSuccess(ModelTypeSet requested_types, SyncCycle* cycle);
void SimulatePollFailed(ModelTypeSet requested_types, SyncCycle* cycle);

void SimulateGuRetryDelayCommandImpl(SyncCycle* cycle, base::TimeDelta delay);

void SimulateThrottledImpl(SyncCycle* cycle, const base::TimeDelta& delta);

void SimulateTypesThrottledImpl(SyncCycle* cycle,
                                ModelTypeSet types,
                                const base::TimeDelta& delta);

void SimulatePartialFailureImpl(SyncCycle* cycle, ModelTypeSet types);

// Works with poll cycles.
void SimulatePollIntervalUpdateImpl(ModelTypeSet requested_types,
                                    SyncCycle* cycle,
                                    const base::TimeDelta& new_poll);

// TODO(tschumann): Most of these actions are only used by
// sync_scheduler_impl_unittest.cc. Move them there to avoid unneccesary
// redirection and keep the context in one place.
ACTION_P(SimulateThrottled, throttle) {
  SimulateThrottledImpl(arg0, throttle);
}

ACTION_P2(SimulateTypesThrottled, types, throttle) {
  SimulateTypesThrottledImpl(arg0, types, throttle);
}

ACTION_P(SimulatePartialFailure, types) {
  SimulatePartialFailureImpl(arg0, types);
}

ACTION_P(SimulatePollIntervalUpdate, poll) {
  SimulatePollIntervalUpdateImpl(arg0, arg1, poll);
}

ACTION_P(SimulateGuRetryDelayCommand, delay) {
  SimulateGuRetryDelayCommandImpl(arg0, delay);
}

}  // namespace test_util
}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_TEST_UTIL_H_
