// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_TRANSPORT_STREAM_PACKETIZER_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_TRANSPORT_STREAM_PACKETIZER_H_

#include <vector>

#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "extensions/renderer/api/display_source/wifi_display/wifi_display_stream_packet_part.h"

namespace extensions {

class WiFiDisplayElementaryStreamInfo;

// This class represents an MPEG Transport Stream (MPEG-TS) packet containing
// WiFi Display elementary stream unit data or related meta information.
class WiFiDisplayTransportStreamPacket {
 public:
  enum { kPacketSize = 188u };

  using Part = WiFiDisplayStreamPacketPart;

  // This class represents a possibly empty padding part in the end of
  // a transport stream packet. The padding part consists of repeated bytes
  // having the same value.
  class PaddingPart {
   public:
    explicit PaddingPart(unsigned size) : size_(size) {}

    unsigned size() const { return size_; }
    uint8_t value() const { return 0xFFu; }

   private:
    const unsigned size_;

    DISALLOW_COPY_AND_ASSIGN(PaddingPart);
  };

  WiFiDisplayTransportStreamPacket(const uint8_t* header_data,
                                   size_t header_size,
                                   const uint8_t* payload_data,
                                   size_t payload_size);

  const Part& header() const { return header_; }
  const Part& payload() const { return payload_; }
  const PaddingPart& filler() const { return filler_; }

 private:
  const Part header_;
  const Part payload_;
  const PaddingPart filler_;

  DISALLOW_COPY_AND_ASSIGN(WiFiDisplayTransportStreamPacket);
};

// The WiFi Display transport stream packetizer packetizes unit buffers to
// MPEG Transport Stream (MPEG-TS) packets containing either meta information
// or Packetized Elementary Stream (PES) packets containing unit data.
//
// Whenever a Transport Stream (TS) packet is fully created and thus ready for
// further processing, a pure virtual member function
// |OnPacketizedTransportStreamPacket| is called.
class WiFiDisplayTransportStreamPacketizer {
 public:
  struct ElementaryStreamState;

  enum ElementaryStreamType : uint8_t {
    AUDIO_AAC = 0x0Fu,
    AUDIO_AC3 = 0x81u,
    AUDIO_LPCM = 0x83u,
    VIDEO_H264 = 0x1Bu,
  };

  // Fixed coding parameters for Linear Pulse-Code Modulation (LPCM) audio
  // streams. See |WiFiDisplayElementaryStreamDescriptor::LPCMAudioStream| for
  // variable ones.
  struct LPCM {
    enum {
      kFramesPerUnit = 6u,
      kChannelSamplesPerFrame = 80u,
      kChannelSamplesPerUnit = kChannelSamplesPerFrame * kFramesPerUnit
    };
  };

  WiFiDisplayTransportStreamPacketizer(
      const base::TimeDelta& delay_for_unit_time_stamps,
      const std::vector<WiFiDisplayElementaryStreamInfo>& stream_infos);
  virtual ~WiFiDisplayTransportStreamPacketizer();

  // Encodes one elementary stream unit buffer (such as one video frame or
  // 2 * |LPCM::kChannelSamplesPerUnit| two-channel LPCM audio samples) into
  // packets:
  //  1) Encodes meta information into meta information packets (by calling
  //     |EncodeMetaInformation|) if needed.
  //  2) Normalizes unit time stamps (|pts| and |dts|) so that they are never
  //     smaller than a program clock reference.
  //  3) Encodes the elementary stream unit buffer to unit data packets.
  // Returns false in the case of an error in which case the caller should stop
  // encoding.
  //
  // In order to minimize encoding delays, |flush| should be true unless
  // the caller is about to continue encoding immediately.
  //
  // Precondition: Elementary streams are configured either using a constructor
  //               or using the |SetElementaryStreams| member function.
  bool EncodeElementaryStreamUnit(unsigned stream_index,
                                  const uint8_t* unit_data,
                                  size_t unit_size,
                                  bool random_access,
                                  base::TimeTicks pts,
                                  base::TimeTicks dts,
                                  bool flush);

  // Encodes meta information (program association table, program map table and
  // program clock reference). Returns false in the case of an error in which
  // case the caller should stop encoding.
  //
  // The |EncodeElementaryStreamUnit| member function calls this member function
  // when needed, thus the caller is responsible for calling this member
  // function explicitly only if the caller does silence suppression and does
  // thus not encode all elementary stream units by calling
  // the |EncodeElementaryStreamUnit| member function.
  //
  // In order to minimize encoding delays, |flush| should be true unless
  // the caller is about to continue encoding immediately.
  //
  // Precondition: Elementary streams are configured either using a constructor
  //               or using the |SetElementaryStreams| member function.
  bool EncodeMetaInformation(bool flush);

  bool SetElementaryStreams(
      const std::vector<WiFiDisplayElementaryStreamInfo>& stream_infos);

  void DetachFromThread() { DETACH_FROM_THREAD(thread_checker_); }

 protected:
  bool EncodeProgramAssociationTable(bool flush);
  bool EncodeProgramClockReference(bool flush);
  bool EncodeProgramMapTables(bool flush);
  void ForceEncodeMetaInformationBeforeNextUnit();

  // Normalizes unit time stamps by delaying them in order to ensure that unit
  // time stamps are never smaller than a program clock reference.
  // Precondition: The |UpdateDelayForUnitTimeStamps| member function is called.
  void NormalizeUnitTimeStamps(base::TimeTicks* pts,
                               base::TimeTicks* dts) const;
  // Update unit time stamp delay in order to ensure that normalized unit time
  // stamps are never smaller than a program clock reference.
  void UpdateDelayForUnitTimeStamps(const base::TimeTicks& pts,
                                    const base::TimeTicks& dts);

  // Called whenever a Transport Stream (TS) packet is fully created and thus
  // ready for further processing.
  virtual bool OnPacketizedTransportStreamPacket(
      const WiFiDisplayTransportStreamPacket& transport_stream_packet,
      bool flush) = 0;

 private:
  struct {
    uint8_t program_association_table_continuity;
    uint8_t program_map_table_continuity;
    uint8_t program_map_table_version;
    uint8_t program_clock_reference_continuity;
  } counters_;
  base::TimeDelta delay_for_unit_time_stamps_;
  base::TimeTicks program_clock_reference_;
  std::vector<ElementaryStreamState> stream_states_;
  std::vector<uint8_t> program_association_table_;
  std::vector<uint8_t> program_map_table_;

  THREAD_CHECKER(thread_checker_);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_TRANSPORT_STREAM_PACKETIZER_H_
