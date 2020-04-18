// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MODEL_MODEL_TYPE_CONTROLLER_DELEGATE_H_
#define COMPONENTS_SYNC_MODEL_MODEL_TYPE_CONTROLLER_DELEGATE_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/sync/base/model_type.h"
#include "components/sync/engine/activation_context.h"
#include "components/sync/engine/cycle/status_counters.h"
#include "components/sync/model/model_error.h"

namespace syncer {

// The ModelTypeControllerDelegate handles communication of ModelTypeController
// with the data type. Unlike the controller which lives on the UI thread, the
// delegate can assume all its functions are run on the model thread.
class ModelTypeControllerDelegate {
 public:
  using AllNodesCallback =
      base::OnceCallback<void(ModelType, std::unique_ptr<base::ListValue>)>;
  using StartCallback =
      base::OnceCallback<void(std::unique_ptr<ActivationContext>)>;
  using StatusCountersCallback =
      base::OnceCallback<void(ModelType, const StatusCounters&)>;

  virtual ~ModelTypeControllerDelegate() = default;

  // Gathers additional information needed before the processor can be
  // connected to a sync worker. Once the metadata has been loaded, the info
  // is collected and given to |callback|.
  virtual void OnSyncStarting(const ModelErrorHandler& error_handler,
                              StartCallback callback) = 0;

  // Indicates that we no longer want to do any sync-related things for this
  // data type. Severs all ties to the sync thread, deletes all local sync
  // metadata, and then destroys the change processor.
  virtual void DisableSync() = 0;

  // Returns a ListValue representing all nodes for the type to |callback|.
  // Used for populating nodes in Sync Node Browser of chrome://sync-internals.
  virtual void GetAllNodesForDebugging(AllNodesCallback callback) = 0;

  // Returns StatusCounters for the type to |callback|.
  // Used for updating data type counters in chrome://sync-internals.
  virtual void GetStatusCountersForDebugging(
      StatusCountersCallback callback) = 0;

  // Estimates memory usage and records it in a histogram.
  virtual void RecordMemoryUsageHistogram() = 0;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_MODEL_MODEL_TYPE_CONTROLLER_DELEGATE_H_
