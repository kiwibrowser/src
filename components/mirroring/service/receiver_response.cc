// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mirroring/service/receiver_response.h"

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "components/mirroring/service/value_util.h"

namespace mirroring {

namespace {

// Get the response type from the type string value in the JSON message.
ResponseType GetResponseType(const std::string& type) {
  if (type == "ANSWER")
    return ResponseType::ANSWER;
  if (type == "STATUS_RESPONSE")
    return ResponseType::STATUS_RESPONSE;
  if (type == "CAPABILITIES_RESPONSE")
    return ResponseType::CAPABILITIES_RESPONSE;
  if (type == "RPC")
    return ResponseType::RPC;
  return ResponseType::UNKNOWN;
}

}  // namespace

Answer::Answer()
    : udp_port(-1), supports_get_status(false), cast_mode("mirroring") {}

Answer::~Answer() {}

Answer::Answer(const Answer& answer) = default;

bool Answer::Parse(const base::Value& raw_value) {
  return (raw_value.is_dict() && GetInt(raw_value, "udpPort", &udp_port) &&
          GetIntArray(raw_value, "ssrcs", &ssrcs) &&
          GetIntArray(raw_value, "sendIndexes", &send_indexes) &&
          GetString(raw_value, "IV", &iv) &&
          GetBool(raw_value, "receiverGetStatus", &supports_get_status) &&
          GetString(raw_value, "castMode", &cast_mode));
}

// ----------------------------------------------------------------------------

ReceiverStatus::ReceiverStatus() : wifi_snr(0) {}

ReceiverStatus::~ReceiverStatus() {}

ReceiverStatus::ReceiverStatus(const ReceiverStatus& status) = default;

bool ReceiverStatus::Parse(const base::Value& raw_value) {
  return (raw_value.is_dict() && GetDouble(raw_value, "wifiSnr", &wifi_snr) &&
          GetIntArray(raw_value, "wifiSpeed", &wifi_speed));
}

// ----------------------------------------------------------------------------

ReceiverKeySystem::ReceiverKeySystem() {}

ReceiverKeySystem::~ReceiverKeySystem() {}

ReceiverKeySystem::ReceiverKeySystem(
    const ReceiverKeySystem& receiver_key_system) = default;

bool ReceiverKeySystem::Parse(const base::Value& raw_value) {
  return (raw_value.is_dict() && GetString(raw_value, "keySystemName", &name) &&
          GetStringArray(raw_value, "initDataTypes", &init_data_types) &&
          GetStringArray(raw_value, "codecs", &codecs) &&
          GetStringArray(raw_value, "secureCodecs", &secure_codecs) &&
          GetStringArray(raw_value, "audioRobustness", &audio_robustness) &&
          GetStringArray(raw_value, "videoRobustness", &video_robustness) &&
          GetString(raw_value, "persistentLicenseSessionSupport",
                    &persistent_license_session_support) &&
          GetString(raw_value, "persistentReleaseMessageSessionSupport",
                    &persistent_release_message_session_support) &&
          GetString(raw_value, "persistentStateSupport",
                    &persistent_state_support) &&
          GetString(raw_value, "distinctiveIdentifierSupport",
                    &distinctive_identifier_support));
}

// ----------------------------------------------------------------------------

ReceiverCapability::ReceiverCapability() {}

ReceiverCapability::~ReceiverCapability() {}

ReceiverCapability::ReceiverCapability(const ReceiverCapability& capabilities) =
    default;

bool ReceiverCapability::Parse(const base::Value& raw_value) {
  if (!raw_value.is_dict() ||
      !GetStringArray(raw_value, "mediaCaps", &media_caps))
    return false;
  auto* found = raw_value.FindKey("keySystems");
  if (!found)
    return true;
  for (const auto& key_system_value : found->GetList()) {
    ReceiverKeySystem key_system;
    if (!key_system.Parse(key_system_value))
      return false;
    key_systems.emplace_back(key_system);
  }
  return true;
}

// ----------------------------------------------------------------------------

ReceiverError::ReceiverError() : code(-1) {}

ReceiverError::~ReceiverError() {}

bool ReceiverError::Parse(const base::Value& raw_value) {
  if (!raw_value.is_dict() || !GetInt(raw_value, "code", &code) ||
      !GetString(raw_value, "description", &description))
    return false;
  auto* found = raw_value.FindKey("details");
  return found && base::JSONWriter::Write(*found, &details);
}

// ----------------------------------------------------------------------------

ReceiverResponse::ReceiverResponse()
    : type(ResponseType::UNKNOWN), session_id(-1), sequence_number(-1) {}

ReceiverResponse::~ReceiverResponse() {}

ReceiverResponse::ReceiverResponse(ReceiverResponse&& receiver_response) =
    default;

ReceiverResponse& ReceiverResponse::operator=(
    ReceiverResponse&& receiver_response) = default;

bool ReceiverResponse::Parse(const std::string& message_data) {
  std::unique_ptr<base::Value> raw_value = base::JSONReader::Read(message_data);
  if (!raw_value || !raw_value->is_dict() ||
      !GetInt(*raw_value, "sessionId", &session_id) ||
      !GetInt(*raw_value, "seqNum", &sequence_number) ||
      !GetString(*raw_value, "result", &result))
    return false;

  if (result == "error") {
    auto* found = raw_value->FindKey("error");
    if (found) {
      error = std::make_unique<ReceiverError>();
      if (!error->Parse(*found))
        return false;
    }
  }

  std::string message_type;
  if (!GetString(*raw_value, "type", &message_type))
    return false;
  // Convert |message_type| to uppercase.
  message_type = base::ToUpperASCII(message_type);
  type = GetResponseType(message_type);
  if (type == ResponseType::UNKNOWN) {
    DVLOG(2) << "Unknown response message type= " << message_type;
    return false;
  }

  auto* found = raw_value->FindKey("answer");
  if (found && !found->is_none()) {
    answer = std::make_unique<Answer>();
    if (!answer->Parse(*found))
      return false;
  }

  found = raw_value->FindKey("status");
  if (found && !found->is_none()) {
    status = std::make_unique<ReceiverStatus>();
    if (!status->Parse(*found))
      return false;
  }

  found = raw_value->FindKey("capabilities");
  if (found && !found->is_none()) {
    capabilities = std::make_unique<ReceiverCapability>();
    if (!capabilities->Parse(*found))
      return false;
  }

  found = raw_value->FindKey("rpc");
  if (found && !found->is_none()) {
    // Decode the base64-encoded string.
    if (!found->is_string() || !base::Base64Decode(found->GetString(), &rpc))
      return false;
  }

  return true;
}

}  // namespace mirroring
