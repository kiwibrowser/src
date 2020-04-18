// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_SOUNDS_TEST_DATA_H_
#define MEDIA_AUDIO_SOUNDS_TEST_DATA_H_

#include <stddef.h>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "media/audio/sounds/audio_stream_handler.h"

namespace media {

const int kTestAudioKey = 1000;

const char kTestAudioData[] =
    "RIFF\x28\x00\x00\x00WAVEfmt \x10\x00\x00\x00"
    "\x01\x00\x02\x00\x80\xbb\x00\x00\x00\x77\x01\x00\x02\x00\x10\x00"
    "data\x04\x00\x00\x00\x01\x00\x01\x00";
const size_t kTestAudioDataSize = arraysize(kTestAudioData) - 1;

class TestObserver : public AudioStreamHandler::TestObserver {
 public:
  TestObserver(const base::Closure& quit);
  ~TestObserver() override;

  // AudioStreamHandler::TestObserver implementation:
  void OnPlay() override;
  void OnStop(size_t cursor) override;

  int num_play_requests() const { return num_play_requests_; }
  int num_stop_requests() const { return num_stop_requests_; }
  int cursor() const { return cursor_; }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::Closure quit_;

  int num_play_requests_;
  int num_stop_requests_;
  int cursor_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

}  // namespace media

#endif  // MEDIA_AUDIO_SOUNDS_TEST_DATA_H_
