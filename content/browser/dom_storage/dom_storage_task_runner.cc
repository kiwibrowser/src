// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/dom_storage_task_runner.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"

namespace content {

// DOMStorageWorkerPoolTaskRunner

DOMStorageWorkerPoolTaskRunner::DOMStorageWorkerPoolTaskRunner(
    scoped_refptr<base::SequencedTaskRunner> primary_sequence,
    scoped_refptr<base::SequencedTaskRunner> commit_sequence)
    : primary_sequence_(std::move(primary_sequence)),
      commit_sequence_(std::move(commit_sequence)) {}

DOMStorageWorkerPoolTaskRunner::~DOMStorageWorkerPoolTaskRunner() = default;

bool DOMStorageWorkerPoolTaskRunner::RunsTasksInCurrentSequence() const {
  return primary_sequence_->RunsTasksInCurrentSequence() ||
         commit_sequence_->RunsTasksInCurrentSequence();
}

bool DOMStorageWorkerPoolTaskRunner::PostDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  return primary_sequence_->PostDelayedTask(from_here, std::move(task), delay);
}

bool DOMStorageWorkerPoolTaskRunner::PostShutdownBlockingTask(
    const base::Location& from_here,
    SequenceID sequence_id,
    base::OnceClosure task) {
  return GetSequencedTaskRunner(sequence_id)
      ->PostTask(from_here, std::move(task));
}

void DOMStorageWorkerPoolTaskRunner::AssertIsRunningOnPrimarySequence() const {
  DCHECK(primary_sequence_->RunsTasksInCurrentSequence());
}

void DOMStorageWorkerPoolTaskRunner::AssertIsRunningOnCommitSequence() const {
  DCHECK(commit_sequence_->RunsTasksInCurrentSequence());
}

scoped_refptr<base::SequencedTaskRunner>
DOMStorageWorkerPoolTaskRunner::GetSequencedTaskRunner(SequenceID sequence_id) {
  if (sequence_id == PRIMARY_SEQUENCE)
    return primary_sequence_;
  DCHECK_EQ(COMMIT_SEQUENCE, sequence_id);
  return commit_sequence_;
}

// MockDOMStorageTaskRunner

MockDOMStorageTaskRunner::MockDOMStorageTaskRunner(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {}

MockDOMStorageTaskRunner::~MockDOMStorageTaskRunner() = default;

bool MockDOMStorageTaskRunner::RunsTasksInCurrentSequence() const {
  return task_runner_->RunsTasksInCurrentSequence();
}

bool MockDOMStorageTaskRunner::PostDelayedTask(const base::Location& from_here,
                                               base::OnceClosure task,
                                               base::TimeDelta delay) {
  return task_runner_->PostTask(from_here, std::move(task));
}

bool MockDOMStorageTaskRunner::PostShutdownBlockingTask(
    const base::Location& from_here,
    SequenceID sequence_id,
    base::OnceClosure task) {
  return task_runner_->PostTask(from_here, std::move(task));
}

void MockDOMStorageTaskRunner::AssertIsRunningOnPrimarySequence() const {
  DCHECK(RunsTasksInCurrentSequence());
}

void MockDOMStorageTaskRunner::AssertIsRunningOnCommitSequence() const {
  DCHECK(RunsTasksInCurrentSequence());
}

scoped_refptr<base::SequencedTaskRunner>
MockDOMStorageTaskRunner::GetSequencedTaskRunner(SequenceID sequence_id) {
  return task_runner_;
}

}  // namespace content
