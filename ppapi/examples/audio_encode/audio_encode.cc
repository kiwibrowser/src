// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include "ppapi/cpp/audio_buffer.h"
#include "ppapi/cpp/audio_encoder.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/logging.h"
#include "ppapi/cpp/media_stream_audio_track.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/utility/completion_callback_factory.h"

// When compiling natively on Windows, PostMessage can be #define-d to
// something else.
#ifdef PostMessage
#undef PostMessage
#endif

// This example demonstrates receiving audio samples from an audio
// track and encoding them.

namespace {

class AudioEncoderInstance : public pp::Instance {
 public:
  explicit AudioEncoderInstance(PP_Instance instance);
  virtual ~AudioEncoderInstance();

 private:
  // pp::Instance implementation.
  virtual void HandleMessage(const pp::Var& var_message);

  std::string ProfileToString(PP_AudioProfile profile);

  void GetSupportedProfiles();
  void OnGetSupportedProfiles(
      int32_t result,
      const std::vector<PP_AudioProfileDescription> profiles);

  void StartAudioTrack(const pp::Resource& resource_track);
  void StopAudioTrack();
  void OnGetBuffer(int32_t result, pp::AudioBuffer buffer);

  void InitializeEncoder();
  void OnEncoderInitialized(int32_t result);
  void OnAudioTrackConfigured(int32_t result);
  void GetEncoderBuffer(const pp::AudioBuffer& track_buffer);
  void OnGetEncoderBuffer(int32_t result,
                          pp::AudioBuffer buffer,
                          pp::AudioBuffer track_buffer);
  void OnEncodeDone(int32_t result);
  void OnGetBitstreamBuffer(int32_t result,
                            const PP_AudioBitstreamBuffer& buffer);

  pp::VarDictionary NewCommand(const std::string& type);

  void Log(const std::string& message);
  void LogError(int32_t error, const std::string& message);

  void PostAudioFormat();
  void PostDataMessage(const void* data, uint32_t size);

  pp::CompletionCallbackFactory<AudioEncoderInstance> callback_factory_;

  // Set when the plugin is not performing any encoding and just passing the
  // uncompressed data directly to web page.
  bool no_encoding_;

  uint32_t channels_;
  uint32_t sample_rate_;
  uint32_t sample_size_;
  uint32_t samples_per_frame_;

