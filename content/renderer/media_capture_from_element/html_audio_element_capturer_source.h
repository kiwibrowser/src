// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_CAPTURE_FROM_ELEMENT_HTML_AUDIO_ELEMENT_CAPTURER_SOURCE_H_
#define CONTENT_RENDERER_MEDIA_CAPTURE_FROM_ELEMENT_HTML_AUDIO_ELEMENT_CAPTURER_SOURCE_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "content/renderer/media/stream/media_stream_audio_source.h"

namespace blink {
class WebMediaPlayer;
}  // namespace blink

namespace media {
class AudioBus;
class WebAudioSourceProviderImpl;
}  // namespace media

namespace content {

// This class is a MediaStreamAudioSource that registers to the constructor-
// passed weak WebAudioSourceProviderImpl to receive a copy of the audio data
// intended for rendering. This copied data is received on OnAudioBus() and sent
// to all the registered Tracks.
class CONTENT_EXPORT HtmlAudioElementCapturerSource final
    : public MediaStreamAudioSource {
 public:
  static HtmlAudioElementCapturerSource*
  CreateFromWebMediaPlayerImpl(blink::WebMediaPlayer* player);

  explicit HtmlAudioElementCapturerSource(
      media::WebAudioSourceProviderImpl* audio_source);
  ~HtmlAudioElementCapturerSource() override;

 private:
  // MediaStreamAudioSource implementation.
  bool EnsureSourceIsStarted() final;
  void EnsureSourceIsStopped() final;
  void SetAudioCallback();

  // To act as an WebAudioSourceProviderImpl::CopyAudioCB.
  void OnAudioBus(std::unique_ptr<media::AudioBus> audio_bus,
                  uint32_t frames_delayed,
                  int sample_rate);

  scoped_refptr<media::WebAudioSourceProviderImpl> audio_source_;

  bool is_started_;
  int last_sample_rate_;
  int last_num_channels_;
  int last_bus_frames_;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<HtmlAudioElementCapturerSource> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HtmlAudioElementCapturerSource);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_CAPTURE_FROM_ELEMENT_HTML_AUDIO_ELEMENT_CAPTURER_SOURCE_H_
