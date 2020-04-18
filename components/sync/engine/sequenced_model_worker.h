// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_SEQUENCED_MODEL_WORKER_H_
#define COMPONENTS_SYNC_ENGINE_SEQUENCED_MODEL_WORKER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "components/sync/engine/model_safe_worker.h"

namespace syncer {

// A ModelSafeWorker for models that accept requests from the
// syncapi that need to be fulfilled on a model specific sequenced task runner.
// TODO(sync): Try to generalize other ModelWorkers (e.g. history, etc).
class SequencedModelWorker : public ModelSafeWorker {
 public:
  SequencedModelWorker(const scoped_refptr<base::SequencedTaskRunner>& runner,
                       ModelSafeGroup group);

  // ModelSafeWorker implementation.
  ModelSafeGroup GetModelSafeGroup() override;
  bool IsOnModelSequence() override;

 private:
  ~SequencedModelWorker() override;

  void ScheduleWork(base::OnceClosure work) override;

  scoped_refptr<base::SequencedTaskRunner> runner_;
  ModelSafeGroup group_;

  DISALLOW_COPY_AND_ASSIGN(SequencedModelWorker);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_SEQUENCED_MODEL_WORKER_H_
