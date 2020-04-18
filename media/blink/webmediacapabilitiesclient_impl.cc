// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webmediacapabilitiesclient_impl.h"

#include <string>
#include <vector>

#include "base/bind_helpers.h"
#include "media/base/audio_codecs.h"
#include "media/base/decode_capabilities.h"
#include "media/base/mime_util.h"
#include "media/base/video_codecs.h"
#include "media/base/video_color_space.h"
#include "media/filters/stream_parser_factory.h"
#include "media/mojo/interfaces/media_types.mojom.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/blink/public/platform/modules/media_capabilities/web_audio_configuration.h"
#include "third_party/blink/public/platform/modules/media_capabilities/web_media_capabilities_info.h"
#include "third_party/blink/public/platform/modules/media_capabilities/web_media_configuration.h"
#include "third_party/blink/public/platform/modules/media_capabilities/web_video_configuration.h"
#include "third_party/blink/public/platform/platform.h"

namespace media {

void BindToHistoryService(mojom::VideoDecodePerfHistoryPtr* history_ptr) {
  DVLOG(2) << __func__;
  blink::Platform* platform = blink::Platform::Current();
  service_manager::Connector* connector = platform->GetConnector();

  connector->BindInterface(platform->GetBrowserServiceName(),
                           mojo::MakeRequest(history_ptr));
}

bool CheckAudioSupport(const blink::WebAudioConfiguration& audio_config) {
  bool audio_supported = false;
  AudioCodec audio_codec = kUnknownAudioCodec;
  bool is_audio_codec_ambiguous = true;

  if (!ParseAudioCodecString(audio_config.mime_type.Ascii(),
                             audio_config.codec.Ascii(),
                             &is_audio_codec_ambiguous, &audio_codec)) {
    // TODO(chcunningham): Replace this and other DVLOGs here with MEDIA_LOG.
    // MediaCapabilities may need its own tab in chrome://media-internals.
    DVLOG(2) << __func__ << " Failed to parse audio contentType: "
             << audio_config.mime_type.Ascii()
             << "; codecs=" << audio_config.codec.Ascii();
    audio_supported = false;
  } else if (is_audio_codec_ambiguous) {
    DVLOG(2) << __func__ << " Invalid (ambiguous) audio codec string:"
             << audio_config.codec.Ascii();
    audio_supported = false;
  } else {
    AudioConfig audio_config = {audio_codec};
    audio_supported = IsSupportedAudioConfig(audio_config);
  }

  return audio_supported;
}

bool CheckVideoSupport(const blink::WebVideoConfiguration& video_config,
                       VideoCodecProfile* out_video_profile) {
  bool video_supported = false;
  VideoCodec video_codec = kUnknownVideoCodec;
  uint8_t video_level = 0;
  VideoColorSpace video_color_space;
  bool is_video_codec_ambiguous = true;

  if (!ParseVideoCodecString(
          video_config.mime_type.Ascii(), video_config.codec.Ascii(),
          &is_video_codec_ambiguous, &video_codec, out_video_profile,
          &video_level, &video_color_space)) {
    DVLOG(2) << __func__ << " Failed to parse video contentType: "
             << video_config.mime_type.Ascii()
             << "; codecs=" << video_config.codec.Ascii();
    video_supported = false;
  } else if (is_video_codec_ambiguous) {
    DVLOG(2) << __func__ << " Invalid (ambiguous) video codec string:"
             << video_config.codec.Ascii();
    video_supported = false;
  } else {
    video_supported = IsSupportedVideoConfig(
        {video_codec, *out_video_profile, video_level, video_color_space});
  }

  return video_supported;
}

bool CheckMseSupport(const blink::WebMediaConfiguration& configuration) {
  DCHECK_EQ(blink::MediaConfigurationType::kMediaSource, configuration.type);

  // For MSE queries, we assume the queried audio and video streams will be
  // placed into separate source buffers.
  // TODO(chcunningham): Clarify this assumption in the spec.

  // Media MIME API expects a vector of codec strings. We query audio and video
  // separately, so |codec_string|.size() should always be 1 or 0 (when no
  // codecs parameter is required for the given mime type).
  std::vector<std::string> codec_vector;

  if (configuration.audio_configuration) {
    const blink::WebAudioConfiguration& audio_config =
        configuration.audio_configuration.value();

    if (!audio_config.codec.Ascii().empty())
      codec_vector.push_back(audio_config.codec.Ascii());

    if (!media::StreamParserFactory::IsTypeSupported(
            audio_config.mime_type.Ascii(), codec_vector)) {
      DVLOG(2) << __func__ << " MSE does not support audio config: "
               << audio_config.mime_type.Ascii() << " "
               << (codec_vector.empty() ? "" : codec_vector[1]);
      return false;
    }
  }

  if (configuration.video_configuration) {
    const blink::WebVideoConfiguration& video_config =
        configuration.video_configuration.value();

    codec_vector.clear();
    if (!video_config.codec.Ascii().empty())
      codec_vector.push_back(video_config.codec.Ascii());

    if (!media::StreamParserFactory::IsTypeSupported(
            video_config.mime_type.Ascii(), codec_vector)) {
      DVLOG(2) << __func__ << " MSE does not support video config: "
               << video_config.mime_type.Ascii() << " "
               << (codec_vector.empty() ? "" : codec_vector[1]);
      return false;
    }
  }

  return true;
}

WebMediaCapabilitiesClientImpl::WebMediaCapabilitiesClientImpl() = default;

WebMediaCapabilitiesClientImpl::~WebMediaCapabilitiesClientImpl() = default;

void VideoPerfInfoCallback(
    std::unique_ptr<blink::WebMediaCapabilitiesQueryCallbacks> callbacks,
    std::unique_ptr<blink::WebMediaCapabilitiesInfo> info,
    bool is_smooth,
    bool is_power_efficient) {
  DCHECK(info->supported);
  info->smooth = is_smooth;
  info->power_efficient = is_power_efficient;
  callbacks->OnSuccess(std::move(info));
}

void WebMediaCapabilitiesClientImpl::DecodingInfo(
    const blink::WebMediaConfiguration& configuration,
    std::unique_ptr<blink::WebMediaCapabilitiesQueryCallbacks> callbacks) {
  std::unique_ptr<blink::WebMediaCapabilitiesInfo> info(
      new blink::WebMediaCapabilitiesInfo());

  // MSE support is cheap to check (regex matching). Do it first.
  if (configuration.type == blink::MediaConfigurationType::kMediaSource &&
      !CheckMseSupport(configuration)) {
    info->supported = info->smooth = info->power_efficient = false;
    callbacks->OnSuccess(std::move(info));
    return;
  }

  bool audio_supported = true;
  if (configuration.audio_configuration)
    audio_supported =
        CheckAudioSupport(configuration.audio_configuration.value());

  // No need to check video capabilities if video not included in configuration
  // or when audio is already known to be unsupported.
  if (!audio_supported || !configuration.video_configuration) {
    // Supported audio-only configurations are always considered smooth and
    // power efficient.
    info->supported = info->smooth = info->power_efficient = audio_supported;
    callbacks->OnSuccess(std::move(info));
    return;
  }

  // Audio is supported and video configuration is provided in the query; all
  // that remains is to check video support and performance.
  DCHECK(audio_supported);
  DCHECK(configuration.video_configuration);
  const blink::WebVideoConfiguration& video_config =
      configuration.video_configuration.value();
  VideoCodecProfile video_profile = VIDEO_CODEC_PROFILE_UNKNOWN;
  bool video_supported = CheckVideoSupport(video_config, &video_profile);

  // Return early for unsupported configurations.
  if (!video_supported) {
    info->supported = info->smooth = info->power_efficient = video_supported;
    callbacks->OnSuccess(std::move(info));
    return;
  }

  // Video is supported! Check its performance history.
  info->supported = true;

  if (!decode_history_ptr_.is_bound())
    BindToHistoryService(&decode_history_ptr_);
  DCHECK(decode_history_ptr_.is_bound());

  mojom::PredictionFeaturesPtr features = mojom::PredictionFeatures::New(
      video_profile, gfx::Size(video_config.width, video_config.height),
      video_config.framerate);

  decode_history_ptr_->GetPerfInfo(
      std::move(features),
      base::BindOnce(&VideoPerfInfoCallback, std::move(callbacks),
                     std::move(info)));
}

}  // namespace media
