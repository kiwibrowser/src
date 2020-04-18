// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/owning_audio_manager_accessor.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "media/audio/audio_manager.h"
#include "media/audio/audio_thread.h"

namespace audio {

namespace {

// Thread class for hosting owned AudioManager on the main thread of the
// service, with a separate worker thread (started on-demand) for running things
// that shouldn't be blocked by main-thread tasks.
class MainThread : public media::AudioThread {
 public:
  MainThread();
  ~MainThread() override;

  // AudioThread implementation.
  void Stop() override;
  base::SingleThreadTaskRunner* GetTaskRunner() override;
  base::SingleThreadTaskRunner* GetWorkerTaskRunner() override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // This is not started until the first time GetWorkerTaskRunner() is called.
  base::Thread worker_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> worker_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(MainThread);
};

MainThread::MainThread()
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      worker_thread_("AudioWorkerThread") {}

MainThread::~MainThread() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

void MainThread::Stop() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (worker_task_runner_) {
    worker_task_runner_ = nullptr;
    worker_thread_.Stop();
  }
}

base::SingleThreadTaskRunner* MainThread::GetTaskRunner() {
  return task_runner_.get();
}

base::SingleThreadTaskRunner* MainThread::GetWorkerTaskRunner() {
  DCHECK(
      task_runner_->BelongsToCurrentThread() ||
      (worker_task_runner_ && worker_task_runner_->BelongsToCurrentThread()));
  if (!worker_task_runner_) {
    CHECK(worker_thread_.Start());
    worker_task_runner_ = worker_thread_.task_runner();
  }
  return worker_task_runner_.get();
}

}  // namespace

OwningAudioManagerAccessor::OwningAudioManagerAccessor(
    AudioManagerFactoryCallback audio_manager_factory_cb)
    : audio_manager_factory_cb_(std::move(audio_manager_factory_cb)) {
  DLOG(WARNING) << "Out of process audio service initializing.";
  DCHECK(audio_manager_factory_cb_);
}

OwningAudioManagerAccessor::~OwningAudioManagerAccessor() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

media::AudioManager* OwningAudioManagerAccessor::GetAudioManager() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!audio_manager_) {
    TRACE_EVENT0("audio", "AudioManager creation");
    DCHECK(audio_manager_factory_cb_);
    base::TimeTicks creation_start_time = base::TimeTicks::Now();

    // TODO(http://crbug/812557): pass AudioLogFactory (needed for output
    // streams).
    audio_manager_ = std::move(audio_manager_factory_cb_)
                         .Run(std::make_unique<MainThread>(), &log_factory_);
    DCHECK(audio_manager_);
    UMA_HISTOGRAM_TIMES("Media.AudioService.AudioManagerStartupTime",
                        base::TimeTicks::Now() - creation_start_time);
  }
  DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
  return audio_manager_.get();
}

void OwningAudioManagerAccessor::Shutdown() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (audio_manager_)
    audio_manager_->Shutdown();
  audio_manager_factory_cb_ = AudioManagerFactoryCallback();
}

}  // namespace audio
