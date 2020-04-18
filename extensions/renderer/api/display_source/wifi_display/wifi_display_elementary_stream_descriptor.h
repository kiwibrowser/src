// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_DESCRIPTOR_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_DESCRIPTOR_H_

#include <stdint.h>

#include <memory>
#include <type_traits>


namespace extensions {

// WiFi Display elementary stream descriptors are used for passing descriptive
// information about elementary streams to WiFiDisplayTransportStreamPacketizer
// which then packetizes that information so that it can be passed to a remote
// sink.
class WiFiDisplayElementaryStreamDescriptor {
 public:
  enum { kHeaderSize = 2u };

  enum DescriptorTag : uint8_t {
    DESCRIPTOR_TAG_AVC_VIDEO = 0x28u,
    DESCRIPTOR_TAG_AVC_TIMING_AND_HRD = 0x2A,
    DESCRIPTOR_TAG_LPCM_AUDIO_STREAM = 0x83u,
  };

  // Make Google Test treat this class as a container.
  using const_iterator = const uint8_t*;
  using iterator = const uint8_t*;

  WiFiDisplayElementaryStreamDescriptor(
      const WiFiDisplayElementaryStreamDescriptor&);
  WiFiDisplayElementaryStreamDescriptor(
      WiFiDisplayElementaryStreamDescriptor&&);
  ~WiFiDisplayElementaryStreamDescriptor();

  WiFiDisplayElementaryStreamDescriptor& operator=(
      WiFiDisplayElementaryStreamDescriptor&&);

  const uint8_t* begin() const { return data(); }
  const uint8_t* data() const;
  bool empty() const { return !data_; }
  const uint8_t* end() const { return data() + size(); }
  size_t size() const;

  DescriptorTag tag() const { return static_cast<DescriptorTag>(data()[0]); }
  uint8_t length() const { return data()[1]; }

  // AVC (Advanced Video Coding) timing and HRD (Hypothetical Reference
  // Decoder) descriptor provides timing and HRD parameters for a video stream.
  struct AVCTimingAndHRD {
    static WiFiDisplayElementaryStreamDescriptor Create();
  };

  // AVC (Advanced Video Coding) video descriptor provides basic coding
  // parameters for a video stream.
  struct AVCVideo {
    enum Level : uint8_t {
      LEVEL_3_1 = 31u,
      LEVEL_3_2 = 32u,
      LEVEL_4 = 40u,
      LEVEL_4_1 = 41u,
      LEVEL_4_2 = 42u
    };
    enum Profile : uint8_t {
      PROFILE_BASELINE = 66u,
      PROFILE_MAIN = 77u,
      PROFILE_HIGH = 100u
    };
    static WiFiDisplayElementaryStreamDescriptor Create(
        Profile profile_idc,
        bool constraint_set0_flag,
        bool constraint_set1_flag,
        bool constraint_set2_flag,
        uint8_t avc_compatible_flags,
        Level level_idc,
        bool avc_still_present);
  };

  class LPCMAudioStream;

 protected:
  WiFiDisplayElementaryStreamDescriptor(DescriptorTag tag, uint8_t length);

  uint8_t* data();
  const uint8_t* private_data() const { return data() + kHeaderSize; }
  uint8_t* private_data() { return data() + kHeaderSize; }

 private:
  std::unique_ptr<uint8_t[]> data_;
};

// LPCM (Linear pulse-code modulation) audio stream descriptor provides basic
// coding parameters for a private WiFi Display LPCM audio stream.
class WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream
    : public WiFiDisplayElementaryStreamDescriptor {
 public:
  enum { kTag = DESCRIPTOR_TAG_LPCM_AUDIO_STREAM };
  enum BitsPerSample : uint8_t { BITS_PER_SAMPLE_16 = 0u };
  enum NumberOfChannels : uint8_t {
    NUMBER_OF_CHANNELS_DUAL_MONO = 0u,
    NUMBER_OF_CHANNELS_STEREO = 1u
  };
  enum SamplingFrequency : uint8_t {
    SAMPLING_FREQUENCY_44_1K = 1u,
    SAMPLING_FREQUENCY_48K = 2u
  };

  static WiFiDisplayElementaryStreamDescriptor Create(
      SamplingFrequency sampling_frequency,
      BitsPerSample bits_per_sample,
      bool emphasis_flag,
      NumberOfChannels number_of_channels);

  BitsPerSample bits_per_sample() const;
  bool emphasis_flag() const;
  NumberOfChannels number_of_channels() const;
  SamplingFrequency sampling_frequency() const;
};

// Subclasses of WiFiDisplayElementaryStreamDescriptor MUST NOT define new
// member variables but only new non-virtual member functions which parse
// the inherited data. This allows WiFiDisplayElementaryStreamDescriptor
// pointers to be cast to subclass pointers.
static_assert(
    std::is_standard_layout<
        WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream>::value,
    "Forbidden memory layout for an elementary stream descriptor");

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_DESCRIPTOR_H_
