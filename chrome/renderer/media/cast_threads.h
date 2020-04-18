// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Manages threads used by Cast Streaming Extensions API. There is a
// singleton object of this class in the renderer.
//
// There are two threads owned by this class:
// 1. Audio encode thread.
// 2. Video encode thread.
// These two threads are started this object is created.

#ifndef CHROME_RENDERER_MEDIA_CAST_THREADS_H_
#define CHROME_RENDERER_MEDIA_CAST_THREADS_H_

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"

class CastThreads {
 public:
  scoped_refptr<base::SingleThreadTaskRunner> GetAudioEncodeTaskRunner();
  scoped_refptr<base::SingleThreadTaskRunner> GetVideoEncodeTaskRunner();

 private:
  friend struct base::LazyInstanceTraitsBase<CastThreads>;

  CastThreads();

  base::Thread audio_encode_thread_;
  base::Thread video_encode_thread_;

  DISALLOW_COPY_AND_ASSIGN(CastThreads);
};

#endif  // CHROME_RENDERER_MEDIA_CAST_THREADS_H_
