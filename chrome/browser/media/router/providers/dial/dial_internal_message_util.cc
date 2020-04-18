// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/dial/dial_internal_message_util.h"

#include <array>

#include "base/base64url.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/media/router/providers/dial/dial_activity_manager.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "net/base/escape.h"
#include "url/url_util.h"

namespace media_router {

namespace {

constexpr int kSequenceNumberWrap = 2 << 30;

int GetNextDialLaunchSequenceNumber() {
  static int next_seq_number = 0;
  next_seq_number = (next_seq_number + 1) % kSequenceNumberWrap;
  return next_seq_number;
}

std::string GetNextSessionId() {
  static int session_id = 0;
  session_id = (session_id + 1) % kSequenceNumberWrap;
  return base::NumberToString(session_id);
}

std::string DialInternalMessageTypeToString(DialInternalMessageType type) {
  switch (type) {
    case DialInternalMessageType::kClientConnect:
      return "client_connect";
    case DialInternalMessageType::kV2Message:
      return "v2_message";
    case DialInternalMessageType::kReceiverAction:
      return "receiver_action";
    case DialInternalMessageType::kNewSession:
      return "new_session";
    case DialInternalMessageType::kCustomDialLaunch:
      return "custom_dial_launch";
    case DialInternalMessageType::kOther:
      break;
  }
  NOTREACHED() << "Unknown message type: " << static_cast<int>(type);
  return "unknown";
}

DialInternalMessageType StringToDialInternalMessageType(
    const std::string& str_type) {
  if (str_type == "client_connect")
    return DialInternalMessageType::kClientConnect;

  if (str_type == "v2_message")
    return DialInternalMessageType::kV2Message;

  if (str_type == "receiver_action")
    return DialInternalMessageType::kReceiverAction;

  if (str_type == "new_session")
    return DialInternalMessageType::kNewSession;

  if (str_type == "custom_dial_launch")
    return DialInternalMessageType::kCustomDialLaunch;

  return DialInternalMessageType::kOther;
}

base::Value CreateReceiver(const MediaSinkInternal& sink) {
  base::Value receiver(base::Value::Type::DICTIONARY);

  std::string label = base::SHA1HashString(sink.sink().id());
  base::Base64UrlEncode(label, base::Base64UrlEncodePolicy::OMIT_PADDING,
                        &label);
  receiver.SetKey("label", base::Value(label));

  receiver.SetKey("friendlyName",
                  base::Value(net::EscapeForHTML(sink.sink().name())));
  receiver.SetKey("capabilities", base::ListValue());
  receiver.SetKey("volume", base::Value(base::Value::Type::NONE));
  receiver.SetKey("isActiveInput", base::Value(base::Value::Type::NONE));
  receiver.SetKey("displayStatus", base::Value(base::Value::Type::NONE));

  receiver.SetKey("receiverType", base::Value("dial"));
  receiver.SetKey("ipAddress",
                  base::Value(sink.dial_data().ip_address.ToString()));
  return receiver;
}

std::string DialReceiverActionToString(DialReceiverAction action) {
  switch (action) {
    case DialReceiverAction::kCast:
      return "cast";
    case DialReceiverAction::kStop:
      return "stop";
  }
  NOTREACHED() << "Unknown DialReceiverAction: " << static_cast<int>(action);
  return "";
}

base::Value CreateReceiverActionBody(const MediaSinkInternal& sink,
                                     DialReceiverAction action) {
  base::Value message_body(base::Value::Type::DICTIONARY);
  message_body.SetKey("receiver", CreateReceiver(sink));
  message_body.SetKey("action",
                      base::Value(DialReceiverActionToString(action)));
  return message_body;
}

base::Value CreateNewSessionBody(const DialLaunchInfo& launch_info,
                                 const MediaSinkInternal& sink) {
  base::Value message_body(base::Value::Type::DICTIONARY);
  message_body.SetKey("sessionId", base::Value(GetNextSessionId()));
  message_body.SetKey("appId", base::Value(""));
  message_body.SetKey("displayName", base::Value(launch_info.app_name));
  message_body.SetKey("statusText", base::Value(""));
  message_body.SetKey("appImages", base::ListValue());
  message_body.SetKey("receiver", CreateReceiver(sink));
  message_body.SetKey("senderApps", base::ListValue());
  message_body.SetKey("namespaces", base::ListValue());
  message_body.SetKey("media", base::ListValue());
  message_body.SetKey("status", base::Value("connected"));
  message_body.SetKey("transportId", base::Value(""));
  return message_body;
}

base::Value CreateCustomDialLaunchBody(const MediaSinkInternal& sink,
                                       const ParsedDialAppInfo& app_info) {
  base::Value message_body(base::Value::Type::DICTIONARY);
  message_body.SetKey("receiver", CreateReceiver(sink));
  message_body.SetKey("appState",
                      base::Value(DialAppStateToString(app_info.state)));
  if (!app_info.extra_data.empty()) {
    base::Value extra_data(base::Value::Type::DICTIONARY);
    for (const auto& key_value : app_info.extra_data)
      message_body.SetKey(key_value.first, base::Value(key_value.second));

    message_body.SetKey("extraData", std::move(extra_data));
  }
  return message_body;
}

base::Value CreateDialMessageCommon(DialInternalMessageType type,
                                    base::Value body,
                                    const std::string& client_id) {
  base::Value message(base::Value::Type::DICTIONARY);
  message.SetKey("type", base::Value(DialInternalMessageTypeToString(type)));
  message.SetKey("message", std::move(body));
  message.SetKey("clientId", base::Value(client_id));
  message.SetKey("sequenceNumber", base::Value(-1));
  message.SetKey("timeoutMillis", base::Value(0));
  return message;
}

}  // namespace

// static
std::unique_ptr<DialInternalMessage> DialInternalMessage::From(
    const std::string& message) {
  // TODO(https://crbug.com/816628): This may need to be parsed out of process.
  std::unique_ptr<base::Value> message_value = base::JSONReader::Read(message);
  if (!message_value) {
    DVLOG(2) << "Failed to read JSON message: " << message;
    return nullptr;
  }

  base::Value* type_value = message_value->FindKey("type");
  if (!type_value || !type_value->is_string()) {
    DVLOG(2) << "Missing type value";
    return nullptr;
  }

  std::string str_type = type_value->GetString();
  DialInternalMessageType message_type =
      StringToDialInternalMessageType(str_type);
  if (message_type == DialInternalMessageType::kOther) {
    DVLOG(2) << __func__ << ": Unsupported message type: " << str_type;
    return nullptr;
  }

  base::Value* client_id_value = message_value->FindKey("clientId");
  if (!client_id_value || !client_id_value->is_string()) {
    DVLOG(2) << "Missing clientId";
    return nullptr;
  }

  // "message" is optional.
  base::Optional<base::Value> message_body;
  base::Value* message_body_value = message_value->FindKey("message");
  if (message_body_value)
    message_body = message_body_value->Clone();

  int sequence_number = -1;
  base::Value* sequence_number_value = message_value->FindKey("sequenceNumber");
  if (sequence_number_value && sequence_number_value->is_int())
    sequence_number = sequence_number_value->GetInt();

  return std::make_unique<DialInternalMessage>(
      message_type, std::move(message_body), client_id_value->GetString(),
      sequence_number);
}

DialInternalMessage::DialInternalMessage(DialInternalMessageType type,
                                         base::Optional<base::Value> body,
                                         const std::string& client_id,
                                         int sequence_number)
    : type(type),
      body(std::move(body)),
      client_id(client_id),
      sequence_number(sequence_number) {}
DialInternalMessage::~DialInternalMessage() = default;

// static
CustomDialLaunchMessageBody CustomDialLaunchMessageBody::From(
    const DialInternalMessage& message) {
  DCHECK(message.type == DialInternalMessageType::kCustomDialLaunch);

  const base::Optional<base::Value>& body = message.body;
  if (!body)
    return CustomDialLaunchMessageBody();

  const base::Value* do_launch_value = body->FindKey("doLaunch");
  if (!do_launch_value || !do_launch_value->is_bool())
    return CustomDialLaunchMessageBody();

  bool do_launch = do_launch_value->GetBool();

  base::Optional<std::string> launch_parameter;
  const base::Value* launch_parameter_value = body->FindKey("launchParameter");
  if (launch_parameter_value && launch_parameter_value->is_string())
    launch_parameter = launch_parameter_value->GetString();

  return CustomDialLaunchMessageBody(do_launch, launch_parameter);
}

CustomDialLaunchMessageBody::CustomDialLaunchMessageBody() = default;
CustomDialLaunchMessageBody::CustomDialLaunchMessageBody(
    bool do_launch,
    const base::Optional<std::string>& launch_parameter)
    : do_launch(do_launch), launch_parameter(launch_parameter) {}
CustomDialLaunchMessageBody::CustomDialLaunchMessageBody(
    const CustomDialLaunchMessageBody& other) = default;
CustomDialLaunchMessageBody::~CustomDialLaunchMessageBody() = default;

// static
bool DialInternalMessageUtil::IsStopSessionMessage(
    const DialInternalMessage& message) {
  if (message.type != DialInternalMessageType::kV2Message)
    return false;

  if (!message.body)
    return false;

  const base::Value* request_type = message.body->FindKey("type");
  return request_type && request_type->is_string() &&
         request_type->GetString() == "STOP";
}

// static
content::PresentationConnectionMessage
DialInternalMessageUtil::CreateNewSessionMessage(
    const DialLaunchInfo& launch_info,
    const MediaSinkInternal& sink) {
  base::Value message = CreateDialMessageCommon(
      DialInternalMessageType::kNewSession,
      CreateNewSessionBody(launch_info, sink), launch_info.client_id);

  std::string str;
  CHECK(base::JSONWriter::Write(message, &str));
  return content::PresentationConnectionMessage(std::move(str));
}

// static
content::PresentationConnectionMessage
DialInternalMessageUtil::CreateReceiverActionCastMessage(
    const DialLaunchInfo& launch_info,
    const MediaSinkInternal& sink) {
  base::Value message = CreateDialMessageCommon(
      DialInternalMessageType::kReceiverAction,
      CreateReceiverActionBody(sink, DialReceiverAction::kCast),
      launch_info.client_id);

  std::string str;
  CHECK(base::JSONWriter::Write(message, &str));
  return content::PresentationConnectionMessage(std::move(str));
}

// static
content::PresentationConnectionMessage
DialInternalMessageUtil::CreateReceiverActionStopMessage(
    const DialLaunchInfo& launch_info,
    const MediaSinkInternal& sink) {
  base::Value message = CreateDialMessageCommon(
      DialInternalMessageType::kReceiverAction,
      CreateReceiverActionBody(sink, DialReceiverAction::kStop),
      launch_info.client_id);

  std::string str;
  CHECK(base::JSONWriter::Write(message, &str));
  return content::PresentationConnectionMessage(std::move(str));
}

// static
std::pair<content::PresentationConnectionMessage, int>
DialInternalMessageUtil::CreateCustomDialLaunchMessage(
    const DialLaunchInfo& launch_info,
    const MediaSinkInternal& sink,
    const ParsedDialAppInfo& app_info) {
  int seq_number = GetNextDialLaunchSequenceNumber();
  base::Value message = CreateDialMessageCommon(
      DialInternalMessageType::kCustomDialLaunch,
      CreateCustomDialLaunchBody(sink, app_info), launch_info.client_id);
  message.SetKey("sequenceNumber", base::Value(seq_number));

  std::string str;
  CHECK(base::JSONWriter::Write(message, &str));
  return {content::PresentationConnectionMessage(std::move(str)), seq_number};
}

}  // namespace media_router
