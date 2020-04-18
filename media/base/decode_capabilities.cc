// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/decode_capabilities.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "media/base/media_switches.h"
#include "media/media_buildflags.h"
#include "third_party/libaom/av1_buildflags.h"
#include "ui/display/display_switches.h"

#if BUILDFLAG(ENABLE_LIBVPX)
#include "third_party/libvpx/source/libvpx/vpx/vp8dx.h"
#include "third_party/libvpx/source/libvpx/vpx/vpx_codec.h"
#endif

namespace media {

bool IsColorSpaceSupported(const media::VideoColorSpace& color_space) {
  switch (color_space.primaries) {
    case media::VideoColorSpace::PrimaryID::EBU_3213_E:
    case media::VideoColorSpace::PrimaryID::INVALID:
      return false;

    // Transfers supported before color management.
    case media::VideoColorSpace::PrimaryID::BT709:
    case media::VideoColorSpace::PrimaryID::UNSPECIFIED:
    case media::VideoColorSpace::PrimaryID::BT470M:
    case media::VideoColorSpace::PrimaryID::BT470BG:
    case media::VideoColorSpace::PrimaryID::SMPTE170M:
      break;

    // Supported with color management.
    case media::VideoColorSpace::PrimaryID::SMPTE240M:
    case media::VideoColorSpace::PrimaryID::FILM:
    case media::VideoColorSpace::PrimaryID::BT2020:
    case media::VideoColorSpace::PrimaryID::SMPTEST428_1:
    case media::VideoColorSpace::PrimaryID::SMPTEST431_2:
    case media::VideoColorSpace::PrimaryID::SMPTEST432_1:
      break;
  }

  switch (color_space.transfer) {
    // Transfers supported before color management.
    case media::VideoColorSpace::TransferID::UNSPECIFIED:
    case media::VideoColorSpace::TransferID::GAMMA22:
    case media::VideoColorSpace::TransferID::BT709:
    case media::VideoColorSpace::TransferID::SMPTE170M:
    case media::VideoColorSpace::TransferID::BT2020_10:
    case media::VideoColorSpace::TransferID::BT2020_12:
    case media::VideoColorSpace::TransferID::IEC61966_2_1:
      break;

    // Supported with color management.
    case media::VideoColorSpace::TransferID::GAMMA28:
    case media::VideoColorSpace::TransferID::SMPTE240M:
    case media::VideoColorSpace::TransferID::LINEAR:
    case media::VideoColorSpace::TransferID::LOG:
    case media::VideoColorSpace::TransferID::LOG_SQRT:
    case media::VideoColorSpace::TransferID::BT1361_ECG:
    case media::VideoColorSpace::TransferID::SMPTEST2084:
    case media::VideoColorSpace::TransferID::IEC61966_2_4:
    case media::VideoColorSpace::TransferID::SMPTEST428_1:
    case media::VideoColorSpace::TransferID::ARIB_STD_B67:
      break;

    // Never supported.
    case media::VideoColorSpace::TransferID::INVALID:
      return false;
  }

  switch (color_space.matrix) {
    // Supported before color management.
    case media::VideoColorSpace::MatrixID::BT709:
    case media::VideoColorSpace::MatrixID::UNSPECIFIED:
    case media::VideoColorSpace::MatrixID::BT470BG:
    case media::VideoColorSpace::MatrixID::SMPTE170M:
    case media::VideoColorSpace::MatrixID::BT2020_NCL:
      break;

    // Supported with color management.
    case media::VideoColorSpace::MatrixID::RGB:
    case media::VideoColorSpace::MatrixID::FCC:
    case media::VideoColorSpace::MatrixID::SMPTE240M:
    case media::VideoColorSpace::MatrixID::YCOCG:
    case media::VideoColorSpace::MatrixID::YDZDX:
    case media::VideoColorSpace::MatrixID::BT2020_CL:
      break;

    // Never supported.
    case media::VideoColorSpace::MatrixID::INVALID:
      return false;
  }

  if (color_space.range == gfx::ColorSpace::RangeID::INVALID)
    return false;

  return true;
}

bool IsVp9ProfileSupported(VideoCodecProfile profile) {
#if BUILDFLAG(ENABLE_LIBVPX)
  // High bit depth capabilities may be toggled via LibVPX config flags.
  static bool vpx_supports_high_bit_depth =
      (vpx_codec_get_caps(vpx_codec_vp9_dx()) & VPX_CODEC_CAP_HIGHBITDEPTH) !=
      0;

  switch (profile) {
    // LibVPX always supports Profiles 0 and 1.
    case VP9PROFILE_PROFILE0:
    case VP9PROFILE_PROFILE1:
      return true;
    case VP9PROFILE_PROFILE2:
    case VP9PROFILE_PROFILE3:
      return vpx_supports_high_bit_depth;
    default:
      NOTREACHED();
  }
#endif
  return false;
}

bool IsSupportedAudioConfig(const AudioConfig& config) {
  switch (config.codec) {
    case media::kCodecAAC:
    case media::kCodecFLAC:
    case media::kCodecMP3:
    case media::kCodecOpus:
    case media::kCodecPCM:
    case media::kCodecPCM_MULAW:
    case media::kCodecPCM_S16BE:
    case media::kCodecPCM_S24BE:
    case media::kCodecPCM_ALAW:
    case media::kCodecVorbis:
      return true;

    case media::kCodecAMR_NB:
    case media::kCodecAMR_WB:
    case media::kCodecGSM_MS:
#if defined(OS_CHROMEOS)
      return true;
#else
      return false;
#endif

    case media::kCodecEAC3:
    case media::kCodecALAC:
    case media::kCodecAC3:
    case media::kCodecMpegHAudio:
    case media::kUnknownAudioCodec:
      return false;
  }

  NOTREACHED();
  return false;
}

// TODO(chcunningham): Query decoders for codec profile support. Add platform
// specific logic for Android (move from MimeUtilIntenral).
bool IsSupportedVideoConfig(const VideoConfig& config) {
  switch (config.codec) {
    case media::kCodecAV1:
#if BUILDFLAG(ENABLE_AV1_DECODER)
      return base::FeatureList::IsEnabled(kAv1Decoder) &&
             IsColorSpaceSupported(config.color_space);
#else
      return false;
#endif

    case media::kCodecVP9:
      // Color management required for HDR to not look terrible.
      return IsColorSpaceSupported(config.color_space) &&
             IsVp9ProfileSupported(config.profile);
    case media::kCodecH264:
    case media::kCodecVP8:
    case media::kCodecTheora:
      return true;

    case media::kUnknownVideoCodec:
    case media::kCodecVC1:
    case media::kCodecMPEG2:
    case media::kCodecMPEG4:
    case media::kCodecHEVC:
    case media::kCodecDolbyVision:
      return false;
  }

  NOTREACHED();
  return false;
}

}  // namespace media
