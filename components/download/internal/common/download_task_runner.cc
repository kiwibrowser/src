// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/download_task_runner.h"

#include "base/lazy_instance.h"
#include "base/no_destructor.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/task_scheduler/lazy_task_runner.h"
#include "build/build_config.h"

namespace download {

namespace {

#if defined(OS_WIN)
// On Windows, the download code dips into COM and the shell here and there,
// necessitating the use of a COM single-threaded apartment sequence.
base::LazyCOMSTATaskRunner g_download_task_runner =
    LAZY_COM_STA_TASK_RUNNER_INITIALIZER(
        base::TaskTraits(base::MayBlock(), base::TaskPriority::USER_VISIBLE),
        base::SingleThreadTaskRunnerThreadMode::SHARED);
#else
base::LazySequencedTaskRunner g_download_task_runner =
    LAZY_SEQUENCED_TASK_RUNNER_INITIALIZER(
        base::TaskTraits(base::MayBlock(), base::TaskPriority::USER_VISIBLE));
#endif

base::LazyInstance<scoped_refptr<base::SingleThreadTaskRunner>>::
    DestructorAtExit g_io_task_runner = LAZY_INSTANCE_INITIALIZER;

// Lock to protect |g_io_task_runner|
base::Lock& GetIOTaskRunnerLock() {
  static base::NoDestructor<base::Lock> instance;
  return *instance;
}

}  // namespace

scoped_refptr<base::SequencedTaskRunner> GetDownloadTaskRunner() {
  return g_download_task_runner.Get();
}

void SetIOTaskRunner(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  base::AutoLock auto_lock(GetIOTaskRunnerLock());
  static int count = 0;
  if (task_runner) {
    DCHECK(!g_io_task_runner.Get() ||
           task_runner.get() == g_io_task_runner.Get().get());
    count++;
    g_io_task_runner.Get() = task_runner;
    return;
  }

  count--;
  DCHECK_GE(count, 0);
  if (count == 0)
    g_io_task_runner.Get() = nullptr;
}

scoped_refptr<base::SingleThreadTaskRunner> GetIOTaskRunner() {
  base::AutoLock auto_lock(GetIOTaskRunnerLock());
  return g_io_task_runner.Get();
}

}  // namespace download
