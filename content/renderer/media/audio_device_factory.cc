// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/audio_device_factory.h"

#include <algorithm>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "content/common/content_constants_internal.h"
#include "content/common/media/renderer_audio_input_stream_factory.mojom.h"
#include "content/renderer/media/audio_input_ipc_factory.h"
#include "content/renderer/media/audio_output_ipc_factory.h"
#include "content/renderer/media/audio_renderer_mixer_manager.h"
#include "content/renderer/media/mojo_audio_input_ipc.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "media/audio/audio_input_device.h"
#include "media/audio/audio_output_device.h"
#include "media/base/audio_renderer_mixer_input.h"
#include "media/base/media_switches.h"

namespace content {

// static
AudioDeviceFactory* AudioDeviceFactory::factory_ = nullptr;

namespace {
#if defined(OS_WIN) || defined(OS_MACOSX) || \
    (defined(OS_LINUX) && !defined(OS_CHROMEOS))
// Due to driver deadlock issues on Windows (http://crbug/422522) there is a
// chance device authorization response is never received from the browser side.
// In this case we will time out, to avoid renderer hang forever waiting for
// device authorization (http://crbug/615589). This will result in "no audio".
// There are also cases when authorization takes too long on Mac and Linux.
const int64_t kMaxAuthorizationTimeoutMs = 10000;
#else
const int64_t kMaxAuthorizationTimeoutMs = 0;  // No timeout.
#endif

scoped_refptr<media::AudioOutputDevice> NewOutputDevice(
    int render_frame_id,
    int session_id,
    const std::string& device_id) {
  auto device = base::MakeRefCounted<media::AudioOutputDevice>(
      AudioOutputIPCFactory::get()->CreateAudioOutputIPC(render_frame_id),
      AudioOutputIPCFactory::get()->io_task_runner(), session_id, device_id,
      // Set authorization request timeout at 80% of renderer hung timeout,
      // but no more than kMaxAuthorizationTimeout.
      base::TimeDelta::FromMilliseconds(
          std::min(kHungRendererDelayMs * 8 / 10, kMaxAuthorizationTimeoutMs)));
  device->RequestDeviceAuthorization();
  return device;
}

// This is where we decide which audio will go to mixers and which one to
// AudioOutpuDevice directly.
bool IsMixable(AudioDeviceFactory::SourceType source_type) {
  if (source_type == AudioDeviceFactory::kSourceMediaElement)
    return true;  // Must ALWAYS go through mixer.

  // Mix everything if experiment is enabled; otherwise mix nothing else.
  return base::FeatureList::IsEnabled(media::kNewAudioRenderingMixingStrategy);
}

scoped_refptr<media::SwitchableAudioRendererSink> NewMixableSink(
    AudioDeviceFactory::SourceType source_type,
    int render_frame_id,
    int session_id,
    const std::string& device_id) {
  RenderThreadImpl* render_thread = RenderThreadImpl::current();
  DCHECK(render_thread) << "RenderThreadImpl is not instantiated, or "
                        << "GetOutputDeviceInfo() is called on a wrong thread ";
  return scoped_refptr<media::AudioRendererMixerInput>(
      render_thread->GetAudioRendererMixerManager()->CreateInput(
          render_frame_id, session_id, device_id,
          AudioDeviceFactory::GetSourceLatencyType(source_type)));
}

}  // namespace

media::AudioLatency::LatencyType AudioDeviceFactory::GetSourceLatencyType(
    AudioDeviceFactory::SourceType source) {
  switch (source) {
    case AudioDeviceFactory::kSourceWebAudioInteractive:
      return media::AudioLatency::LATENCY_INTERACTIVE;
    case AudioDeviceFactory::kSourceNone:
    case AudioDeviceFactory::kSourceWebRtc:
    case AudioDeviceFactory::kSourceNonRtcAudioTrack:
    case AudioDeviceFactory::kSourceWebAudioBalanced:
      return media::AudioLatency::LATENCY_RTC;
    case AudioDeviceFactory::kSourceMediaElement:
    case AudioDeviceFactory::kSourceWebAudioPlayback:
      return media::AudioLatency::LATENCY_PLAYBACK;
    case AudioDeviceFactory::kSourceWebAudioExact:
      return media::AudioLatency::LATENCY_EXACT_MS;
  }
  NOTREACHED();
  return media::AudioLatency::LATENCY_INTERACTIVE;
}

scoped_refptr<media::AudioRendererSink>
AudioDeviceFactory::NewAudioRendererMixerSink(int render_frame_id,
                                              int session_id,
                                              const std::string& device_id) {
  return NewFinalAudioRendererSink(render_frame_id, session_id, device_id);
}

// static
scoped_refptr<media::AudioRendererSink>
AudioDeviceFactory::NewAudioRendererSink(SourceType source_type,
                                         int render_frame_id,
                                         int session_id,
                                         const std::string& device_id) {
  if (factory_) {
    scoped_refptr<media::AudioRendererSink> device =
        factory_->CreateAudioRendererSink(source_type, render_frame_id,
                                          session_id, device_id);
    if (device)
      return device;
  }

  if (IsMixable(source_type))
    return NewMixableSink(source_type, render_frame_id, session_id, device_id);

  UMA_HISTOGRAM_BOOLEAN("Media.Audio.Render.SinkCache.UsedForSinkCreation",
                        false);
  return NewFinalAudioRendererSink(render_frame_id, session_id, device_id);
}

// static
scoped_refptr<media::SwitchableAudioRendererSink>
AudioDeviceFactory::NewSwitchableAudioRendererSink(
    SourceType source_type,
    int render_frame_id,
    int session_id,
    const std::string& device_id) {
  if (factory_) {
    scoped_refptr<media::SwitchableAudioRendererSink> sink =
        factory_->CreateSwitchableAudioRendererSink(
            source_type, render_frame_id, session_id, device_id);
    if (sink)
      return sink;
  }

  if (IsMixable(source_type))
    return NewMixableSink(source_type, render_frame_id, session_id, device_id);

  // AudioOutputDevice is not RestartableAudioRendererSink, so we can't return
  // anything for those who wants to create an unmixable sink.
  NOTIMPLEMENTED();
  return nullptr;
}

// static
scoped_refptr<media::AudioCapturerSource>
AudioDeviceFactory::NewAudioCapturerSource(int render_frame_id,
                                           int session_id) {
  if (factory_) {
    // We don't pass on |session_id|, as this branch is only used for tests.
    scoped_refptr<media::AudioCapturerSource> source =
        factory_->CreateAudioCapturerSource(render_frame_id);
    if (source)
      return source;
  }

  return base::MakeRefCounted<media::AudioInputDevice>(
      AudioInputIPCFactory::get()->CreateAudioInputIPC(render_frame_id,
                                                       session_id));
}

// static
media::OutputDeviceInfo AudioDeviceFactory::GetOutputDeviceInfo(
    int render_frame_id,
    int session_id,
    const std::string& device_id) {
  RenderThreadImpl* render_thread = RenderThreadImpl::current();
  DCHECK(render_thread) << "RenderThreadImpl is not instantiated, or "
                        << "GetOutputDeviceInfo() is called on a wrong thread ";
  return render_thread->GetAudioRendererMixerManager()->GetOutputDeviceInfo(
      render_frame_id, session_id, device_id);
}

AudioDeviceFactory::AudioDeviceFactory() {
  DCHECK(!factory_) << "Can't register two factories at once.";
  factory_ = this;
}

AudioDeviceFactory::~AudioDeviceFactory() {
  factory_ = nullptr;
}

// static
scoped_refptr<media::AudioRendererSink>
AudioDeviceFactory::NewFinalAudioRendererSink(int render_frame_id,
                                              int session_id,
                                              const std::string& device_id) {
  if (factory_) {
    scoped_refptr<media::AudioRendererSink> sink =
        factory_->CreateFinalAudioRendererSink(render_frame_id, session_id,
                                               device_id);
    if (sink)
      return sink;
  }

  return NewOutputDevice(render_frame_id, session_id, device_id);
}

}  // namespace content
