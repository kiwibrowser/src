# TaskScheduler Migration

[TOC]

## Overview

[`base/task_scheduler/post_task.h`](https://cs.chromium.org/chromium/src/base/task_scheduler/post_task.h)
was introduced to Chrome in Q1. The API is fully documented under [Threading and
Tasks in Chrome](threading_and_tasks.md). This page will go into more details
about how to migrate callers of existing APIs to TaskScheduler.

The SequencedWorkerPools and BrowserThreads (not UI/IO) are already being
redirected to TaskScheduler under the hood so it's now "merely" a matter of
updating the actual call sites.

Much of the migration has already been automated but the callers that remain
require manual intervention from the OWNERS.

Here's [a list](https://docs.google.com/spreadsheets/d/18x9PGMlfgWcBr4fDz2SEEtIwTpSjcBFT2Puib47ZF1w/edit)
of everything that's left. Please pick items in this list, assign them to self
and tick the boxes when done.

And some [slides](https://docs.google.com/presentation/d/191H9hBO0r5pH2JVMeYYV-yrP1175JoSlrZyK5QEeUlE/edit?usp=sharing)
with a migration example.

## BlockingPool (and other SequencedWorkerPools)

Tag migration CLs with BUG=[667892](https://crbug.com/667892).

The remaining callers of BrowserThread::GetBlockingPool() require manual
intervention because they're plumbing the SequencedWorkerPool multiple layers
into another component.

The TaskScheduler API explicitly discourages this paradigm. Instead exposing a
static API from post_task.h and encouraging that individual components grab the
TaskRunner/TaskTraits they need instead of bring injected one from their owner
and hoping for the right traits. This often allows cleaning up multiple layers
of plumbing without otherwise hurting testing as documented
[here](threading_and_tasks.md#TaskRunner-ownership-encourage-no-dependency-injection).

Replace methods that used to
```cpp
  DCHECK(content::BrowserThread::GetBlockingPool()->RunsTasksInCurrentSequence());
```
and make them use
```cpp
  base::ThreadRestrictions::AssertIOAllowed();
```

The TaskScheduler API intentionally doesn't provide a
TaskScheduler::RunsTasksInCurrentSequence() equivalent as ultimately everything
will run in TaskScheduler and that'd be meaningless... As such prefer asserting
the properties your task needs as documentation rather than where it runs.

You can of course still use
```cpp
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
```
if you have access to the TaskRunner instance (generic asserts discussed above
are meant for anonymous methods that require static checks).

Note: Contrary to overheard belief: SequencedWorkerPool::PostTask() resulted in
an unsequenced (parallel) task. The SequencedWorkerPool allowed posting to
sequences but on its own was just a plain TaskRunner (despite having sequence in
its name...). base::PostTaskWithTraits() is thus the proper replacement if not
having explicit sequencing was intended, otherwise
base::CreateSequenceTaskRunnerWithTraits() is what you're looking for.

## BrowserThreads

All BrowserThreads but UI/IO are being migrated to TaskScheduler
(i.e. FILE/FILE_USER_BLOCKING/DB/PROCESS_LAUNCHER/CACHE).

Tag migration CLs with BUG=[689520](https://crbug.com/689520).

This migration requires manual intervention because:
 1. Everything on BrowserThread::FOO has to be assumed to depend on being
    sequenced with everything else on BrowserThread::FOO until decided otherwise
    by a developer.
 2. Everything on BrowserThread::FOO has to be assumed to be thread-affine until
    decided otherwise by a developer.

As a developer your goal is to get rid of all uses of BrowserThread::FOO in your
assigned files by:
 1. Splitting things into their own execution sequence (i.e. post to a TaskRunner
    obtained from post_task.h -- see [Threading and Tasks in
    Chrome](threading_and_tasks.md) for details).
 2. Removing the plumbing: if GetTaskRunnerForThread(BrowserThread::FOO) is
    passed down into a component the prefered paradigm is to remove all of that
    plumbing and simply have the leaf layers requiring a TaskRunner get it from
    base::CreateSequencedTaskRunnerWithTraits() directly.
 3. Ideally migrating from a single-threaded context to a
    [much preferred](threading_and_tasks.md#Prefer-Sequences-to-Threads) sequenced context.
    * Note: if your tasks use COM APIs (Component Object Model on Windows),
      you'll need to use CreateCOMSTATaskRunnerWithTraits() and sequencing will
      not be an option (there are DCHECKs in place that will fire if your task
      uses COM without being on a COM initialized TaskRunner).

## Relevant single-thread -> sequence mappings

* base::SingleThreadTaskRunner -> base::SequencedTaskRunner
  * SingleThreadTaskRunner::BelongsToCurrentThread() -> SeqeuenceTaskRunner::RunsTasksInCurrentSequence()
* base::ThreadTaskRunnerHandle -> base::SequencedTaskRunnerHandle
* base::ThreadChecker -> base::SequenceChecker
  * ThreadChecker::CalledOnValidThread() -> DCHECK_CALLED_ON_VALID_SEQUENCE(...)
* base::ThreadLocalStorage::Slot -> base::SequenceLocalStorageSlot
* BrowserThread::PostTaskAndReplyWithResult() -> base::PostTaskAndReplyWithResult()
  (from post_task.h or from task_runner_util.h (if you need to feed a TaskRunner))
* BrowserThread::DeleteOnThread -> base::OnTaskRunnerDeleter / base::RefCountedDeleteOnSequence
* BrowserMessageFilter::OverrideThreadForMessage() -> BrowserMessageFilter::OverrideTaskRunnerForMessage()
* CreateSingleThreadTaskRunnerWithTraits() -> CreateSequencedTaskRunnerWithTraits()
   * Every CreateSingleThreadTaskRunnerWithTraits() usage should be accompanied
     with a comment and ideally a bug to make it sequence when the sequence-unfriendly
     dependency is addressed (again [Prefer Sequences to
     Threads](threading_and_tasks.md#Prefer-Sequences-to-Threads)).


### Other relevant mappings for tests

* base::MessageLoop -> base::test::ScopedTaskEnvironment
* content::TestBrowserThread -> content::TestBrowserThreadBundle (if you still
  need other BrowserThreads and ScopedTaskEnvironment if you don't)
* base::RunLoop().Run() -(maybe)> content::RunAllTasksUntilIdle()
   * If test code was previously using RunLoop to execute things off the main
     thread (as TestBrowserThreadBundle grouped everything under a single
     MessageLoop), flushing tasks will now require asking for that explicitly.
   * Or ScopedTaskEnvironment::RunUntilIdle() if you're not using
     TestBrowserThreadBundle.
   * If you need to control the order of execution of main thread versus
     scheduler you can individually RunLoop.Run() and
     TaskScheduler::FlushForTesting()
   * If you need the TaskScheduler to not run anything until explicitly asked to
     use ScopedTaskEnvironment::ExecutionMode::QUEUED.

## Other known migration hurdles and recommended paradigms
* Everything in a file/component needs to run on the same sequence but there
  isn't a clear place to own/access the common SequencedTaskRunner =>
  base::Lazy(Sequenced|SingleThread|COMSTA)TaskRunner.
* For anything else, ping [base/task_scheduler/OWNERS](https://cs.chromium.org/chromium/src/base/task_scheduler/OWNERS)
  or [scheduler-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/scheduler-dev),
  thanks!
