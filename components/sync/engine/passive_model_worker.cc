// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/passive_model_worker.h"

#include <utility>

namespace syncer {

PassiveModelWorker::PassiveModelWorker() = default;

PassiveModelWorker::~PassiveModelWorker() {}

void PassiveModelWorker::ScheduleWork(base::OnceClosure work) {
  std::move(work).Run();
}

ModelSafeGroup PassiveModelWorker::GetModelSafeGroup() {
  return GROUP_PASSIVE;
}

bool PassiveModelWorker::IsOnModelSequence() {
  // Passive types are checked by SyncBackendRegistrar.
  NOTREACHED();
  return false;
}

}  // namespace syncer
