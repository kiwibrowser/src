// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/formats/webm/webm_colour_parser.h"

#include "base/logging.h"
#include "media/formats/webm/webm_constants.h"

namespace media {

// The definitions below are copied from the current libwebm top-of-tree.
// Chromium's third_party/libwebm doesn't have these yet. We can probably just
// include libwebm header directly in the future.
// ---- Begin copy/paste from libwebm/webm_parser/include/webm/dom_types.h ----

/**
 A parsed \WebMID{MatrixCoefficients} element.

 Matroska/WebM adopted these values from Table 4 of ISO/IEC 23001-8:2013/DCOR1.
 See that document for further details.
 */
enum class MatrixCoefficients : std::uint64_t {
  /**
   The identity matrix.

   Typically used for GBR (often referred to as RGB); however, may also be used
   for YZX (often referred to as XYZ).
   */
  kRgb = 0,

  /**
   Rec. ITU-R BT.709-5.
   */
  kBt709 = 1,

  /**
   Image characteristics are unknown or are determined by the application.
   */
  kUnspecified = 2,

  /**
   United States Federal Communications Commission Title 47 Code of Federal
   Regulations (2003) 73.682 (a) (20).
   */
  kFcc = 4,

  /**
   Rec. ITU-R BT.470‑6 System B, G (historical).
   */
  kBt470Bg = 5,

  /**
   Society of Motion Picture and Television Engineers 170M (2004).
   */
  kSmpte170M = 6,

  /**
   Society of Motion Picture and Television Engineers 240M (1999).
   */
  kSmpte240M = 7,

  /**
   YCgCo.
   */
  kYCgCo = 8,

  /**
   Rec. ITU-R BT.2020 (non-constant luminance).
   */
  kBt2020NonconstantLuminance = 9,

  /**
   Rec. ITU-R BT.2020 (constant luminance).
   */
  kBt2020ConstantLuminance = 10,
};

/**
 A parsed \WebMID{Range} element.
 */
enum class Range : std::uint64_t {
  /**
   Unspecified.
   */
  kUnspecified = 0,

  /**
   Broadcast range.
   */
  kBroadcast = 1,

  /**
   Full range (no clipping).
   */
  kFull = 2,

  /**
   Defined by MatrixCoefficients/TransferCharacteristics.
   */
  kDerived = 3,

  kMinValue = kUnspecified,
  kMaxValue = kDerived,
};

/**
 A parsed \WebMID{TransferCharacteristics} element.

 Matroska/WebM adopted these values from Table 3 of ISO/IEC 23001-8:2013/DCOR1.
 See that document for further details.
 */
enum class TransferCharacteristics : std::uint64_t {
  /**
   Rec. ITU-R BT.709-6.
   */
  kBt709 = 1,

  /**
   Image characteristics are unknown or are determined by the application.
   */
  kUnspecified = 2,

  /**
   Rec. ITU‑R BT.470‑6 System M (historical) with assumed display gamma 2.2.
   */
  kGamma22curve = 4,

  /**
   Rec. ITU‑R BT.470-6 System B, G (historical) with assumed display gamma 2.8.
   */
  kGamma28curve = 5,

  /**
   Society of Motion Picture and Television Engineers 170M (2004).
   */
  kSmpte170M = 6,

  /**
   Society of Motion Picture and Television Engineers 240M (1999).
   */
  kSmpte240M = 7,

  /**
   Linear transfer characteristics.
   */
  kLinear = 8,

  /**
   Logarithmic transfer characteristic (100:1 range).
   */
  kLog = 9,

  /**
   Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range).
   */
  kLogSqrt = 10,

  /**
   IEC 61966-2-4.
   */
  kIec6196624 = 11,

  /**
   Rec. ITU‑R BT.1361-0 extended colour gamut system (historical).
   */
  kBt1361ExtendedColourGamut = 12,

  /**
   IEC 61966-2-1 sRGB or sYCC.
   */
  kIec6196621 = 13,

  /**
   Rec. ITU-R BT.2020-2 (10-bit system).
   */
  k10BitBt2020 = 14,

  /**
   Rec. ITU-R BT.2020-2 (12-bit system).
   */
  k12BitBt2020 = 15,

  /**
   Society of Motion Picture and Television Engineers ST 2084.
   */
  kSmpteSt2084 = 16,

