// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_recorder/audio_track_recorder.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/stl_util.h"
#include "content/renderer/media/stream/media_stream_audio_track.h"
#include "content/renderer/media_recorder/audio_track_opus_encoder.h"
#include "content/renderer/media_recorder/audio_track_pcm_encoder.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_parameters.h"
#include "media/base/bind_to_current_loop.h"

// Note that this code follows the Chrome media convention of defining a "frame"
// as "one multi-channel sample" as opposed to another common definition meaning
// "a chunk of samples". Here this second definition of "frame" is called a
// "buffer"; so what might be called "frame duration" is instead "buffer
// duration", and so on.

namespace content {

AudioTrackRecorder::CodecId AudioTrackRecorder::GetPreferredCodecId() {
  return CodecId::OPUS;
}

AudioTrackRecorder::AudioTrackRecorder(CodecId codec,
                                       const blink::WebMediaStreamTrack& track,
                                       OnEncodedAudioCB on_encoded_audio_cb,
                                       int32_t bits_per_second)
    : track_(track),
      encoder_(CreateAudioEncoder(codec,
                                  std::move(on_encoded_audio_cb),
                                  bits_per_second)),
      encoder_thread_("AudioEncoderThread") {
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  DCHECK(!track_.IsNull());
  DCHECK(MediaStreamAudioTrack::From(track_));

  // Start the |encoder_thread_|. From this point on, |encoder_| should work
  // only on |encoder_thread_|, as enforced by DCHECKs.
  encoder_thread_.Start();

  // Connect the source provider to the track as a sink.
  MediaStreamAudioSink::AddToAudioTrack(this, track_);
}

AudioTrackRecorder::~AudioTrackRecorder() {
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  MediaStreamAudioSink::RemoveFromAudioTrack(this, track_);
}

// Creates an audio encoder from the codec. Returns nullptr if the codec is
// invalid.
AudioTrackEncoder* AudioTrackRecorder::CreateAudioEncoder(
    CodecId codec,
    OnEncodedAudioCB on_encoded_audio_cb,
    int32_t bits_per_second) {
  if (codec == CodecId::PCM) {
    return new AudioTrackPcmEncoder(
        media::BindToCurrentLoop(std::move(on_encoded_audio_cb)));
  }

  // All other paths will use the AudioTrackOpusEncoder.
  return new AudioTrackOpusEncoder(
      media::BindToCurrentLoop(std::move(on_encoded_audio_cb)),
      bits_per_second);
}

void AudioTrackRecorder::OnSetFormat(const media::AudioParameters& params) {
  // If the source is restarted, might have changed to another capture thread.
  capture_thread_checker_.DetachFromThread();
  DCHECK(capture_thread_checker_.CalledOnValidThread());

  encoder_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioTrackEncoder::OnSetFormat, encoder_, params));
}

void AudioTrackRecorder::OnData(const media::AudioBus& audio_bus,
                                base::TimeTicks capture_time) {
  DCHECK(capture_thread_checker_.CalledOnValidThread());
  DCHECK(!capture_time.is_null());

  std::unique_ptr<media::AudioBus> audio_data =
      media::AudioBus::Create(audio_bus.channels(), audio_bus.frames());
  audio_bus.CopyTo(audio_data.get());

  encoder_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AudioTrackEncoder::EncodeAudio, encoder_,
                                std::move(audio_data), capture_time));
}

void AudioTrackRecorder::Pause() {
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  DCHECK(encoder_);
  encoder_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioTrackEncoder::set_paused, encoder_, true));
}

void AudioTrackRecorder::Resume() {
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  DCHECK(encoder_);
  encoder_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioTrackEncoder::set_paused, encoder_, false));
}

}  // namespace content
