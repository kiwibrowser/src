// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_PASSIVE_MODEL_WORKER_H_
#define COMPONENTS_SYNC_ENGINE_PASSIVE_MODEL_WORKER_H_

#include "base/macros.h"
#include "components/sync/engine/model_safe_worker.h"

namespace syncer {

// Implementation of ModelSafeWorker for passive types.  All work is
// done on the same sequence DoWorkAndWaitUntilDone is called on (i.e. the same
// sequence Sync runs on).
class PassiveModelWorker : public ModelSafeWorker {
 public:
  PassiveModelWorker();

  // ModelSafeWorker implementation.
  ModelSafeGroup GetModelSafeGroup() override;
  bool IsOnModelSequence() override;

 private:
  ~PassiveModelWorker() override;

  void ScheduleWork(base::OnceClosure work) override;

  DISALLOW_COPY_AND_ASSIGN(PassiveModelWorker);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_PASSIVE_MODEL_WORKER_H_
