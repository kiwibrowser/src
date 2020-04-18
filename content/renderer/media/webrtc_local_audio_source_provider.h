// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_LOCAL_AUDIO_SOURCE_PROVIDER_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_LOCAL_AUDIO_SOURCE_PROVIDER_H_

#include <stddef.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/public/renderer/media_stream_audio_sink.h"
#include "media/base/audio_converter.h"
#include "third_party/blink/public/platform/web_audio_source_provider.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace media {
class AudioBus;
class AudioConverter;
class AudioFifo;
class AudioParameters;
}

namespace blink {
class WebAudioSourceProviderClient;
}

namespace content {

// TODO(miu): This implementation should be renamed to WebAudioMediaStreamSink,
// as it should work as a provider for WebAudio from ANY MediaStreamAudioTrack.
// http://crbug.com/577874
//
// WebRtcLocalAudioSourceProvider provides a bridge between classes:
//     MediaStreamAudioTrack ---> blink::WebAudioSourceProvider
//
// WebRtcLocalAudioSourceProvider works as a sink to the MediaStreamAudioTrack
// and stores the capture data to a FIFO. When the media stream is connected to
// WebAudio MediaStreamAudioSourceNode as a source provider,
// MediaStreamAudioSourceNode will periodically call provideInput() to get the
// data from the FIFO.
//
// All calls are protected by a lock.
class CONTENT_EXPORT WebRtcLocalAudioSourceProvider
    : public blink::WebAudioSourceProvider,
      public media::AudioConverter::InputCallback,
      public MediaStreamAudioSink {
 public:
  static const size_t kWebAudioRenderBufferSize;

  explicit WebRtcLocalAudioSourceProvider(
      const blink::WebMediaStreamTrack& track);
  ~WebRtcLocalAudioSourceProvider() override;

  // MediaStreamAudioSink implementation.
  void OnData(const media::AudioBus& audio_bus,
              base::TimeTicks estimated_capture_time) override;
  void OnSetFormat(const media::AudioParameters& params) override;
  void OnReadyStateChanged(
      blink::WebMediaStreamSource::ReadyState state) override;

  // blink::WebAudioSourceProvider implementation.
  void SetClient(blink::WebAudioSourceProviderClient* client) override;
  void ProvideInput(const blink::WebVector<float*>& audio_data,
                    size_t number_of_frames) override;

  // media::AudioConverter::Inputcallback implementation.
  // This function is triggered by provideInput()on the WebAudio audio thread,
  // so it has been under the protection of |lock_|.
  double ProvideInput(media::AudioBus* audio_bus,
                      uint32_t frames_delayed) override;

  // Method to allow the unittests to inject its own sink parameters to avoid
  // query the hardware.
  // TODO(xians,tommi): Remove and instead offer a way to inject the sink
  // parameters so that the implementation doesn't rely on the global default
  // hardware config but instead gets the parameters directly from the sink
  // (WebAudio in this case). Ideally the unit test should be able to use that
  // same mechanism to inject the sink parameters for testing.
  void SetSinkParamsForTesting(const media::AudioParameters& sink_params);

 private:
  // Used to DCHECK that some methods are called on the capture audio thread.
  base::ThreadChecker capture_thread_checker_;

  std::unique_ptr<media::AudioConverter> audio_converter_;
  std::unique_ptr<media::AudioFifo> fifo_;
  std::unique_ptr<media::AudioBus> output_wrapper_;
  bool is_enabled_;
  media::AudioParameters source_params_;
  media::AudioParameters sink_params_;

  // Protects all the member variables above.
  base::Lock lock_;

  // Used to report the correct delay to |webaudio_source_|.
  base::TimeTicks last_fill_;

  // The audio track that this source provider is connected to.
  blink::WebMediaStreamTrack track_;

  // Flag to tell if the track has been stopped or not.
  bool track_stopped_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcLocalAudioSourceProvider);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_LOCAL_AUDIO_SOURCE_PROVIDER_H_
