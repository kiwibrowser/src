// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mirroring/service/mirror_settings.h"

#include <algorithm>

using media::cast::FrameSenderConfig;
using media::cast::Codec;
using media::cast::RtpPayloadType;
using media::ResolutionChangePolicy;

namespace mirroring {

namespace {

// Starting end-to-end latency for animated content.
constexpr base::TimeDelta kAnimatedPlayoutDelay =
    base::TimeDelta::FromMilliseconds(400);

// Minimum end-to-end latency. This allows cast streaming to adaptively lower
// latency in interactive streaming scenarios.
// TODO(miu): This was 120 before stable launch, but we got user feedback that
// this was causing audio drop-outs. So, we need to fix the Cast Streaming
// implementation before lowering this setting.
constexpr base::TimeDelta kMinPlayoutDelay =
    base::TimeDelta::FromMilliseconds(400);

// Maximum end-to-end latency.
constexpr base::TimeDelta kMaxPlayoutDelay =
    base::TimeDelta::FromMilliseconds(800);

constexpr int kAudioTimebase = 48000;
constexpr int kVidoTimebase = 90000;
constexpr int kAudioChannels = 2;
constexpr int kAudioFramerate = 100;  // 100 FPS for 10ms packets.
constexpr int kMinVideoBitrate = 300000;
constexpr int kMaxVideoBitrate = 5000000;
constexpr int kAudioBitrate = 0;   // 0 means automatic.
constexpr int kMaxFrameRate = 30;  // The maximum frame rate for captures.
constexpr int kMaxWidth = 1920;    // Maximum video width in pixels.
constexpr int kMaxHeight = 1080;   // Maximum video height in pixels.
constexpr int kMinWidth = 180;     // Minimum video frame width in pixels.
constexpr int kMinHeight = 180;    // Minimum video frame height in pixels.

}  // namespace

MirrorSettings::MirrorSettings()
    : min_width_(kMinWidth),
      min_height_(kMinHeight),
      max_width_(kMaxWidth),
      max_height_(kMaxHeight) {}

MirrorSettings::~MirrorSettings() {}

// static
FrameSenderConfig MirrorSettings::GetDefaultAudioConfig(
    RtpPayloadType payload_type,
    Codec codec) {
  FrameSenderConfig config;
  config.sender_ssrc = 1;
  config.receiver_ssrc = 2;
  config.min_playout_delay = kMinPlayoutDelay;
  config.max_playout_delay = kMaxPlayoutDelay;
  config.animated_playout_delay = kAnimatedPlayoutDelay;
  config.rtp_payload_type = payload_type;
  config.rtp_timebase = kAudioTimebase;
  config.channels = kAudioChannels;
  config.min_bitrate = config.max_bitrate = config.start_bitrate =
      kAudioBitrate;
  config.max_frame_rate = kAudioFramerate;  // 10 ms audio frames
  config.codec = codec;
  return config;
}

// static
FrameSenderConfig MirrorSettings::GetDefaultVideoConfig(
    RtpPayloadType payload_type,
    Codec codec) {
  FrameSenderConfig config;
  config.sender_ssrc = 11;
  config.receiver_ssrc = 12;
  config.min_playout_delay = kMinPlayoutDelay;
  config.max_playout_delay = kMaxPlayoutDelay;
  config.animated_playout_delay = kAnimatedPlayoutDelay;
  config.rtp_payload_type = payload_type;
  config.rtp_timebase = kVidoTimebase;
  config.channels = 1;
  config.min_bitrate = kMinVideoBitrate;
  config.max_bitrate = kMaxVideoBitrate;
  config.start_bitrate = kMinVideoBitrate;
  config.max_frame_rate = kMaxFrameRate;
  config.codec = codec;
  return config;
}

void MirrorSettings::SetResolutionContraints(int max_width, int max_height) {
  max_width_ = std::max(max_width, min_width_);
  max_height_ = std::max(max_height, min_height_);
}

media::VideoCaptureParams MirrorSettings::GetVideoCaptureParams() {
  media::VideoCaptureParams params;
  params.requested_format =
      media::VideoCaptureFormat(gfx::Size(max_width_, max_height_),
                                kMaxFrameRate, media::PIXEL_FORMAT_I420);
  if (max_height_ == min_height_ && max_width_ == min_width_) {
    params.resolution_change_policy = ResolutionChangePolicy::FIXED_RESOLUTION;
  } else if ((100 * min_width_ / min_height_) ==
             (100 * max_width_ / max_height_)) {
    params.resolution_change_policy =
        ResolutionChangePolicy::FIXED_ASPECT_RATIO;
  } else {
    params.resolution_change_policy = ResolutionChangePolicy::ANY_WITHIN_LIMIT;
  }
  DCHECK(params.IsValid());
  return params;
}

base::Value MirrorSettings::ToDictionaryValue() {
  base::Value settings(base::Value::Type::DICTIONARY);
  settings.SetKey("maxWidth", base::Value(max_width_));
  settings.SetKey("maxHeight", base::Value(max_height_));
  settings.SetKey("minWidth", base::Value(min_width_));
  settings.SetKey("minHeight", base::Value(min_height_));
  settings.SetKey("senderSideLetterboxing", base::Value(true));
  settings.SetKey("minFrameRate", base::Value(0));
  settings.SetKey("maxFrameRate", base::Value(kMaxFrameRate));
  settings.SetKey("minVideoBitrate", base::Value(kMinVideoBitrate));
  settings.SetKey("maxVideoBitrate", base::Value(kMaxVideoBitrate));
  settings.SetKey("audioBitrate", base::Value(kAudioBitrate));
  settings.SetKey(
      "maxLatencyMillis",
      base::Value(static_cast<int32_t>(kMaxPlayoutDelay.InMilliseconds())));
  settings.SetKey(
      "minLatencyMillis",
      base::Value(static_cast<int32_t>(kMinPlayoutDelay.InMilliseconds())));
  settings.SetKey("animatedLatencyMillis",
                  base::Value(static_cast<int32_t>(
                      kAnimatedPlayoutDelay.InMilliseconds())));
  settings.SetKey("dscpEnabled", base::Value(false));
  settings.SetKey("enableLogging", base::Value(true));
  settings.SetKey("useTdls", base::Value(false));
  return settings;
}

}  // namespace mirroring
