// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BATTOR_AGENT_BATTOR_PROTOCOL_H_
#define TOOLS_BATTOR_AGENT_BATTOR_PROTOCOL_H_

#include <stdint.h>

namespace battor {

// Control characters in the BattOr protocol.
enum BattOrControlByte : uint8_t {
  // Indicates the start of a message in the protocol. All other instances of
  // this byte must be escaped (with BATTOR_SPECIAL_BYTE_ESCAPE).
  BATTOR_CONTROL_BYTE_START = 0x00,
  // Indicates the end of a message in the protocol. All other instances of
  // this byte must be escaped (with BATTOR_SPECIAL_BYTE_ESCAPE).
  BATTOR_CONTROL_BYTE_END = 0x01,
  // Indicates that the next byte should not be interpreted as a special
  // character, but should instead be interpreted as itself.
  BATTOR_CONTROL_BYTE_ESCAPE = 0x02,
};

// Types of BattOr messages that can be sent.
enum BattOrMessageType : uint8_t {
  // Indicates a control message sent from the client to the BattOr to tell the
  // BattOr to do something.
  BATTOR_MESSAGE_TYPE_CONTROL = 0x03,
  // Indicates a control message ack sent from the BattOr back to the client to
  // signal that the BattOr received the control message.
  BATTOR_MESSAGE_TYPE_CONTROL_ACK,
  // Indicates that the message contains Voltage and current measurements.
  BATTOR_MESSAGE_TYPE_SAMPLES,
  // TODO(charliea): Figure out what this is.
  BATTOR_MESSAGE_TYPE_PRINT,
};

// Types of BattOr control messages that can be sent.
enum BattOrControlMessageType : uint8_t {
  // Tells the BattOr to initialize itself.
  BATTOR_CONTROL_MESSAGE_TYPE_INIT = 0x00,
  // Tells the BattOr to reset itself.
  BATTOR_CONTROL_MESSAGE_TYPE_RESET,
  // Tells the BattOr to run a self test.
  BATTOR_CONTROL_MESSAGE_TYPE_SELF_TEST,
  // Tells the BattOr to send its EEPROM contents over the serial connection.
  BATTOR_CONTROL_MESSAGE_TYPE_READ_EEPROM,
  // Sets the current measurement's gain.
  BATTOR_CONTROL_MESSAGE_TYPE_SET_GAIN,
  // Tells the BattOr to start taking samples and sending them over the
  // connection.
  BATTOR_CONTROL_MESSAGE_TYPE_START_SAMPLING_UART,
  // Tells the BattOr to start taking samples and storing them on its SD card.
  BATTOR_CONTROL_MESSAGE_TYPE_START_SAMPLING_SD,
  // Tells the BattOr to start streaming the samples stored on its SD card over
  // the connection.
  BATTOR_CONTROL_MESSAGE_TYPE_READ_SD_UART,
  // Tells the BattOr to send back the number of samples it's collected so far.
  // This is used for syncing the clocks between the agent and the BattOr.
  BATTOR_CONTROL_MESSAGE_TYPE_READ_SAMPLE_COUNT,
  // Tells the BattOr to send back the git hash of the firmware.
  BATTOR_CONTROL_MESSAGE_TYPE_GET_FIRMWARE_GIT_HASH,
  // Read if the BattOr is portable or not.
  BATTOR_CONTROL_MESSAGE_TYPE_GET_MODE_PORTABLE,
  // Write the RTC seconds.
  BATTOR_CONTROL_MESSAGE_TYPE_SET_RTC,
  // Read the RTC seconds.
  BATTOR_CONTROL_MESSAGE_TYPE_GET_RTC,
};

// The gain level for the BattOr to use.
enum BattOrGain : uint8_t { BATTOR_GAIN_LOW = 0, BATTOR_GAIN_HIGH };

// The data types below are packed to ensure byte-compatibility with the BattOr
// firmware.
#pragma pack(push, 1)

// See: BattOrMessageType::BATTOR_MESSAGE_TYPE_CONTROL above.
struct BattOrControlMessage {
  BattOrControlMessageType type;
  uint16_t param1;
  uint16_t param2;
};

// See: BattOrMessageType::BATTOR_MESSAGE_TYPE_CONTROL_ACK above.
struct BattOrControlMessageAck {
  BattOrControlMessageType type;
  uint8_t param;
};

// TODO(charliea, aschulman): Write better descriptions for the EEPROM fields
// when we actually start doing the math to convert raw BattOr readings to
// accurate ones.

// The BattOr's EEPROM is persistent storage that contains information that we
// need in order to convert raw BattOr readings into accurate voltage and
// current measurements.
struct BattOrEEPROM {
  uint8_t magic[4];
  uint16_t version;
  char serial_num[20];
  uint32_t timestamp;
  float r1;
  float r2;
  float r3;
  float low_gain;
  float low_gain_correction_factor;
  float low_gain_correction_offset;
  uint16_t low_gain_amppot;
  float high_gain;
  float high_gain_correction_factor;
  float high_gain_correction_offset;
  uint16_t high_gain_amppot;
  uint32_t sd_sample_rate;
  uint16_t sd_tdiv;
  uint16_t sd_tovf;
  uint16_t sd_filpot;
  uint32_t uart_sr;
  uint16_t uart_tdiv;
  uint16_t uart_tovf;
  uint16_t uart_filpot;
  uint32_t crc32;
};

// The BattOrFrameHeader begins every frame containing BattOr samples.
struct BattOrFrameHeader {
  // The number of frames that have preceded this one.
  uint32_t sequence_number;
  // The number of bytes of raw samples in this frame.
  uint16_t length;
};

// A single BattOr sample. These samples are raw because they come directly from
// the BattOr's analog to digital converter and comprise only part of the
// equation to calculate meaningful voltage and current measurements.
struct RawBattOrSample {
  int16_t voltage_raw;
  int16_t current_raw;
};

// A single BattOr sample after timestamp assignment and conversion to unitful
// numbers.
struct BattOrSample {
  double time_ms;
  double voltage_mV;
  double current_mA;
};

#pragma pack(pop)

}  // namespace battor

#endif  // TOOLS_BATTOR_AGENT_BATTOR_PROTOCOL_H_