  pp::MediaStreamAudioTrack audio_track_;
  pp::AudioEncoder audio_encoder_;
};

AudioEncoderInstance::AudioEncoderInstance(PP_Instance instance)
    : pp::Instance(instance),
      callback_factory_(this),
      no_encoding_(true),
      channels_(0),
      sample_rate_(0),
      sample_size_(0),
      samples_per_frame_(0),
      audio_encoder_(this) {
  GetSupportedProfiles();
}

AudioEncoderInstance::~AudioEncoderInstance() {}

void AudioEncoderInstance::HandleMessage(const pp::Var& var_message) {
  if (!var_message.is_dictionary()) {
    LogError(PP_ERROR_FAILED,
             "Cannot handle incoming message, not a dictionary");
    return;
  }

  pp::VarDictionary dict_message(var_message);
  std::string command = dict_message.Get("command").AsString();

  if (command == "start") {
    pp::Var var_track = dict_message.Get("track");
    if (!var_track.is_resource()) {
      LogError(PP_ERROR_FAILED, "Given track is not a resource");
      return;
    }
    std::string profile = dict_message.Get("profile").AsString();
    if (profile == "wav") {
      no_encoding_ = true;
    } else {
      no_encoding_ = false;
    }
    StartAudioTrack(var_track.AsResource());
  } else if (command == "stop") {
    StopAudioTrack();
  } else {
    LogError(PP_ERROR_FAILED, "Invalid command");
  }
}

std::string AudioEncoderInstance::ProfileToString(PP_AudioProfile profile) {
  if (profile == PP_AUDIOPROFILE_OPUS)
    return "opus";
  return "unknown";
}

void AudioEncoderInstance::GetSupportedProfiles() {
  audio_encoder_.GetSupportedProfiles(callback_factory_.NewCallbackWithOutput(
      &AudioEncoderInstance::OnGetSupportedProfiles));
}

void AudioEncoderInstance::OnGetSupportedProfiles(
    int32_t result,
    const std::vector<PP_AudioProfileDescription> profiles) {
  pp::VarDictionary dictionary = NewCommand("supportedProfiles");
  pp::VarArray js_profiles;
  dictionary.Set(pp::Var("profiles"), js_profiles);

  if (result < 0) {
    LogError(result, "Cannot get supported profiles");
    PostMessage(dictionary);
  }

  int32_t idx = 0;
  for (std::vector<PP_AudioProfileDescription>::const_iterator it =
           profiles.begin();
       it != profiles.end(); ++it) {
    pp::VarDictionary js_profile;
    js_profile.Set(pp::Var("name"), pp::Var(ProfileToString(it->profile)));
    js_profile.Set(pp::Var("sample_size"),
                   pp::Var(static_cast<int32_t>(it->sample_size)));
    js_profile.Set(pp::Var("sample_rate"),
                   pp::Var(static_cast<int32_t>(it->sample_rate)));
    js_profiles.Set(idx++, js_profile);
  }
  PostMessage(dictionary);
}

void AudioEncoderInstance::StartAudioTrack(const pp::Resource& resource_track) {
  audio_track_ = pp::MediaStreamAudioTrack(resource_track);
  audio_track_.GetBuffer(callback_factory_.NewCallbackWithOutput(
      &AudioEncoderInstance::OnGetBuffer));
}

void AudioEncoderInstance::StopAudioTrack() {
  channels_ = 0;
  audio_track_.Close();
  audio_track_ = pp::MediaStreamAudioTrack();
  audio_encoder_.Close();
  audio_encoder_ = pp::AudioEncoder();
}

void AudioEncoderInstance::OnGetBuffer(int32_t result, pp::AudioBuffer buffer) {
  if (result == PP_ERROR_ABORTED)
    return;
  if (result != PP_OK) {
    LogError(result, "Cannot get audio track buffer");
    return;
  }

  // If this is the first buffer, then we need to initialize the encoder.
  if (channels_ == 0) {
    channels_ = buffer.GetNumberOfChannels();
    sample_rate_ = buffer.GetSampleRate();
    sample_size_ = buffer.GetSampleSize();
    samples_per_frame_ = buffer.GetNumberOfSamples();

    if (no_encoding_) {
      PostAudioFormat();
      PostDataMessage(buffer.GetDataBuffer(), buffer.GetDataBufferSize());
      audio_track_.GetBuffer(callback_factory_.NewCallbackWithOutput(
          &AudioEncoderInstance::OnGetBuffer));
    } else {
      InitializeEncoder();
    }

    // Given that once the encoder is initialized we might reconfigure the
    // media track, we discard the first buffer to keep this example a bit
    // simpler.
    audio_track_.RecycleBuffer(buffer);
  } else {
    if (no_encoding_) {
      PostDataMessage(buffer.GetDataBuffer(), buffer.GetDataBufferSize());
      audio_track_.RecycleBuffer(buffer);
    } else {
      GetEncoderBuffer(buffer);
    }

    audio_track_.GetBuffer(callback_factory_.NewCallbackWithOutput(
        &AudioEncoderInstance::OnGetBuffer));
  }
}

void AudioEncoderInstance::InitializeEncoder() {
  if (audio_encoder_.is_null())
    audio_encoder_ = pp::AudioEncoder(this);
  audio_encoder_.Initialize(
      channels_, static_cast<PP_AudioBuffer_SampleRate>(sample_rate_),
      static_cast<PP_AudioBuffer_SampleSize>(sample_size_),
      PP_AUDIOPROFILE_OPUS, 100000, PP_HARDWAREACCELERATION_WITHFALLBACK,
      callback_factory_.NewCallback(
          &AudioEncoderInstance::OnEncoderInitialized));
}

void AudioEncoderInstance::OnEncoderInitialized(int32_t result) {
  if (result == PP_ERROR_ABORTED)
    return;
  if (result != PP_OK) {
    LogError(result, "Cannot initialize encoder");
    return;
  }

  int32_t attribs[] = {
      // Duration in milliseconds.
      PP_MEDIASTREAMAUDIOTRACK_ATTRIB_DURATION,
      1000 / (sample_rate_ / audio_encoder_.GetNumberOfSamples()),
      PP_MEDIASTREAMAUDIOTRACK_ATTRIB_NONE};
  audio_track_.Configure(attribs,
                         callback_factory_.NewCallback(
                             &AudioEncoderInstance::OnAudioTrackConfigured));

  samples_per_frame_ = audio_encoder_.GetNumberOfSamples();
  PostAudioFormat();
}

void AudioEncoderInstance::OnAudioTrackConfigured(int32_t result) {
  if (result == PP_ERROR_ABORTED)
    return;
  if (result != PP_OK) {
    LogError(result, "Cannot configure audio track buffer duration");
    return;
  }

  audio_track_.GetBuffer(callback_factory_.NewCallbackWithOutput(
      &AudioEncoderInstance::OnGetBuffer));
  audio_encoder_.GetBitstreamBuffer(callback_factory_.NewCallbackWithOutput(
      &AudioEncoderInstance::OnGetBitstreamBuffer));
}

void AudioEncoderInstance::GetEncoderBuffer(
    const pp::AudioBuffer& track_buffer) {
  audio_encoder_.GetBuffer(callback_factory_.NewCallbackWithOutput(
      &AudioEncoderInstance::OnGetEncoderBuffer, track_buffer));
}

void AudioEncoderInstance::OnGetEncoderBuffer(int32_t result,
                                              pp::AudioBuffer buffer,
                                              pp::AudioBuffer track_buffer) {
  const char* error(nullptr);

  if (result != PP_OK)
    error = "Cannot get encoder buffer";
  if (buffer.GetDataBufferSize() != track_buffer.GetDataBufferSize()) {
    result = PP_ERROR_FAILED;
    error = "Invalid buffer size";
  }

  if (result == PP_OK) {
    memcpy(buffer.GetDataBuffer(), track_buffer.GetDataBuffer(),
           buffer.GetDataBufferSize());
    audio_encoder_.Encode(buffer, callback_factory_.NewCallback(
                                      &AudioEncoderInstance::OnEncodeDone));
  } else {
    LogError(result, error);
    StopAudioTrack();
  }
  audio_track_.RecycleBuffer(track_buffer);
}

void AudioEncoderInstance::OnEncodeDone(int32_t result) {}

void AudioEncoderInstance::OnGetBitstreamBuffer(
    int32_t result,
    const PP_AudioBitstreamBuffer& buffer) {
  if (result == PP_ERROR_ABORTED)
    return;
  if (result != PP_OK) {
    LogError(result, "Cannot get bitstream buffer");
    return;
  }

  audio_encoder_.GetBitstreamBuffer(callback_factory_.NewCallbackWithOutput(
      &AudioEncoderInstance::OnGetBitstreamBuffer));

  PostDataMessage(buffer.buffer, buffer.size);
  audio_encoder_.RecycleBitstreamBuffer(buffer);
}

pp::VarDictionary AudioEncoderInstance::NewCommand(const std::string& type) {
  pp::VarDictionary dictionary;
  dictionary.Set(pp::Var("command"), pp::Var(type));
  return dictionary;
}

void AudioEncoderInstance::Log(const std::string& message) {
  pp::VarDictionary dictionary = NewCommand("log");
  dictionary.Set(pp::Var("message"), pp::Var(message));
  PostMessage(dictionary);
}

void AudioEncoderInstance::LogError(int32_t error, const std::string& message) {
  pp::VarDictionary dictionary = NewCommand("error");
  std::string msg("Error: ");
  msg.append(message);
  msg.append(" : ");
  msg.append(pp::Var(error).DebugString());
  dictionary.Set(pp::Var("message"), pp::Var(msg));
  PostMessage(dictionary);
}

void AudioEncoderInstance::PostAudioFormat() {
  pp::VarDictionary dictionary = NewCommand("format");
  dictionary.Set(pp::Var("channels"), pp::Var(static_cast<int32_t>(channels_)));
  dictionary.Set(pp::Var("sample_rate"),
                 pp::Var(static_cast<int32_t>(sample_rate_)));
  dictionary.Set(pp::Var("sample_size"),
                 pp::Var(static_cast<int32_t>(sample_size_)));
  dictionary.Set(pp::Var("sample_per_frame"),
                 pp::Var(static_cast<int32_t>(samples_per_frame_)));
  PostMessage(dictionary);
}

void AudioEncoderInstance::PostDataMessage(const void* data, uint32_t size) {
  pp::VarDictionary dictionary = NewCommand("data");
  uint8_t* buffer;

  pp::VarArrayBuffer array_buffer = pp::VarArrayBuffer(size);
  buffer = static_cast<uint8_t*>(array_buffer.Map());
  memcpy(buffer, data, size);
  array_buffer.Unmap();

  dictionary.Set(pp::Var("buffer"), array_buffer);

  PostMessage(dictionary);
}

class AudioEncoderModule : public pp::Module {
 public:
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new AudioEncoderInstance(instance);
  }
};

}  // namespace

namespace pp {

// Factory function for your specialization of the Module object.
Module* CreateModule() {
  return new AudioEncoderModule();
}

}  // namespace pp
