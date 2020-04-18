// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_AUDIO_SINK_H_
#define CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_AUDIO_SINK_H_

#include <vector>

#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/public/renderer/media_stream_sink.h"

namespace blink {
class WebMediaStreamTrack;
}

namespace media {
class AudioBus;
class AudioParameters;
}

namespace content {

class CONTENT_EXPORT MediaStreamAudioSink : public MediaStreamSink {
 public:
  // Adds a MediaStreamAudioSink to the audio track to receive audio data from
  // the track.
  // Called on the main render thread.
  static void AddToAudioTrack(MediaStreamAudioSink* sink,
                              const blink::WebMediaStreamTrack& track);

  // Removes a MediaStreamAudioSink from the audio track to stop receiving
  // audio data from the track.
  // Called on the main render thread.
  static void RemoveFromAudioTrack(MediaStreamAudioSink* sink,
                                   const blink::WebMediaStreamTrack& track);

  // Returns the format of the audio track.
  // Called on the main render thread.
  static media::AudioParameters GetFormatFromAudioTrack(
      const blink::WebMediaStreamTrack& track);

  // Callback called to deliver audio data. The data in |audio_bus| respects the
  // AudioParameters passed in the last call to OnSetFormat().  Called on
  // real-time audio thread.
  //
  // |estimated_capture_time| is the local time at which the first sample frame
  // in |audio_bus| either: 1) was generated, if it was done so locally; or 2)
  // should be targeted for play-out, if it was generated from a remote
  // source. Either way, an implementation should not play-out the audio before
  // this point-in-time. This value is NOT a high-resolution timestamp, and so
  // it should not be used as a presentation time; but, instead, it should be
  // used for buffering playback and for A/V synchronization purposes.
  virtual void OnData(const media::AudioBus& audio_bus,
                      base::TimeTicks estimated_capture_time) = 0;

  // Callback called when the format of the audio stream has changed.  This is
  // always called at least once before OnData(), and on the same thread.
  virtual void OnSetFormat(const media::AudioParameters& params) = 0;

 protected:
  ~MediaStreamAudioSink() override {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_AUDIO_SINK_H_
