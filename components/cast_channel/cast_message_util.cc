// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cast_channel/cast_message_util.h"

#include <memory>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "components/cast_channel/cast_auth_util.h"
#include "components/cast_channel/proto/cast_channel.pb.h"

using base::Value;

namespace cast_channel {

namespace {
// Reserved message namespaces for internal messages.
constexpr char kCastInternalNamespacePrefix[] = "urn:x-cast:com.google.cast.";
constexpr char kAuthNamespace[] = "urn:x-cast:com.google.cast.tp.deviceauth";
constexpr char kHeartbeatNamespace[] =
    "urn:x-cast:com.google.cast.tp.heartbeat";
constexpr char kConnectionNamespace[] =
    "urn:x-cast:com.google.cast.tp.connection";
constexpr char kReceiverNamespace[] = "urn:x-cast:com.google.cast.receiver";
constexpr char kBroadcastNamespace[] = "urn:x-cast:com.google.cast.broadcast";

// Text payload keys.
constexpr char kTypeNodeId[] = "type";
constexpr char kRequestIdNodeId[] = "requestId";

// Cast application protocol message types.
constexpr char kKeepAlivePingType[] = "PING";
constexpr char kKeepAlivePongType[] = "PONG";
constexpr char kGetAppAvailabilityRequestType[] = "GET_APP_AVAILABILITY";
constexpr char kConnectionRequestType[] = "CONNECT";
constexpr char kBroadcastRequestType[] = "APPLICATION_BROADCAST";
constexpr char kLaunchRequestType[] = "LAUNCH";
constexpr char kReceiverStatusType[] = "RECEIVER_STATUS";
constexpr char kLaunchErrorType[] = "LAUNCH_ERROR";

// The value used for "sdkType" in a virtual connect request. Historically, this
// value is used in the Media Router extension, but here it is reused in Chrome.
constexpr int kVirtualConnectSdkType = 2;

// The value used for "connectionType" in a virtual connect request. This value
// stands for CONNECTION_TYPE_LOCAL, which is the only type used in Chrome.
constexpr int kVirtualConnectTypeLocal = 1;

void FillCommonCastMessageFields(CastMessage* message,
                                 const std::string& source_id,
                                 const std::string& destination_id,
                                 const std::string& message_namespace) {
  message->set_protocol_version(CastMessage::CASTV2_1_0);
  message->set_source_id(source_id);
  message->set_destination_id(destination_id);
  message->set_namespace_(message_namespace);
}

CastMessage CreateKeepAliveMessage(const char* keep_alive_type) {
  base::DictionaryValue type_dict;
  type_dict.SetString(kTypeNodeId, keep_alive_type);
  return CreateCastMessage(kHeartbeatNamespace, type_dict, kPlatformSenderId,
                           kPlatformReceiverId);
}

// Returns the value to be set as the "platform" value in a virtual connect
// request. The value is platform-dependent and is taken from the Platform enum
// defined in third_party/metrics_proto/cast_logs.proto.
int GetVirtualConnectPlatformValue() {
#if defined(OS_WIN)
  return 3;
#elif defined(OS_MACOSX)
  return 4;
#elif defined(OS_CHROMEOS)
  return 5;
#elif defined(OS_LINUX)
  return 6;
#else
  return 0;
#endif
}

}  // namespace

bool IsCastMessageValid(const CastMessage& message_proto) {
  if (!message_proto.IsInitialized())
    return false;

  if (message_proto.namespace_().empty() || message_proto.source_id().empty() ||
      message_proto.destination_id().empty()) {
    return false;
  }
  return (message_proto.payload_type() == CastMessage_PayloadType_STRING &&
          message_proto.has_payload_utf8()) ||
         (message_proto.payload_type() == CastMessage_PayloadType_BINARY &&
          message_proto.has_payload_binary());
}

std::unique_ptr<base::DictionaryValue> GetDictionaryFromCastMessage(
    const CastMessage& message) {
  if (!message.has_payload_utf8())
    return nullptr;

  return base::DictionaryValue::From(
      base::JSONReader::Read(message.payload_utf8()));
}

bool IsCastInternalNamespace(const std::string& message_namespace) {
  // Note: any namespace with the prefix is assumed to be reserved for internal
  // messages.
  return base::StartsWith(message_namespace, kCastInternalNamespacePrefix,
                          base::CompareCase::SENSITIVE);
}

CastMessageType ParseMessageType(const CastMessage& message) {
  std::unique_ptr<base::DictionaryValue> dictionary =
      GetDictionaryFromCastMessage(message);
  return dictionary ? ParseMessageTypeFromPayload(*dictionary)
                    : CastMessageType::kOther;
}

CastMessageType ParseMessageTypeFromPayload(const base::Value& payload) {
  const Value* type_string =
      payload.FindKeyOfType(kTypeNodeId, Value::Type::STRING);
  return type_string ? CastMessageTypeFromString(type_string->GetString())
                     : CastMessageType::kOther;
}

const char* CastMessageTypeToString(CastMessageType message_type) {
  switch (message_type) {
    case CastMessageType::kPing:
      return kKeepAlivePingType;
    case CastMessageType::kPong:
      return kKeepAlivePongType;
    case CastMessageType::kGetAppAvailability:
      return kGetAppAvailabilityRequestType;
    case CastMessageType::kConnect:
      return kConnectionRequestType;
    case CastMessageType::kBroadcast:
      return kBroadcastRequestType;
    case CastMessageType::kLaunch:
      return kLaunchRequestType;
    case CastMessageType::kReceiverStatus:
      return kReceiverStatusType;
    case CastMessageType::kLaunchError:
      return kLaunchErrorType;
    case CastMessageType::kOther:
      return "";
  }
  NOTREACHED();
  return "";
}

CastMessageType CastMessageTypeFromString(const std::string& type) {
  if (type == kKeepAlivePingType)
    return CastMessageType::kPing;
  if (type == kKeepAlivePongType)
    return CastMessageType::kPong;
  if (type == kGetAppAvailabilityRequestType)
    return CastMessageType::kGetAppAvailability;
  if (type == kConnectionRequestType)
    return CastMessageType::kConnect;
  if (type == kBroadcastRequestType)
    return CastMessageType::kBroadcast;
  if (type == kLaunchRequestType)
    return CastMessageType::kLaunch;
  if (type == kReceiverStatusType)
    return CastMessageType::kReceiverStatus;
  if (type == kLaunchErrorType)
    return CastMessageType::kLaunchError;

  DVLOG(1) << "Unknown message type: " << type;
  return CastMessageType::kOther;
}

std::string CastMessageToString(const CastMessage& message_proto) {
  std::string out("{");
  out += "namespace = " + message_proto.namespace_();
  out += ", sourceId = " + message_proto.source_id();
  out += ", destId = " + message_proto.destination_id();
  out += ", type = " + base::IntToString(message_proto.payload_type());
  out += ", str = \"" + message_proto.payload_utf8() + "\"}";
  return out;
}

std::string AuthMessageToString(const DeviceAuthMessage& message) {
  std::string out("{");
  if (message.has_challenge()) {
    out += "challenge: {}, ";
  }
  if (message.has_response()) {
    out += "response: {signature: (";
    out += base::NumberToString(message.response().signature().length());
    out += " bytes), certificate: (";
    out += base::NumberToString(
        message.response().client_auth_certificate().length());
    out += " bytes)}";
  }
  if (message.has_error()) {
    out += ", error: {";
    out += base::IntToString(message.error().error_type());
    out += "}";
  }
  out += "}";
  return out;
}

void CreateAuthChallengeMessage(CastMessage* message_proto,
                                const AuthContext& auth_context) {
  CHECK(message_proto);
  DeviceAuthMessage auth_message;

  AuthChallenge* challenge = auth_message.mutable_challenge();
  DCHECK(challenge);
  challenge->set_sender_nonce(auth_context.nonce());
  challenge->set_hash_algorithm(SHA256);

  std::string auth_message_string;
  auth_message.SerializeToString(&auth_message_string);

  FillCommonCastMessageFields(message_proto, kPlatformSenderId,
                              kPlatformReceiverId, kAuthNamespace);
  message_proto->set_payload_type(CastMessage_PayloadType_BINARY);
  message_proto->set_payload_binary(auth_message_string);
}

bool IsAuthMessage(const CastMessage& message) {
  return message.namespace_() == kAuthNamespace;
}

bool IsReceiverMessage(const CastMessage& message) {
  return message.namespace_() == kReceiverNamespace;
}

bool IsPlatformSenderMessage(const CastMessage& message) {
  return message.destination_id() != cast_channel::kPlatformSenderId;
}

CastMessage CreateKeepAlivePingMessage() {
  return CreateKeepAliveMessage(kKeepAlivePingType);
}

CastMessage CreateKeepAlivePongMessage() {
  return CreateKeepAliveMessage(kKeepAlivePongType);
}

CastMessage CreateVirtualConnectionRequest(
    const std::string& source_id,
    const std::string& destination_id,
    VirtualConnectionType connection_type,
    const std::string& user_agent,
    const std::string& browser_version) {
  DCHECK(destination_id != kPlatformReceiverId || connection_type == kStrong);

  // Parse system_version from user agent string. It contains platform, OS and
  // CPU info and is contained in the first set of parentheses of the user
  // agent string (e.g., X11; Linux x86_64).
  std::string system_version;
  size_t start_index = user_agent.find('(');
  if (start_index != std::string::npos) {
    size_t end_index = user_agent.find(')', start_index + 1);
    if (end_index != std::string::npos) {
      system_version =
          user_agent.substr(start_index + 1, end_index - start_index - 1);
    }
  }

  Value dict(Value::Type::DICTIONARY);
  dict.SetKey(kTypeNodeId, Value(kConnectionRequestType));
  dict.SetKey("userAgent", Value(user_agent));
  dict.SetKey("connType", Value(connection_type));
  dict.SetKey("origin", Value(Value::Type::DICTIONARY));

  Value sender_info(Value::Type::DICTIONARY);
  sender_info.SetKey("sdkType", Value(kVirtualConnectSdkType));
  sender_info.SetKey("version", Value(browser_version));
  sender_info.SetKey("browserVersion", Value(browser_version));
  sender_info.SetKey("platform", Value(GetVirtualConnectPlatformValue()));
  sender_info.SetKey("connectionType", Value(kVirtualConnectTypeLocal));
  if (!system_version.empty())
    sender_info.SetKey("systemVersion", Value(system_version));

  dict.SetKey("senderInfo", std::move(sender_info));

  return CreateCastMessage(kConnectionNamespace, dict, source_id,
                           destination_id);
}

CastMessage CreateGetAppAvailabilityRequest(const std::string& source_id,
                                            int request_id,
                                            const std::string& app_id) {
  Value dict(Value::Type::DICTIONARY);
  dict.SetKey(kTypeNodeId, Value(kGetAppAvailabilityRequestType));
  Value app_id_value(Value::Type::LIST);
  app_id_value.GetList().push_back(Value(app_id));
  dict.SetKey("appId", std::move(app_id_value));
  dict.SetKey(kRequestIdNodeId, Value(request_id));

  return CreateCastMessage(kReceiverNamespace, dict, source_id,
                           kPlatformReceiverId);
}

BroadcastRequest::BroadcastRequest(const std::string& broadcast_namespace,
                                   const std::string& message)
    : broadcast_namespace(broadcast_namespace), message(message) {}
BroadcastRequest::~BroadcastRequest() = default;

bool BroadcastRequest::operator==(const BroadcastRequest& other) const {
  return broadcast_namespace == other.broadcast_namespace &&
         message == other.message;
}

CastMessage CreateBroadcastRequest(const std::string& source_id,
                                   int request_id,
                                   const std::vector<std::string>& app_ids,
                                   const BroadcastRequest& request) {
  Value dict(Value::Type::DICTIONARY);
  dict.SetKey(kTypeNodeId, Value(kBroadcastRequestType));
  std::vector<Value> app_ids_value;
  for (const std::string& app_id : app_ids)
    app_ids_value.push_back(Value(app_id));

  dict.SetKey("appIds", Value(std::move(app_ids_value)));
  dict.SetKey("namespace", Value(request.broadcast_namespace));
  dict.SetKey("message", Value(request.message));
  return CreateCastMessage(kBroadcastNamespace, dict, source_id,
                           kPlatformReceiverId);
}

CastMessage CreateLaunchRequest(const std::string& source_id,
                                int request_id,
                                const std::string& app_id,
                                const std::string& locale) {
  Value dict(Value::Type::DICTIONARY);
  dict.SetKey(kTypeNodeId, Value(kLaunchRequestType));
  dict.SetKey(kRequestIdNodeId, Value(request_id));
  dict.SetKey("appId", Value(app_id));
  dict.SetKey("language", Value(locale));

  return CreateCastMessage(kReceiverNamespace, dict, source_id,
                           kPlatformReceiverId);
}

CastMessage CreateCastMessage(const std::string& message_namespace,
                              const base::Value& message,
                              const std::string& source_id,
                              const std::string& destination_id) {
  CastMessage output;
  FillCommonCastMessageFields(&output, source_id, destination_id,
                              message_namespace);
  output.set_payload_type(
      CastMessage::PayloadType::CastMessage_PayloadType_STRING);
  CHECK(base::JSONWriter::Write(message, output.mutable_payload_utf8()));
  return output;
}

const char* GetAppAvailabilityResultToString(GetAppAvailabilityResult result) {
  switch (result) {
    case GetAppAvailabilityResult::kAvailable:
      return "available";
    case GetAppAvailabilityResult::kUnavailable:
      return "unavailable";
    case GetAppAvailabilityResult::kUnknown:
      return "unknown";
  }
}

bool GetRequestIdFromResponse(const Value& payload, int* request_id) {
  DCHECK(request_id);
  DCHECK(payload.is_dict());

  const Value* request_id_value =
      payload.FindKeyOfType(kRequestIdNodeId, Value::Type::INTEGER);
  if (!request_id_value)
    return false;

  *request_id = request_id_value->GetInt();
  return true;
}

GetAppAvailabilityResult GetAppAvailabilityResultFromResponse(
    const Value& payload,
    const std::string& app_id) {
  DCHECK(payload.is_dict());
  const Value* availability_value =
      payload.FindPathOfType({"availability", app_id}, Value::Type::STRING);
  if (!availability_value)
    return GetAppAvailabilityResult::kUnknown;

  if (availability_value->GetString() == "APP_AVAILABLE")
    return GetAppAvailabilityResult::kAvailable;
  if (availability_value->GetString() == "APP_UNAVAILABLE")
    return GetAppAvailabilityResult::kUnavailable;

  return GetAppAvailabilityResult::kUnknown;
}

LaunchSessionResponse::LaunchSessionResponse() {}
LaunchSessionResponse::LaunchSessionResponse(LaunchSessionResponse&& other) =
    default;
LaunchSessionResponse::~LaunchSessionResponse() = default;

LaunchSessionResponse GetLaunchSessionResponse(
    const base::DictionaryValue& payload) {
  const Value* type_value =
      payload.FindKeyOfType(kTypeNodeId, Value::Type::STRING);
  if (!type_value)
    return LaunchSessionResponse();

  if (type_value->GetString() != kReceiverStatusType &&
      type_value->GetString() != kLaunchErrorType)
    return LaunchSessionResponse();

  LaunchSessionResponse response;
  if (type_value->GetString() == kLaunchErrorType) {
    response.result = LaunchSessionResponse::Result::kError;
    return response;
  }

  const Value* receiver_status =
      payload.FindKeyOfType("status", Value::Type::DICTIONARY);
  if (!receiver_status)
    return LaunchSessionResponse();

  response.result = LaunchSessionResponse::Result::kOk;
  response.receiver_status = receiver_status->Clone();
  return response;
}

}  // namespace cast_channel