  /**
   Society of Motion Picture and Television Engineers ST 428-1.
   */
  kSmpteSt4281 = 17,

  /**
   Association of Radio Industries and Businesses (ARIB) STD-B67.
   */
  kAribStdB67Hlg = 18,
};

/**
 A parsed \WebMID{Primaries} element.

 Matroska/WebM adopted these values from Table 2 of ISO/IEC 23001-8:2013/DCOR1.
 See that document for further details.
 */
enum class Primaries : std::uint64_t {
  /**
   Rec. ITU‑R BT.709-6.
   */
  kBt709 = 1,

  /**
   Image characteristics are unknown or are determined by the application.
   */
  kUnspecified = 2,

  /**
   Rec. ITU‑R BT.470‑6 System M (historical).
   */
  kBt470M = 4,

  /**
   Rec. ITU‑R BT.470‑6 System B, G (historical).
   */
  kBt470Bg = 5,

  /**
   Society of Motion Picture and Television Engineers 170M (2004).
   */
  kSmpte170M = 6,

  /**
   Society of Motion Picture and Television Engineers 240M (1999).
   */
  kSmpte240M = 7,

  /**
   Generic film.
   */
  kFilm = 8,

  /**
   Rec. ITU-R BT.2020-2.
   */
  kBt2020 = 9,

  /**
   Society of Motion Picture and Television Engineers ST 428-1.
   */
  kSmpteSt4281 = 10,

