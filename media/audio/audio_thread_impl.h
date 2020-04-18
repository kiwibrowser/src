// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_THREAD_IMPL_H_
#define MEDIA_AUDIO_AUDIO_THREAD_IMPL_H_

#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "media/audio/audio_thread.h"

namespace media {

class MEDIA_EXPORT AudioThreadImpl : public AudioThread {
 public:
  AudioThreadImpl();
  ~AudioThreadImpl() override;

  // AudioThread implementation.
  void Stop() override;
  base::SingleThreadTaskRunner* GetTaskRunner() override;
  base::SingleThreadTaskRunner* GetWorkerTaskRunner() override;

 private:
  base::Thread thread_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> worker_task_runner_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(AudioThreadImpl);
};

}  // namespace content

#endif  // MEDIA_AUDIO_AUDIO_THREAD_IMPL_H_
