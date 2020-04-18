// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/cast_threads.h"

#include "base/logging.h"
#include "base/single_thread_task_runner.h"

CastThreads::CastThreads()
    : audio_encode_thread_("CastAudioEncodeThread"),
      video_encode_thread_("CastVideoEncodeThread") {
  audio_encode_thread_.Start();
  video_encode_thread_.Start();
}

scoped_refptr<base::SingleThreadTaskRunner>
CastThreads::GetAudioEncodeTaskRunner() {
  return audio_encode_thread_.task_runner();
}

scoped_refptr<base::SingleThreadTaskRunner>
CastThreads::GetVideoEncodeTaskRunner() {
  return video_encode_thread_.task_runner();
}
