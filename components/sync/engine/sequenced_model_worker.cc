// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/sequenced_model_worker.h"

#include <utility>

namespace syncer {

SequencedModelWorker::SequencedModelWorker(
    const scoped_refptr<base::SequencedTaskRunner>& runner,
    ModelSafeGroup group)
    : runner_(runner), group_(group) {}

void SequencedModelWorker::ScheduleWork(base::OnceClosure work) {
  if (runner_->RunsTasksInCurrentSequence()) {
    DLOG(WARNING) << "Already on sequenced task runner " << runner_;
    std::move(work).Run();
  } else {
    runner_->PostTask(FROM_HERE, std::move(work));
  }
}

ModelSafeGroup SequencedModelWorker::GetModelSafeGroup() {
  return group_;
}

bool SequencedModelWorker::IsOnModelSequence() {
  return runner_->RunsTasksInCurrentSequence();
}

SequencedModelWorker::~SequencedModelWorker() {}

}  // namespace syncer
