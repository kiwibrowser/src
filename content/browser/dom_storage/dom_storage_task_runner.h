// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_TASK_RUNNER_H_
#define CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_TASK_RUNNER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "content/common/content_export.h"

namespace content {

// DOMStorage uses two task sequences (primary vs commit) to avoid
// primary access from queuing up behind commits to disk.
// * Initialization, shutdown, and administrative tasks are performed as
//   shutdown-blocking primary sequence tasks.
// * Tasks directly related to the javascript'able interface are performed
//   as shutdown-blocking primary sequence tasks.
//   TODO(michaeln): Skip tasks for reading during shutdown.
// * Internal tasks related to committing changes to disk are performed as
//   shutdown-blocking commit sequence tasks.
class CONTENT_EXPORT DOMStorageTaskRunner
    : public base::TaskRunner {
 public:
  enum SequenceID {
    PRIMARY_SEQUENCE,
    COMMIT_SEQUENCE
  };

  // The PostTask() and PostDelayedTask() methods defined by TaskRunner
  // post shutdown-blocking tasks on the primary sequence.
  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override = 0;

  // Posts a shutdown blocking task to |sequence_id|.
  virtual bool PostShutdownBlockingTask(const base::Location& from_here,
                                        SequenceID sequence_id,
                                        base::OnceClosure task) = 0;

  virtual void AssertIsRunningOnPrimarySequence() const = 0;
  virtual void AssertIsRunningOnCommitSequence() const = 0;

  virtual scoped_refptr<base::SequencedTaskRunner> GetSequencedTaskRunner(
      SequenceID sequence_id) = 0;

 protected:
  ~DOMStorageTaskRunner() override {}
};

// A DOMStorageTaskRunner which manages a primary and a commit sequence.
class CONTENT_EXPORT DOMStorageWorkerPoolTaskRunner :
      public DOMStorageTaskRunner {
 public:
  // |primary_sequence| and |commit_sequence| should have
  // TaskShutdownBehaviour::BLOCK_SHUTDOWN semantics.
  DOMStorageWorkerPoolTaskRunner(
      scoped_refptr<base::SequencedTaskRunner> primary_sequence,
      scoped_refptr<base::SequencedTaskRunner> commit_sequence);

  bool RunsTasksInCurrentSequence() const override;

  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;

  bool PostShutdownBlockingTask(const base::Location& from_here,
                                SequenceID sequence_id,
                                base::OnceClosure task) override;

  void AssertIsRunningOnPrimarySequence() const override;
  void AssertIsRunningOnCommitSequence() const override;

  scoped_refptr<base::SequencedTaskRunner> GetSequencedTaskRunner(
      SequenceID sequence_id) override;

 protected:
  ~DOMStorageWorkerPoolTaskRunner() override;

 private:
  scoped_refptr<base::SequencedTaskRunner> primary_sequence_;
  scoped_refptr<base::SequencedTaskRunner> commit_sequence_;

  DISALLOW_COPY_AND_ASSIGN(DOMStorageWorkerPoolTaskRunner);
};

// A derived class used in unit tests that ignores all delays so
// we don't block in unit tests waiting for timeouts to expire.
// There is no distinction between [non]-shutdown-blocking or
// the primary sequence vs the commit sequence in the mock,
// all tasks are scheduled on |task_runner| with zero delay.
class CONTENT_EXPORT MockDOMStorageTaskRunner :
      public DOMStorageTaskRunner {
 public:
  explicit MockDOMStorageTaskRunner(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  bool RunsTasksInCurrentSequence() const override;

  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;

  bool PostShutdownBlockingTask(const base::Location& from_here,
                                SequenceID sequence_id,
                                base::OnceClosure task) override;

  void AssertIsRunningOnPrimarySequence() const override;
  void AssertIsRunningOnCommitSequence() const override;

  scoped_refptr<base::SequencedTaskRunner> GetSequencedTaskRunner(
      SequenceID sequence_id) override;

 protected:
  ~MockDOMStorageTaskRunner() override;

 private:
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(MockDOMStorageTaskRunner);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_TASK_RUNNER_H_
