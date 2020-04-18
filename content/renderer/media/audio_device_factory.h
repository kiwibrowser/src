// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_AUDIO_DEVICE_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_AUDIO_DEVICE_FACTORY_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "media/base/audio_latency.h"
#include "media/base/output_device_info.h"

namespace media {
class AudioRendererSink;
class SwitchableAudioRendererSink;
class AudioCapturerSource;
}

namespace content {

// A factory for creating AudioRendererSinks and AudioCapturerSources. There is
// a global factory function that can be installed for the purposes of testing
// to provide specialized implementations.
// TODO(olka): rename it, probably split it into AudioRendererSinkFactory and
// AudioCapturerSourceFactory.
class CONTENT_EXPORT AudioDeviceFactory {
 public:
  // Types of audio sources. Each source can have individual mixing and/or
  // latency requirements for output. The source is specified by the client when
  // requesting output sink from the factory, and the factory creates the output
  // sink basing on those requirements.
  enum SourceType {
    kSourceNone = 0,
    kSourceMediaElement,
    kSourceWebRtc,
    kSourceNonRtcAudioTrack,
    kSourceWebAudioInteractive,
    kSourceWebAudioBalanced,
    kSourceWebAudioPlayback,
    kSourceWebAudioExact,
    kSourceLast = kSourceWebAudioExact  // Only used for validation of format.
  };

  // Maps the source type to the audio latency it requires.
  static media::AudioLatency::LatencyType GetSourceLatencyType(
      SourceType source);

  // Creates a sink for AudioRendererMixer.
  // |render_frame_id| refers to the RenderFrame containing the entity
  // producing the audio. If |session_id| is nonzero, it is used by the browser
  // to select the correct input device ID and its associated output device, if
  // it exists. If |session_id| is zero, |device_id| identify the output device
  // to use.
  // If |session_id| is zero and |device_id| is empty, the default output
  // device will be selected.
  static scoped_refptr<media::AudioRendererSink> NewAudioRendererMixerSink(
      int render_frame_id,
      int session_id,
      const std::string& device_id);

  // Creates an AudioRendererSink bound to an AudioOutputDevice.
  // Basing on |source_type| and build configuration, audio played out through
  // the sink goes to AOD directly or can be mixed with other audio before that.
  // TODO(olka): merge it with NewRestartableOutputDevice() as soon as
  // AudioOutputDevice is fixed to be restartable.
  static scoped_refptr<media::AudioRendererSink> NewAudioRendererSink(
      SourceType source_type,
      int render_frame_id,
      int session_id,
      const std::string& device_id);

  // Creates a SwitchableAudioRendererSink bound to an AudioOutputDevice
  // Basing on |source_type| and build configuration, audio played out through
  // the sink goes to AOD directly or can be mixed with other audio before that.
  static scoped_refptr<media::SwitchableAudioRendererSink>
  NewSwitchableAudioRendererSink(SourceType source_type,
                                 int render_frame_id,
                                 int session_id,
                                 const std::string& device_id);

  // A helper to get device info in the absence of AudioOutputDevice.
  // Must be called on renderer thread only.
  static media::OutputDeviceInfo GetOutputDeviceInfo(
      int render_frame_id,
      int session_id,
      const std::string& device_id);

  // Creates an AudioCapturerSource using the currently registered factory.
  // |render_frame_id| refers to the RenderFrame containing the entity
  // consuming the audio.
  static scoped_refptr<media::AudioCapturerSource> NewAudioCapturerSource(
      int render_frame_id,
      int session_id);

 protected:
  AudioDeviceFactory();
  virtual ~AudioDeviceFactory();

  // You can derive from this class and specify an implementation for these
  // functions to provide alternate audio device implementations.
  // If the return value of either of these function is NULL, we fall back
  // on the default implementation.

  // Creates a final sink in the rendering pipeline, which represents the actual
  // output device.
  virtual scoped_refptr<media::AudioRendererSink> CreateFinalAudioRendererSink(
      int render_frame_id,
      int sesssion_id,
      const std::string& device_id) = 0;

  virtual scoped_refptr<media::AudioRendererSink> CreateAudioRendererSink(
      SourceType source_type,
      int render_frame_id,
      int sesssion_id,
      const std::string& device_id) = 0;

  virtual scoped_refptr<media::SwitchableAudioRendererSink>
  CreateSwitchableAudioRendererSink(SourceType source_type,
                                    int render_frame_id,
                                    int sesssion_id,
                                    const std::string& device_id) = 0;

  virtual scoped_refptr<media::AudioCapturerSource> CreateAudioCapturerSource(
      int render_frame_id) = 0;

 private:
  // The current globally registered factory. This is NULL when we should
  // create the default AudioRendererSinks.
  static AudioDeviceFactory* factory_;

  static scoped_refptr<media::AudioRendererSink> NewFinalAudioRendererSink(
      int render_frame_id,
      int session_id,
      const std::string& device_id);

  DISALLOW_COPY_AND_ASSIGN(AudioDeviceFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_AUDIO_DEVICE_FACTORY_H_
