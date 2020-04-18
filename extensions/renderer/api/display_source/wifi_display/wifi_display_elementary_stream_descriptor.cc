// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/api/display_source/wifi_display/wifi_display_elementary_stream_descriptor.h"

#include <cstring>

#include "base/logging.h"

namespace extensions {

WiFiDisplayElementaryStreamDescriptor::WiFiDisplayElementaryStreamDescriptor(
    const WiFiDisplayElementaryStreamDescriptor& other) {
  if (!other.empty()) {
    data_.reset(new uint8_t[other.size()]);
    std::memcpy(data(), other.data(), other.size());
  }
}

WiFiDisplayElementaryStreamDescriptor::WiFiDisplayElementaryStreamDescriptor(
    WiFiDisplayElementaryStreamDescriptor&&) = default;

WiFiDisplayElementaryStreamDescriptor::WiFiDisplayElementaryStreamDescriptor(
    DescriptorTag tag,
    uint8_t length)
    : data_(new uint8_t[kHeaderSize + length]) {
  uint8_t* p = data();
  *p++ = tag;
  *p++ = length;
  DCHECK_EQ(private_data(), p);
}

WiFiDisplayElementaryStreamDescriptor::
    ~WiFiDisplayElementaryStreamDescriptor() {}

WiFiDisplayElementaryStreamDescriptor& WiFiDisplayElementaryStreamDescriptor::
operator=(WiFiDisplayElementaryStreamDescriptor&&) = default;

const uint8_t* WiFiDisplayElementaryStreamDescriptor::data() const {
  return data_.get();
}

uint8_t* WiFiDisplayElementaryStreamDescriptor::data() {
  return data_.get();
}

size_t WiFiDisplayElementaryStreamDescriptor::size() const {
  if (empty())
    return 0u;
  return kHeaderSize + length();
}

WiFiDisplayElementaryStreamDescriptor
WiFiDisplayElementaryStreamDescriptor::AVCTimingAndHRD::Create() {
  WiFiDisplayElementaryStreamDescriptor descriptor(
      DESCRIPTOR_TAG_AVC_TIMING_AND_HRD, 2u);
  uint8_t* p = descriptor.private_data();
  *p++ = (false << 7) |  // hrd_management_valid_flag
         (0x3Fu << 1) |  // reserved (all six bits on)
         (false << 0);   // picture_and_timing_info_present
                         // No picture nor timing info bits.
  *p++ = (false << 7) |  // fixed_frame_rate_flag
         (false << 6) |  // temporal_poc_flag
         (false << 5) |  // picture_to_display_conversion_flag
         (0x1Fu << 0);   // reserved (all five bits on)
  DCHECK_EQ(descriptor.end(), p);
  return descriptor;
}

WiFiDisplayElementaryStreamDescriptor
WiFiDisplayElementaryStreamDescriptor::AVCVideo::Create(
    Profile profile_idc,
    bool constraint_set0_flag,
    bool constraint_set1_flag,
    bool constraint_set2_flag,
    uint8_t avc_compatible_flags,
    Level level_idc,
    bool avc_still_present) {
  const bool avc_24_hour_picture_flag = false;
  WiFiDisplayElementaryStreamDescriptor descriptor(DESCRIPTOR_TAG_AVC_VIDEO,
                                                   4u);
  uint8_t* p = descriptor.private_data();
  *p++ = profile_idc;
  *p++ = (constraint_set0_flag << 7) | (constraint_set1_flag << 6) |
         (constraint_set2_flag << 5) | (avc_compatible_flags << 0);
  *p++ = level_idc;
  *p++ = (avc_still_present << 7) | (avc_24_hour_picture_flag << 6) |
         (0x3Fu << 0);  // Reserved (all 6 bits on)
  DCHECK_EQ(descriptor.end(), p);
  return descriptor;
}

namespace {
struct LPCMAudioStreamByte0 {
  enum : uint8_t {
    kBitsPerSampleShift = 3u,
    kBitsPerSampleMask = ((1u << 2) - 1u) << kBitsPerSampleShift,
    kEmphasisFlagShift = 0u,
    kEmphasisFlagMask = 1u << kEmphasisFlagShift,
    kReservedOnBitsShift = 1u,
    kReservedOnBitsMask = ((1u << 2) - 1u) << kReservedOnBitsShift,
    kSamplingFrequencyShift = 5u,
    kSamplingFrequencyMask = ((1u << 3) - 1u) << kSamplingFrequencyShift,
  };
};

struct LPCMAudioStreamByte1 {
  enum : uint8_t {
    kNumberOfChannelsShift = 5u,
    kNumberOfChannelsMask = ((1u << 3) - 1u) << kNumberOfChannelsShift,
    kReservedOnBitsShift = 0u,
    kReservedOnBitsMask = ((1u << 4) - 1u) << kReservedOnBitsShift,
    // The bit not listed above having a shift 4u is a reserved off bit.
  };
};
}  // namespace

WiFiDisplayElementaryStreamDescriptor
WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream::Create(
    SamplingFrequency sampling_frequency,
    BitsPerSample bits_per_sample,
    bool emphasis_flag,
    NumberOfChannels number_of_channels) {
  WiFiDisplayElementaryStreamDescriptor descriptor(
      DESCRIPTOR_TAG_LPCM_AUDIO_STREAM, 2u);
  uint8_t* p = descriptor.private_data();
  *p++ = (sampling_frequency << LPCMAudioStreamByte0::kSamplingFrequencyShift) |
         (bits_per_sample << LPCMAudioStreamByte0::kBitsPerSampleShift) |
         LPCMAudioStreamByte0::kReservedOnBitsMask |
         (emphasis_flag << LPCMAudioStreamByte0::kEmphasisFlagShift);
  *p++ = (number_of_channels << LPCMAudioStreamByte1::kNumberOfChannelsShift) |
         LPCMAudioStreamByte1::kReservedOnBitsMask;
  DCHECK_EQ(descriptor.end(), p);
  return descriptor;
}

WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream::BitsPerSample
WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream::bits_per_sample()
    const {
  return static_cast<BitsPerSample>(
      (private_data()[0] & LPCMAudioStreamByte0::kBitsPerSampleMask) >>
      LPCMAudioStreamByte0::kBitsPerSampleShift);
}

bool WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream::emphasis_flag()
    const {
  return (private_data()[0] & LPCMAudioStreamByte0::kEmphasisFlagMask) != 0u;
}

WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream::NumberOfChannels
WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream::number_of_channels()
    const {
  return static_cast<NumberOfChannels>(
      (private_data()[1] & LPCMAudioStreamByte1::kNumberOfChannelsMask) >>
      LPCMAudioStreamByte1::kNumberOfChannelsShift);
}

WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream::SamplingFrequency
WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream::sampling_frequency()
    const {
  return static_cast<SamplingFrequency>(
      (private_data()[0] & LPCMAudioStreamByte0::kSamplingFrequencyMask) >>
      LPCMAudioStreamByte0::kSamplingFrequencyShift);
}

}  // namespace extensions
