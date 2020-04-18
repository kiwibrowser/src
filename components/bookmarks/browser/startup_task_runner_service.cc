// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bookmarks/browser/startup_task_runner_service.h"

#include "base/deferred_sequenced_task_runner.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"

namespace bookmarks {

StartupTaskRunnerService::StartupTaskRunnerService(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner)
    : io_task_runner_(io_task_runner) {
  DCHECK(io_task_runner_);
}

StartupTaskRunnerService::~StartupTaskRunnerService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

scoped_refptr<base::DeferredSequencedTaskRunner>
    StartupTaskRunnerService::GetBookmarkTaskRunner() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!bookmark_task_runner_) {
    bookmark_task_runner_ =
        new base::DeferredSequencedTaskRunner(io_task_runner_);
  }
  return bookmark_task_runner_;
}

void StartupTaskRunnerService::StartDeferredTaskRunners() {
  GetBookmarkTaskRunner()->Start();
}

}  // namespace bookmarks
