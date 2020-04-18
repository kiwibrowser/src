// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MIRRORING_SERVICE_RECEIVER_RESPONSE_H_
#define COMPONENTS_MIRRORING_SERVICE_RECEIVER_RESPONSE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"

namespace mirroring {

// Receiver response message type.
enum ResponseType {
  UNKNOWN,
  ANSWER,                 // Response to OFFER message.
  STATUS_RESPONSE,        // Response to GET_STATUS message.
  CAPABILITIES_RESPONSE,  // Response to GET_CAPABILITIES message.
  RPC,                    // Rpc binary messages. The payload is base64 encoded.
};

struct Answer {
  Answer();
  ~Answer();
  Answer(const Answer& answer);
  bool Parse(const base::Value& raw_value);

  // The UDP port used for all streams in this session.
  int32_t udp_port;
  // The indexes chosen from the OFFER message.
  std::vector<int32_t> send_indexes;
  // The RTP SSRC used to send the RTCP feedback of the stream, indicated by
  // the |send_indexes| above.
  std::vector<int32_t> ssrcs;
  // A 128bit hex number containing the initialization vector for the crypto.
  std::string iv;
  // Indicates whether receiver supports the GET_STATUS command.
  bool supports_get_status;
  // "mirroring" for screen mirroring, or "remoting" for media remoting.
  std::string cast_mode;
};

struct ReceiverStatus {
  ReceiverStatus();
  ~ReceiverStatus();
  ReceiverStatus(const ReceiverStatus& status);
  bool Parse(const base::Value& raw_value);

  // Current WiFi signal to noise ratio in decibels.
  double wifi_snr;
  // Min, max, average, and current bandwidth in bps in order of the WiFi link.
  // Example: [1200, 1300, 1250, 1230].
  std::vector<int32_t> wifi_speed;
};

struct ReceiverKeySystem {
  ReceiverKeySystem();
  ~ReceiverKeySystem();
  ReceiverKeySystem(const ReceiverKeySystem& receiver_key_system);
  bool Parse(const base::Value& raw_value);

  // Reverse URI (e.g. com.widevine.alpha).
  std::string name;
  // EME init data types (e.g. cenc).
  std::vector<std::string> init_data_types;
  // Codecs supported by key system. This will include AVC and VP8 on all
  // Chromecasts.
  std::vector<std::string> codecs;
  // Codecs that are also hardware-secure.
  std::vector<std::string> secure_codecs;
  // Support levels for audio encryption robustness.
  std::vector<std::string> audio_robustness;
  // Support levels for video encryption robustness.
  std::vector<std::string> video_robustness;

  std::string persistent_license_session_support;
  std::string persistent_release_message_session_support;
  std::string persistent_state_support;
  std::string distinctive_identifier_support;
};

struct ReceiverCapability {
  ReceiverCapability();
  ~ReceiverCapability();
  ReceiverCapability(const ReceiverCapability& capabilities);
  bool Parse(const base::Value& raw_value);

  // Set of capabilities (e.g., ac3, 4k, hevc, vp9, dolby_vision, etc.).
  std::vector<std::string> media_caps;
  std::vector<ReceiverKeySystem> key_systems;
};

struct ReceiverError {
  ReceiverError();
  ~ReceiverError();
  bool Parse(const base::Value& raw_value);

  int32_t code;
  std::string description;
  std::string details;  // In JSON format.
};

struct ReceiverResponse {
  ReceiverResponse();
  ~ReceiverResponse();
  ReceiverResponse(ReceiverResponse&& receiver_response);
  ReceiverResponse& operator=(ReceiverResponse&& receiver_response);
  bool Parse(const std::string& message_data);

  ResponseType type;
  // All messages have same |session_id| for each mirroring session. This value
  // is provided by the media router provider.
  int32_t session_id;
  // This should be same as the value in the corresponding query/OFFER messages
  // for non-rpc messages.
  int32_t sequence_number;

  std::string result;  // "ok" or "error".

  // Only one of the following has value, according to |type|.
  std::unique_ptr<Answer> answer;
  std::string rpc;
  std::unique_ptr<ReceiverStatus> status;
  std::unique_ptr<ReceiverCapability> capabilities;
  // Can only be non-null when result is "error".
  std::unique_ptr<ReceiverError> error;
};

}  // namespace mirroring

#endif  // COMPONENTS_MIRRORING_SERVICE_RECEIVER_RESPONSE_H_
