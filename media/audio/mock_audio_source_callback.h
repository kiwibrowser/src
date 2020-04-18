// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_MOCK_AUDIO_SOURCE_CALLBACK_H_
#define MEDIA_AUDIO_MOCK_AUDIO_SOURCE_CALLBACK_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/time/time.h"
#include "media/audio/audio_io.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {

class MockAudioSourceCallback : public AudioOutputStream::AudioSourceCallback {
 public:
  MockAudioSourceCallback();
  ~MockAudioSourceCallback() override;

  MOCK_METHOD4(OnMoreData,
               int(base::TimeDelta, base::TimeTicks, int, AudioBus*));
  MOCK_METHOD0(OnError, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAudioSourceCallback);
};

}  // namespace media

#endif  // MEDIA_AUDIO_MOCK_AUDIO_SOURCE_CALLBACK_H_
