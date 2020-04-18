// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_MEDIA_CAST_RECEIVER_AUDIO_VALVE_H_
#define CHROME_RENDERER_MEDIA_CAST_RECEIVER_AUDIO_VALVE_H_

#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "media/base/audio_capturer_source.h"
#include "media/base/audio_push_fifo.h"

namespace media {
class AudioBus;
}

// Forwards calls to |cb| until Stop is called.  If the client requested a
// different buffer size than that provided by the Cast Receiver, AudioPushFifo
// is used to rectify that.
//
// Thread-safe.
// All functions may block depending on contention.
class CastReceiverAudioValve :
    public base::RefCountedThreadSafe<CastReceiverAudioValve> {
 public:
  CastReceiverAudioValve(const media::AudioParameters& params,
                         media::AudioCapturerSource::CaptureCallback* cb);

  // Called on an unknown thread to provide more decoded audio data from the
  // Cast Receiver.
  void DeliverDecodedAudio(const media::AudioBus* audio_bus,
                           base::TimeTicks playout_time);

  // When this returns, no more calls will be forwarded to |cb|.
  void Stop();

 private:
  friend class base::RefCountedThreadSafe<CastReceiverAudioValve>;

  ~CastReceiverAudioValve();

  // Called by AudioPushFifo zero or more times during the call to Capture().
  // Delivers audio data in the required buffer size to |cb_|.
  void DeliverRebufferedAudio(const media::AudioBus& audio_bus,
                              int frame_delay);

  media::AudioCapturerSource::CaptureCallback* cb_;
  base::Lock lock_;

  media::AudioPushFifo fifo_;
  const int sample_rate_;

  // Used to pass the current playout time between DeliverDecodedAudio() and
  // DeviliverRebufferedAudio().
  base::TimeTicks current_playout_time_;
};

#endif  // CHROME_RENDERER_MEDIA_CAST_RECEIVER_AUDIO_VALVE_H_
