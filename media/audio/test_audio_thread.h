// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_TEST_AUDIO_THREAD_H_
#define MEDIA_AUDIO_TEST_AUDIO_THREAD_H_

#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "media/audio/audio_thread.h"

namespace media {

class TestAudioThread : public AudioThread {
 public:
  TestAudioThread();
  explicit TestAudioThread(bool use_real_thread);
  ~TestAudioThread() override;

  // AudioThread implementation.
  void Stop() override;
  base::SingleThreadTaskRunner* GetTaskRunner() override;
  base::SingleThreadTaskRunner* GetWorkerTaskRunner() override;

 private:
  std::unique_ptr<base::Thread> thread_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(TestAudioThread);
};

}  // namespace media

#endif  // MEDIA_AUDIO_TEST_AUDIO_THREAD_H_
