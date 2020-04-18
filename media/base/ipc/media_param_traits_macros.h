// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_IPC_MEDIA_PARAM_TRAITS_MACROS_H_
#define MEDIA_BASE_IPC_MEDIA_PARAM_TRAITS_MACROS_H_

#include "ipc/ipc_message_macros.h"
#include "media/base/audio_codecs.h"
#include "media/base/audio_parameters.h"
#include "media/base/buffering_state.h"
#include "media/base/cdm_config.h"
#include "media/base/cdm_key_information.h"
#include "media/base/cdm_promise.h"
#include "media/base/channel_layout.h"
#include "media/base/content_decryption_module.h"
#include "media/base/decode_status.h"
#include "media/base/decrypt_config.h"
#include "media/base/decryptor.h"
#include "media/base/demuxer_stream.h"
#include "media/base/eme_constants.h"
#include "media/base/encryption_scheme.h"
#include "media/base/hdr_metadata.h"
#include "media/base/media_log_event.h"
#include "media/base/output_device_info.h"
#include "media/base/overlay_info.h"
#include "media/base/pipeline_status.h"
#include "media/base/sample_format.h"
#include "media/base/subsample_entry.h"
#include "media/base/video_codecs.h"
#include "media/base/video_color_space.h"
#include "media/base/video_rotation.h"
#include "media/base/video_types.h"
#include "media/base/watch_time_keys.h"
// TODO(crbug.com/676224): When EnabledIf attribute is supported in mojom files,
// move CdmProxy related code into #if BUILDFLAG(ENABLE_LIBRARY_CDMS).
#include "media/cdm/cdm_proxy.h"
#include "media/media_buildflags.h"
#include "ui/gfx/ipc/color/gfx_param_traits_macros.h"

// Enum traits.

IPC_ENUM_TRAITS_MAX_VALUE(media::AudioCodec, media::AudioCodec::kAudioCodecMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::AudioLatency::LatencyType,
                          media::AudioLatency::LATENCY_COUNT)

IPC_ENUM_TRAITS_MAX_VALUE(media::AudioParameters::Format,
                          media::AudioParameters::AUDIO_FORMAT_LAST)

IPC_ENUM_TRAITS_MAX_VALUE(media::BufferingState,
                          media::BufferingState::BUFFERING_STATE_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::CdmKeyInformation::KeyStatus,
                          media::CdmKeyInformation::KEY_STATUS_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::CdmMessageType,
                          media::CdmMessageType::MESSAGE_TYPE_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::CdmPromise::Exception,
                          media::CdmPromise::Exception::EXCEPTION_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::CdmProxy::Function,
                          media::CdmProxy::Function::kMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::CdmProxy::Protocol,
                          media::CdmProxy::Protocol::kMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::CdmProxy::Status,
                          media::CdmProxy::Status::kMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::CdmSessionType,
                          media::CdmSessionType::SESSION_TYPE_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::ChannelLayout, media::CHANNEL_LAYOUT_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::ColorSpace, media::COLOR_SPACE_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::DecodeStatus,
                          media::DecodeStatus::DECODE_STATUS_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::Decryptor::Status,
                          media::Decryptor::Status::kStatusMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::Decryptor::StreamType,
                          media::Decryptor::StreamType::kStreamTypeMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::DemuxerStream::Status,
                          media::DemuxerStream::kStatusMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::DemuxerStream::Type,
                          media::DemuxerStream::TYPE_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::EmeInitDataType, media::EmeInitDataType::MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::EncryptionMode,
                          media::EncryptionMode::kMaxValue)

IPC_ENUM_TRAITS_MAX_VALUE(media::EncryptionScheme::CipherMode,
                          media::EncryptionScheme::CipherMode::CIPHER_MODE_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::HdcpVersion,
                          media::HdcpVersion::kHdcpVersionMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::MediaLogEvent::Type,
                          media::MediaLogEvent::TYPE_LAST)

