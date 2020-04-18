// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/audio_encoder.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_audio_encoder.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_AudioEncoder_0_1>() {
  return PPB_AUDIOENCODER_INTERFACE_0_1;
}

}  // namespace

AudioEncoder::AudioEncoder() {
}

AudioEncoder::AudioEncoder(const InstanceHandle& instance) {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    PassRefFromConstructor(
        get_interface<PPB_AudioEncoder_0_1>()->Create(instance.pp_instance()));
  }
}

AudioEncoder::AudioEncoder(const AudioEncoder& other) : Resource(other) {
}

int32_t AudioEncoder::GetSupportedProfiles(const CompletionCallbackWithOutput<
    std::vector<PP_AudioProfileDescription> >& cc) {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    return get_interface<PPB_AudioEncoder_0_1>()->GetSupportedProfiles(
        pp_resource(), cc.output(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t AudioEncoder::Initialize(uint32_t channels,
                                 PP_AudioBuffer_SampleRate input_sample_rate,
                                 PP_AudioBuffer_SampleSize input_sample_size,
                                 PP_AudioProfile output_profile,
                                 uint32_t initial_bitrate,
                                 PP_HardwareAcceleration acceleration,
                                 const CompletionCallback& cc) {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    return get_interface<PPB_AudioEncoder_0_1>()->Initialize(
        pp_resource(), channels, input_sample_rate, input_sample_size,
        output_profile, initial_bitrate, acceleration,
        cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t AudioEncoder::GetNumberOfSamples() {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    return get_interface<PPB_AudioEncoder_0_1>()->GetNumberOfSamples(
        pp_resource());
  }
  return PP_ERROR_NOINTERFACE;
}

int32_t AudioEncoder::GetBuffer(
    const CompletionCallbackWithOutput<AudioBuffer>& cc) {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    return get_interface<PPB_AudioEncoder_0_1>()->GetBuffer(
        pp_resource(), cc.output(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t AudioEncoder::Encode(const AudioBuffer& audio_buffer,
                             const CompletionCallback& cc) {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    return get_interface<PPB_AudioEncoder_0_1>()->Encode(
        pp_resource(), audio_buffer.pp_resource(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t AudioEncoder::GetBitstreamBuffer(
    const CompletionCallbackWithOutput<PP_AudioBitstreamBuffer>& cc) {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    return get_interface<PPB_AudioEncoder_0_1>()->GetBitstreamBuffer(
        pp_resource(), cc.output(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

void AudioEncoder::RecycleBitstreamBuffer(
    const PP_AudioBitstreamBuffer& bitstream_buffer) {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    get_interface<PPB_AudioEncoder_0_1>()->RecycleBitstreamBuffer(
        pp_resource(), &bitstream_buffer);
  }
}

void AudioEncoder::RequestBitrateChange(uint32_t bitrate) {
  if (has_interface<PPB_AudioEncoder_0_1>()) {
    get_interface<PPB_AudioEncoder_0_1>()->RequestBitrateChange(pp_resource(),
                                                                bitrate);
  }
}

void AudioEncoder::Close() {
  if (has_interface<PPB_AudioEncoder_0_1>())
    get_interface<PPB_AudioEncoder_0_1>()->Close(pp_resource());
}

}  // namespace pp
