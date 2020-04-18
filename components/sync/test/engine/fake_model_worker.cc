// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/test/engine/fake_model_worker.h"

#include <utility>

namespace syncer {

FakeModelWorker::FakeModelWorker(ModelSafeGroup group) : group_(group) {}

FakeModelWorker::~FakeModelWorker() {
  // We may need to relax this is FakeModelWorker is used in a
  // multi-threaded test; since ModelSafeWorkers are
  // RefCountedThreadSafe, they could theoretically be destroyed from
  // a different thread.
  DCHECK(thread_checker_.CalledOnValidThread());
}

void FakeModelWorker::ScheduleWork(base::OnceClosure work) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Simply do the work on the current thread.
  std::move(work).Run();
}

ModelSafeGroup FakeModelWorker::GetModelSafeGroup() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return group_;
}

bool FakeModelWorker::IsOnModelSequence() {
  return thread_checker_.CalledOnValidThread();
}

}  // namespace syncer
