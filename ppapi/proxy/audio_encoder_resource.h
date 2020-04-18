// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_AUDIO_ENCODER_RESOURCE_H_
#define PPAPI_PROXY_AUDIO_ENCODER_RESOURCE_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/shared_impl/media_stream_buffer_manager.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/thunk/ppb_audio_encoder_api.h"

namespace ppapi {

class TrackedCallback;

namespace proxy {

class AudioBufferResource;

class PPAPI_PROXY_EXPORT AudioEncoderResource
    : public PluginResource,
      public thunk::PPB_AudioEncoder_API,
      public ppapi::MediaStreamBufferManager::Delegate {
 public:
  AudioEncoderResource(Connection connection, PP_Instance instance);
  ~AudioEncoderResource() override;

  thunk::PPB_AudioEncoder_API* AsPPB_AudioEncoder_API() override;

 private:
  // MediaStreamBufferManager::Delegate implementation.
  void OnNewBufferEnqueued() override {}

  // PPB_AudioEncoder_API implementation.
  int32_t GetSupportedProfiles(
      const PP_ArrayOutput& output,
      const scoped_refptr<TrackedCallback>& callback) override;
  int32_t Initialize(uint32_t channels,
                     PP_AudioBuffer_SampleRate input_sample_rate,
                     PP_AudioBuffer_SampleSize input_sample_size,
                     PP_AudioProfile output_profile,
                     uint32_t initial_bitrate,
                     PP_HardwareAcceleration acceleration,
                     const scoped_refptr<TrackedCallback>& callback) override;
  int32_t GetNumberOfSamples() override;
  int32_t GetBuffer(PP_Resource* audio_buffer,
                    const scoped_refptr<TrackedCallback>& callback) override;
  int32_t Encode(PP_Resource audio_buffer,
                 const scoped_refptr<TrackedCallback>& callback) override;
  int32_t GetBitstreamBuffer(
      PP_AudioBitstreamBuffer* bitstream_buffer,
      const scoped_refptr<TrackedCallback>& callback) override;
  void RecycleBitstreamBuffer(
      const PP_AudioBitstreamBuffer* bitstream_buffer) override;
  void RequestBitrateChange(uint32_t bitrate) override;
  void Close() override;

  // PluginResource implementation.
  void OnReplyReceived(const ResourceMessageReplyParams& params,
                       const IPC::Message& msg) override;

  // Message handlers for the host's messages.
  void OnPluginMsgGetSupportedProfilesReply(
      const PP_ArrayOutput& output,
      const ResourceMessageReplyParams& params,
      const std::vector<PP_AudioProfileDescription>& profiles);
  void OnPluginMsgInitializeReply(const ResourceMessageReplyParams& params,
                                  int32_t number_of_samples,
                                  int32_t audio_buffer_count,
                                  int32_t audio_buffer_size,
                                  int32_t bitstream_buffer_count,
                                  int32_t bitstream_buffer_size);
  void OnPluginMsgEncodeReply(const ResourceMessageReplyParams& params,
                              int32_t buffer_id);
  void OnPluginMsgBitstreamBufferReady(const ResourceMessageReplyParams& params,
                                       int32_t buffer_id);
  void OnPluginMsgNotifyError(const ResourceMessageReplyParams& params,
                              int32_t error);

  // Internal utility functions.
  void NotifyError(int32_t error);
  void TryGetAudioBuffer();
  void TryWriteBitstreamBuffer();
  void ReleaseBuffers();

  int32_t encoder_last_error_;

  bool initialized_;

  uint32_t number_of_samples_;

  using AudioBufferMap =
      std::map<PP_Resource, scoped_refptr<AudioBufferResource>>;
  AudioBufferMap audio_buffers_;

  scoped_refptr<TrackedCallback> get_supported_profiles_callback_;
  scoped_refptr<TrackedCallback> initialize_callback_;
  scoped_refptr<TrackedCallback> get_buffer_callback_;
  PP_Resource* get_buffer_data_;

  using EncodeMap = std::map<int32_t, scoped_refptr<TrackedCallback>>;
  EncodeMap encode_callbacks_;

  scoped_refptr<TrackedCallback> get_bitstream_buffer_callback_;
  PP_AudioBitstreamBuffer* get_bitstream_buffer_data_;

  MediaStreamBufferManager audio_buffer_manager_;
  MediaStreamBufferManager bitstream_buffer_manager_;

  // Map of bitstream buffer pointers to buffer ids.
  using BufferMap = std::map<void*, int32_t>;
  BufferMap bitstream_buffer_map_;

  DISALLOW_COPY_AND_ASSIGN(AudioEncoderResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_AUDIO_ENCODER_RESOURCE_H_