IPC_ENUM_TRAITS_MAX_VALUE(media::OutputDeviceStatus,
                          media::OUTPUT_DEVICE_STATUS_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::PipelineStatus,
                          media::PipelineStatus::PIPELINE_STATUS_MAX);

IPC_ENUM_TRAITS_MAX_VALUE(media::SampleFormat, media::kSampleFormatMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::VideoCodec, media::kVideoCodecMax)

IPC_ENUM_TRAITS_MAX_VALUE(media::WatchTimeKey,
                          media::WatchTimeKey::kWatchTimeKeyMax);

IPC_ENUM_TRAITS_MIN_MAX_VALUE(media::VideoCodecProfile,
                              media::VIDEO_CODEC_PROFILE_MIN,
                              media::VIDEO_CODEC_PROFILE_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::VideoPixelFormat, media::PIXEL_FORMAT_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(media::VideoRotation, media::VIDEO_ROTATION_MAX)

IPC_ENUM_TRAITS_VALIDATE(
    media::VideoColorSpace::PrimaryID,
    static_cast<int>(value) ==
        static_cast<int>(
            media::VideoColorSpace::GetPrimaryID(static_cast<int>(value))));

IPC_ENUM_TRAITS_VALIDATE(
    media::VideoColorSpace::TransferID,
    static_cast<int>(value) ==
        static_cast<int>(
            media::VideoColorSpace::GetTransferID(static_cast<int>(value))));

IPC_ENUM_TRAITS_VALIDATE(
    media::VideoColorSpace::MatrixID,
    static_cast<int>(value) ==
        static_cast<int>(
            media::VideoColorSpace::GetMatrixID(static_cast<int>(value))));

// Struct traits.

IPC_STRUCT_TRAITS_BEGIN(media::CdmConfig)
  IPC_STRUCT_TRAITS_MEMBER(allow_distinctive_identifier)
  IPC_STRUCT_TRAITS_MEMBER(allow_persistent_state)
  IPC_STRUCT_TRAITS_MEMBER(use_hw_secure_codecs)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(media::CdmKeyInformation)
  IPC_STRUCT_TRAITS_MEMBER(key_id)
  IPC_STRUCT_TRAITS_MEMBER(status)
  IPC_STRUCT_TRAITS_MEMBER(system_code)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(media::MediaLogEvent)
  IPC_STRUCT_TRAITS_MEMBER(id)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(params)
  IPC_STRUCT_TRAITS_MEMBER(time)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(media::SubsampleEntry)
  IPC_STRUCT_TRAITS_MEMBER(clear_bytes)
  IPC_STRUCT_TRAITS_MEMBER(cypher_bytes)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(media::VideoColorSpace)
  IPC_STRUCT_TRAITS_MEMBER(primaries)
  IPC_STRUCT_TRAITS_MEMBER(transfer)
  IPC_STRUCT_TRAITS_MEMBER(matrix)
  IPC_STRUCT_TRAITS_MEMBER(range)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(media::MasteringMetadata)
  IPC_STRUCT_TRAITS_MEMBER(primary_r)
  IPC_STRUCT_TRAITS_MEMBER(primary_g)
  IPC_STRUCT_TRAITS_MEMBER(primary_b)
  IPC_STRUCT_TRAITS_MEMBER(white_point)
  IPC_STRUCT_TRAITS_MEMBER(luminance_max)
  IPC_STRUCT_TRAITS_MEMBER(luminance_min)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(media::HDRMetadata)
  IPC_STRUCT_TRAITS_MEMBER(mastering_metadata)
  IPC_STRUCT_TRAITS_MEMBER(max_content_light_level)
  IPC_STRUCT_TRAITS_MEMBER(max_frame_average_light_level)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(media::OverlayInfo)
  IPC_STRUCT_TRAITS_MEMBER(surface_id)
  IPC_STRUCT_TRAITS_MEMBER(routing_token)
  IPC_STRUCT_TRAITS_MEMBER(is_fullscreen)
IPC_STRUCT_TRAITS_END()

#endif  // MEDIA_BASE_IPC_MEDIA_PARAM_TRAITS_MACROS_H_
