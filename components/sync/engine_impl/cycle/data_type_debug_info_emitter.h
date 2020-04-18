// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_DATA_TYPE_DEBUG_INFO_EMITTER_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_DATA_TYPE_DEBUG_INFO_EMITTER_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/sync/base/model_type.h"
#include "components/sync/engine/cycle/commit_counters.h"
#include "components/sync/engine/cycle/update_counters.h"

namespace syncer {

class TypeDebugInfoObserver;

// Supports various kinds of debugging requests for a certain directory type.
//
// The Emit*() functions send updates to registered TypeDebugInfoObservers.
// The DataTypeDebugInfoEmitter does not directly own that list; it is
// managed by the ModelTypeRegistry.
//
// For Update and Commit counters, the job of keeping the counters up to date
// is delegated to the UpdateHandler and CommitContributors. For the Stats
// counters, the emitter will let sub class to fetch all the required
// information on demand.
class DataTypeDebugInfoEmitter {
 public:
  // The |observers| is not owned.  |observers| may be modified outside of this
  // object and is expected to outlive this object.
  DataTypeDebugInfoEmitter(
      ModelType type,
      base::ObserverList<TypeDebugInfoObserver>* observers);

  virtual ~DataTypeDebugInfoEmitter();

  // Returns a reference to the current commit counters.
  const CommitCounters& GetCommitCounters() const;

  // Allows others to mutate the commit counters.
  CommitCounters* GetMutableCommitCounters();

  // Triggerss a commit counters update to registered observers.
  void EmitCommitCountersUpdate();

  // Returns a reference to the current update counters.
  const UpdateCounters& GetUpdateCounters() const;

  // Allows others to mutate the update counters.
  UpdateCounters* GetMutableUpdateCounters();

  // Triggers an update counters update to registered observers.
  void EmitUpdateCountersUpdate();

  // Triggers a status counters update to registered observers.
  virtual void EmitStatusCountersUpdate() = 0;

 protected:
  const ModelType type_;

  // Because there are so many emitters that come into and out of existence, it
  // doesn't make sense to have them manage their own observer list.  They all
  // share one observer list that is provided by their owner and which is
  // guaranteed to outlive them.
  base::ObserverList<TypeDebugInfoObserver>* type_debug_info_observers_;

 private:
  CommitCounters commit_counters_;
  UpdateCounters update_counters_;

  DISALLOW_COPY_AND_ASSIGN(DataTypeDebugInfoEmitter);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_DATA_TYPE_DEBUG_INFO_EMITTER_H_
