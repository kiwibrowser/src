// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_REMOTING_PROTO_UTILS_H_
#define MEDIA_REMOTING_PROTO_UTILS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/cdm_config.h"
#include "media/base/cdm_key_information.h"
#include "media/base/cdm_promise.h"
#include "media/base/content_decryption_module.h"
#include "media/base/decoder_buffer.h"
#include "media/base/demuxer_stream.h"
#include "media/base/eme_constants.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder_config.h"
#include "media/remoting/rpc.pb.h"

namespace media {
namespace remoting {

class CdmPromiseResult;

// Utility class to convert data between media::DecoderBuffer and byte array.
// It is to serialize media::DecoderBuffer structure except for actual data
// into pb::DecoderBuffer followed by byte array of decoder buffer. The reason
// data is not part of proto buffer because it would cost unnecessary time to
// wait for whole proto received before conversion given the fact that decoder
// buffer data can vary from hundred bytes to 3~5MB. Also, it would costs extra
// CPU to sirealize/de-serialize decoder buffer which is encoded and encrypted
// as wire format for data transmission.
//
// DecoderBufferSegment {
//  // Payload version. Default value is 0.
//  u8 payload_version;
//
//  // Length of pb::DecoderBuffer (protobuf-encoded of media::DecoderBuffer
//                   except for data).
//  u16 buffer_segment_size;
//  // pb::DecoderBuffer.
//  u8[buffer_segment_size] buffer_segment;
//
//  // Length of data in media::DecoderBuffer.
//  u32 data_buffer_size;
//  // media::DecoderBuffer data.
//  u8[data_buffer_size] data_buffer;
//};

// Converts DecoderBufferSegment into byte array.
std::vector<uint8_t> DecoderBufferToByteArray(
    const DecoderBuffer& decoder_buffer);

// Converts byte array into DecoderBufferSegment.
scoped_refptr<DecoderBuffer> ByteArrayToDecoderBuffer(const uint8_t* data,
                                                      uint32_t size);

// Data type conversion between media::AudioDecoderConfig and proto buffer.
void ConvertAudioDecoderConfigToProto(const AudioDecoderConfig& audio_config,
                                      pb::AudioDecoderConfig* audio_message);
bool ConvertProtoToAudioDecoderConfig(
    const pb::AudioDecoderConfig& audio_message,
    AudioDecoderConfig* audio_config);

// Data type conversion between media::VideoDecoderConfig and proto buffer.
void ConvertVideoDecoderConfigToProto(const VideoDecoderConfig& video_config,
                                      pb::VideoDecoderConfig* video_message);
bool ConvertProtoToVideoDecoderConfig(
    const pb::VideoDecoderConfig& video_message,
    VideoDecoderConfig* video_config);

// Data type conversion between media::VideoDecoderConfig and proto buffer.
void ConvertProtoToPipelineStatistics(
    const pb::PipelineStatistics& stats_message,
    PipelineStatistics* stats);

// Data type conversion between media::CdmKeysInfo and proto buffer.
void ConvertCdmKeyInfoToProto(
    const CdmKeysInfo& keys_information,
    pb::CdmClientOnSessionKeysChange* key_change_message);
void ConvertProtoToCdmKeyInfo(
    const pb::CdmClientOnSessionKeysChange keychange_message,
    CdmKeysInfo* key_information);

// Data type conversion between CdmPromiseResult and proto buffer.
void ConvertCdmPromiseToProto(const CdmPromiseResult& result,
                              pb::CdmPromise* promise_message);
void ConvertCdmPromiseWithSessionIdToProto(const CdmPromiseResult& result,
                                           const std::string& session_id,
                                           pb::CdmPromise* promise_message);
void ConvertCdmPromiseWithCdmIdToProto(const CdmPromiseResult& result,
                                       int cdm_id,
                                       pb::CdmPromise* promise_message);
bool ConvertProtoToCdmPromise(const pb::CdmPromise& promise_message,
                              CdmPromiseResult* result);
bool ConvertProtoToCdmPromiseWithCdmIdSessionId(const pb::RpcMessage& message,
                                                CdmPromiseResult* result,
                                                int* cdm_id,
                                                std::string* session_id);

//==================================================================
class CdmPromiseResult {
 public:
  CdmPromiseResult();
  CdmPromiseResult(CdmPromise::Exception exception,
                   uint32_t system_code,
                   std::string error_message);
  CdmPromiseResult(const CdmPromiseResult& other);
  ~CdmPromiseResult();

  static CdmPromiseResult SuccessResult();

  bool success() const { return success_; }
  CdmPromise::Exception exception() const { return exception_; }
  uint32_t system_code() const { return system_code_; }
  const std::string& error_message() const { return error_message_; }

 private:
  bool success_;
  CdmPromise::Exception exception_;
  uint32_t system_code_;
  std::string error_message_;
};

}  // namespace remoting
}  // namespace media

#endif  // MEDIA_REMOTING_PROTO_UTILS_H_
