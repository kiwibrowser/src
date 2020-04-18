// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_DECODER_STREAM_TRAITS_H_
#define MEDIA_FILTERS_DECODER_STREAM_TRAITS_H_

#include "base/containers/flat_set.h"
#include "base/time/time.h"
#include "media/base/audio_decoder.h"
#include "media/base/cdm_context.h"
#include "media/base/channel_layout.h"
#include "media/base/demuxer_stream.h"
#include "media/base/moving_average.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/filters/audio_timestamp_validator.h"

namespace media {

class AudioBuffer;
class AudioDecoderConfig;
class CdmContext;
class DemuxerStream;
class VideoDecoderConfig;
class VideoFrame;

template <DemuxerStream::Type StreamType>
class DecoderStreamTraits {};

enum class PostDecodeAction { DELIVER, DROP };

template <>
class MEDIA_EXPORT DecoderStreamTraits<DemuxerStream::AUDIO> {
 public:
  using OutputType = AudioBuffer;
  using DecoderType = AudioDecoder;
  using DecoderConfigType = AudioDecoderConfig;
  using InitCB = AudioDecoder::InitCB;
  using OutputCB = AudioDecoder::OutputCB;
  using WaitingForDecryptionKeyCB = AudioDecoder::WaitingForDecryptionKeyCB;

  static std::string ToString();
  static bool NeedsBitstreamConversion(DecoderType* decoder);
  static scoped_refptr<OutputType> CreateEOSOutput();

  DecoderStreamTraits(MediaLog* media_log, ChannelLayout initial_hw_layout);

  void ReportStatistics(const StatisticsCB& statistics_cb, int bytes_decoded);
  void InitializeDecoder(
      DecoderType* decoder,
      const DecoderConfigType& config,
      bool low_delay,
      CdmContext* cdm_context,
      const InitCB& init_cb,
      const OutputCB& output_cb,
      const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb);
  DecoderConfigType GetDecoderConfig(DemuxerStream* stream);
  void OnDecode(const DecoderBuffer& buffer);
  PostDecodeAction OnDecodeDone(const scoped_refptr<OutputType>& buffer);
  void OnStreamReset(DemuxerStream* stream);
  void OnConfigChanged(const DecoderConfigType& config);

 private:
  // Validates encoded timestamps match decoded output duration. MEDIA_LOG warns
  // if timestamp gaps are detected. Sufficiently large gaps can lead to AV sync
  // drift.
  std::unique_ptr<AudioTimestampValidator> audio_ts_validator_;
  MediaLog* media_log_;
  // HW layout at the time pipeline was started. Will not reflect possible
  // device changes.
  ChannelLayout initial_hw_layout_;
  PipelineStatistics stats_;
};

template <>
class MEDIA_EXPORT DecoderStreamTraits<DemuxerStream::VIDEO> {
 public:
  using OutputType = VideoFrame;
  using DecoderType = VideoDecoder;
  using DecoderConfigType = VideoDecoderConfig;
  using InitCB = VideoDecoder::InitCB;
  using OutputCB = VideoDecoder::OutputCB;
  using WaitingForDecryptionKeyCB = VideoDecoder::WaitingForDecryptionKeyCB;

  static std::string ToString();
  static bool NeedsBitstreamConversion(DecoderType* decoder);
  static scoped_refptr<OutputType> CreateEOSOutput();

  explicit DecoderStreamTraits(MediaLog* media_log);

  DecoderConfigType GetDecoderConfig(DemuxerStream* stream);
  void ReportStatistics(const StatisticsCB& statistics_cb, int bytes_decoded);
  void InitializeDecoder(
      DecoderType* decoder,
      const DecoderConfigType& config,
      bool low_delay,
      CdmContext* cdm_context,
      const InitCB& init_cb,
      const OutputCB& output_cb,
      const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb);
  void OnDecode(const DecoderBuffer& buffer);
  PostDecodeAction OnDecodeDone(const scoped_refptr<OutputType>& buffer);
  void OnStreamReset(DemuxerStream* stream);
  void OnConfigChanged(const DecoderConfigType& config) {}

 private:
  base::TimeDelta last_keyframe_timestamp_;
  MovingAverage keyframe_distance_average_;
  base::flat_set<base::TimeDelta> frames_to_drop_;
  PipelineStatistics stats_;
};

}  // namespace media

#endif  // MEDIA_FILTERS_DECODER_STREAM_TRAITS_H_
