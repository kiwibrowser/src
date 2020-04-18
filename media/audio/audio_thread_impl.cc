// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_thread_impl.h"

#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"

namespace media {

AudioThreadImpl::AudioThreadImpl() : thread_("AudioThread") {
#if defined(OS_WIN)
  thread_.init_com_with_mta(true);
#endif
  CHECK(thread_.Start());

#if defined(OS_MACOSX)
  // On Mac, the audio task runner must belong to the main thread.
  // See http://crbug.com/158170.
  task_runner_ = base::ThreadTaskRunnerHandle::Get();
#else
  task_runner_ = thread_.task_runner();
#endif
  worker_task_runner_ = thread_.task_runner();
}

AudioThreadImpl::~AudioThreadImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void AudioThreadImpl::Stop() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Note that on MACOSX, we can still have tasks posted on the |task_runner_|,
  // since it is the main thread task runner and we do not stop the main thread.
  // But this is fine becuase none of those tasks will actually run.
  thread_.Stop();
}

base::SingleThreadTaskRunner* AudioThreadImpl::GetTaskRunner() {
  return task_runner_.get();
}

base::SingleThreadTaskRunner* AudioThreadImpl::GetWorkerTaskRunner() {
  return worker_task_runner_.get();
}

}  // namespace media
