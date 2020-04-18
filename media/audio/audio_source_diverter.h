// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_SOURCE_DIVERTER_H_
#define MEDIA_AUDIO_AUDIO_SOURCE_DIVERTER_H_

#include "base/time/time.h"
#include "media/base/audio_bus.h"
#include "media/base/media_export.h"

// Audio sources may optionally implement AudioSourceDiverter to temporarily
// divert audio data to an alternate AudioOutputStream.  This allows the audio
// data to be plumbed to an alternate consumer; for example, a loopback
// mechanism for audio mirroring.

namespace media {

class AudioOutputStream;
class AudioParameters;

class MEDIA_EXPORT AudioPushSink {
 public:
  // Call this function to push audio data into the sink.
  virtual void OnData(std::unique_ptr<AudioBus> source,
                      base::TimeTicks reference_time) = 0;

  // Close the stream.
  // After calling this method, the object should not be used anymore.
  virtual void Close() = 0;
};

class MEDIA_EXPORT AudioSourceDiverter {
public:
  // Returns the audio parameters of the divertable audio data.
  virtual const AudioParameters& GetAudioParameters() = 0;

  // Start providing audio data to the given |to_stream|, which is in an
  // unopened state.  |to_stream| remains under the control of the
  // AudioSourceDiverter.
  virtual void StartDiverting(AudioOutputStream* to_stream) = 0;

  // Stops diverting audio data to the stream.  The AudioSourceDiverter is
  // responsible for making sure the stream is closed, perhaps asynchronously.
  virtual void StopDiverting() = 0;

  // Start duplicating the current audio stream, and push the copied data into
  // |sink|.
  virtual void StartDuplicating(AudioPushSink* sink) = 0;

  // Stop duplicating for the specified |sink|.  The AudioSourceDiverter is
  // responsible for making sure the sink is closed, perhaps asynchronously.
  virtual void StopDuplicating(AudioPushSink* sink) = 0;

 protected:
  virtual ~AudioSourceDiverter() {}
};

}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_SOURCE_DIVERTER_H_