  /**
   JEDEC P22 phosphors/EBU Tech. 3213-E (1975).
   */
  kJedecP22Phosphors = 22,
};

// ---- End copy/paste from libwebm/webm_parser/include/webm/dom_types.h ----

WebMColorMetadata::WebMColorMetadata() = default;
WebMColorMetadata::WebMColorMetadata(const WebMColorMetadata& rhs) = default;

WebMMasteringMetadataParser::WebMMasteringMetadataParser() = default;
WebMMasteringMetadataParser::~WebMMasteringMetadataParser() = default;

bool WebMMasteringMetadataParser::OnFloat(int id, double val) {
  switch (id) {
    case kWebMIdPrimaryRChromaticityX:
      mastering_metadata_.primary_r.set_x(val);
      break;
    case kWebMIdPrimaryRChromaticityY:
      mastering_metadata_.primary_r.set_y(val);
      break;
    case kWebMIdPrimaryGChromaticityX:
      mastering_metadata_.primary_g.set_x(val);
      break;
    case kWebMIdPrimaryGChromaticityY:
      mastering_metadata_.primary_g.set_y(val);
      break;
    case kWebMIdPrimaryBChromaticityX:
      mastering_metadata_.primary_b.set_x(val);
      break;
    case kWebMIdPrimaryBChromaticityY:
      mastering_metadata_.primary_b.set_y(val);
      break;
    case kWebMIdWhitePointChromaticityX:
      mastering_metadata_.white_point.set_x(val);
      break;
    case kWebMIdWhitePointChromaticityY:
      mastering_metadata_.white_point.set_y(val);
      break;
    case kWebMIdLuminanceMax:
      mastering_metadata_.luminance_max = val;
      break;
    case kWebMIdLuminanceMin:
      mastering_metadata_.luminance_min = val;
      break;
    default:
      DVLOG(1) << "Unexpected id in MasteringMetadata: 0x" << std::hex << id;
      return false;
  }
  return true;
}

WebMColourParser::WebMColourParser() {
  Reset();
}

WebMColourParser::~WebMColourParser() = default;

void WebMColourParser::Reset() {
  matrix_coefficients_ = -1;
  bits_per_channel_ = -1;
  chroma_subsampling_horz_ = -1;
  chroma_subsampling_vert_ = -1;
  cb_subsampling_horz_ = -1;
  cb_subsampling_vert_ = -1;
  chroma_siting_horz_ = -1;
  chroma_siting_vert_ = -1;
  range_ = -1;
  transfer_characteristics_ = -1;
  primaries_ = -1;
  max_content_light_level_ = -1;
  max_frame_average_light_level_ = -1;
}

WebMParserClient* WebMColourParser::OnListStart(int id) {
  if (id == kWebMIdMasteringMetadata) {
    mastering_metadata_parsed_ = false;
    return &mastering_metadata_parser_;
  }

  return this;
}

bool WebMColourParser::OnListEnd(int id) {
  if (id == kWebMIdMasteringMetadata)
    mastering_metadata_parsed_ = true;
  return true;
}

bool WebMColourParser::OnUInt(int id, int64_t val) {
  int64_t* dst = nullptr;

  switch (id) {
    case kWebMIdMatrixCoefficients:
      dst = &matrix_coefficients_;
      break;
    case kWebMIdBitsPerChannel:
      dst = &bits_per_channel_;
      break;
    case kWebMIdChromaSubsamplingHorz:
      dst = &chroma_subsampling_horz_;
      break;
    case kWebMIdChromaSubsamplingVert:
      dst = &chroma_subsampling_vert_;
      break;
    case kWebMIdCbSubsamplingHorz:
      dst = &cb_subsampling_horz_;
      break;
    case kWebMIdCbSubsamplingVert:
      dst = &cb_subsampling_vert_;
      break;
    case kWebMIdChromaSitingHorz:
      dst = &chroma_siting_horz_;
      break;
    case kWebMIdChromaSitingVert:
      dst = &chroma_siting_vert_;
      break;
    case kWebMIdRange:
      dst = &range_;
      break;
    case kWebMIdTransferCharacteristics:
      dst = &transfer_characteristics_;
      break;
    case kWebMIdPrimaries:
      dst = &primaries_;
      break;
    case kWebMIdMaxCLL:
      dst = &max_content_light_level_;
      break;
    case kWebMIdMaxFALL:
      dst = &max_frame_average_light_level_;
      break;
    default:
      return true;
  }

  DCHECK(dst);
  if (*dst != -1) {
    LOG(ERROR) << "Multiple values for id " << std::hex << id << " specified ("
               << *dst << " and " << val << ")";
    return false;
  }

  *dst = val;
  return true;
}

WebMColorMetadata WebMColourParser::GetWebMColorMetadata() const {
  WebMColorMetadata color_metadata;

  if (bits_per_channel_ != -1)
    color_metadata.BitsPerChannel = bits_per_channel_;

  if (chroma_subsampling_horz_ != -1)
    color_metadata.ChromaSubsamplingHorz = chroma_subsampling_horz_;
  if (chroma_subsampling_vert_ != -1)
    color_metadata.ChromaSubsamplingVert = chroma_subsampling_vert_;
  if (cb_subsampling_horz_ != -1)
    color_metadata.CbSubsamplingHorz = cb_subsampling_horz_;
  if (cb_subsampling_vert_ != -1)
    color_metadata.CbSubsamplingVert = cb_subsampling_vert_;
  if (chroma_siting_horz_ != -1)
    color_metadata.ChromaSitingHorz = chroma_siting_horz_;
  if (chroma_siting_vert_ != -1)
    color_metadata.ChromaSitingVert = chroma_siting_vert_;

  gfx::ColorSpace::RangeID range_id = gfx::ColorSpace::RangeID::INVALID;
  if (range_ >= static_cast<int64_t>(Range::kMinValue) &&
      range_ <= static_cast<int64_t>(Range::kMaxValue)) {
    switch (static_cast<Range>(range_)) {
      case Range::kUnspecified:
        range_id = gfx::ColorSpace::RangeID::INVALID;
        break;
      case Range::kBroadcast:
        range_id = gfx::ColorSpace::RangeID::LIMITED;
        break;
      case Range::kFull:
        range_id = gfx::ColorSpace::RangeID::FULL;
        break;
      case Range::kDerived:
        range_id = gfx::ColorSpace::RangeID::DERIVED;
        break;
    }
  }
  color_metadata.color_space = VideoColorSpace(
      primaries_, transfer_characteristics_, matrix_coefficients_, range_id);

  if (max_content_light_level_ != -1)
    color_metadata.hdr_metadata.max_content_light_level =
        max_content_light_level_;

  if (max_frame_average_light_level_ != -1)
    color_metadata.hdr_metadata.max_frame_average_light_level =
        max_frame_average_light_level_;

  if (mastering_metadata_parsed_)
    color_metadata.hdr_metadata.mastering_metadata =
        mastering_metadata_parser_.GetMasteringMetadata();

  return color_metadata;
}

}  // namespace media
