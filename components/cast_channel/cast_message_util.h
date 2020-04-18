// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_
#define COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_

#include <string>

#include "base/values.h"

namespace cast_channel {

class AuthContext;
class CastMessage;
class DeviceAuthMessage;

// Sender and receiver IDs to use for platform messages.
constexpr char kPlatformSenderId[] = "sender-0";
constexpr char kPlatformReceiverId[] = "receiver-0";

// Cast application protocol message types.
enum class CastMessageType {
  kPing,
  kPong,
  kGetAppAvailability,
  kConnect,    // Virtual connection request
  kBroadcast,  // Application broadcast / precache
  kLaunch,     // Session launch request
  kReceiverStatus,
  kLaunchError,
  kOther  // Add new types above |kOther|.
};

// Checks if the contents of |message_proto| are valid.
bool IsCastMessageValid(const CastMessage& message_proto);

// Parses and returns the UTF-8 payload from |message|. Returns nullptr
// if the UTF-8 payload doesn't exist, or if it is not a dictionary.
std::unique_ptr<base::DictionaryValue> GetDictionaryFromCastMessage(
    const CastMessage& message);

// Returns true if |message_namespace| is a namespace reserved for internal
// messages.
bool IsCastInternalNamespace(const std::string& message_namespace);

// Parses the JSON-encoded payload of |message| and returns the value in the
// "type" field or |kUnknown| if the parse fails or the field is not found.
// The result is only valid if |message| is a Cast application protocol message.
CastMessageType ParseMessageType(const CastMessage& message);

// Similar to |ParseMessageType()|, but |payload| is already JSON-parsed.
CastMessageType ParseMessageTypeFromPayload(const base::Value& payload);

// Returns a human readable string for |message_type|.
const char* CastMessageTypeToString(CastMessageType message_type);

// Returns the CastMessageType for |type|, or |kOther| if it does not
// correspond to a known type.
CastMessageType CastMessageTypeFromString(const std::string& type);

// Returns a human readable string for |message_proto|.
std::string CastMessageToString(const CastMessage& message_proto);

// Returns a human readable string for |message|.
std::string AuthMessageToString(const DeviceAuthMessage& message);

// Fills |message_proto| appropriately for an auth challenge request message.
// Uses the nonce challenge in |auth_context|.
void CreateAuthChallengeMessage(CastMessage* message_proto,
                                const AuthContext& auth_context);

// Returns whether the given message is an auth handshake message.
bool IsAuthMessage(const CastMessage& message);

// Returns whether |message| is a Cast receiver message.
bool IsReceiverMessage(const CastMessage& message);

// Returns whether |message| is destined for the platform sender.
bool IsPlatformSenderMessage(const CastMessage& message);

// Creates a keep-alive message of either type PING or PONG.
CastMessage CreateKeepAlivePingMessage();
CastMessage CreateKeepAlivePongMessage();

enum VirtualConnectionType {
  kStrong = 0,
  // kWeak = 1 not used.
  kInvisible = 2
};

// Creates a virtual connection request message for |source_id| and
// |destination_id|. |user_agent| and |browser_version| will be included with
// the request.
// If |destination_id| is kPlatformReceiverId, then |connection_type| must be
// kStrong. Otherwise |connection_type| can be either kStrong or kInvisible.
CastMessage CreateVirtualConnectionRequest(
    const std::string& source_id,
    const std::string& destination_id,
    VirtualConnectionType connection_type,
    const std::string& user_agent,
    const std::string& browser_version);

// Creates an app availability request for |app_id| from |source_id| with
// ID |request_id|.
// TODO(imcheng): May not need |source_id|, just use sender-0?
CastMessage CreateGetAppAvailabilityRequest(const std::string& source_id,
                                            int request_id,
                                            const std::string& app_id);

// Represents a broadcast request. Currently it is used for precaching data
// on a receiver.
struct BroadcastRequest {
  BroadcastRequest(const std::string& broadcast_namespace,
                   const std::string& message);
  ~BroadcastRequest();
  bool operator==(const BroadcastRequest& other) const;

  std::string broadcast_namespace;
  std::string message;
};

// Creates a broadcast request with the given parameters.
CastMessage CreateBroadcastRequest(const std::string& source_id,
                                   int request_id,
                                   const std::vector<std::string>& app_ids,
                                   const BroadcastRequest& request);

// Creates a session launch request with the given parameters.
CastMessage CreateLaunchRequest(const std::string& source_id,
                                int request_id,
                                const std::string& app_id,
                                const std::string& locale);

// Creates a generic CastMessage with |message| as the string payload. Used for
// app messages.
CastMessage CreateCastMessage(const std::string& message_namespace,
                              const base::Value& message,
                              const std::string& source_id,
                              const std::string& destination_id);

// Possible results of a GET_APP_AVAILABILITY request.
enum class GetAppAvailabilityResult {
  kAvailable,
  kUnavailable,
  kUnknown,
};

const char* GetAppAvailabilityResultToString(GetAppAvailabilityResult result);

// Extracts request ID from |payload| corresponding to a Cast message response.
// If request ID is available, assigns it to |request_id|. Return |true| if
// request ID is found.
bool GetRequestIdFromResponse(const base::Value& payload, int* request_id);

// Returns the GetAppAvailabilityResult corresponding to |app_id| in |payload|.
// Returns kUnknown if result is not found.
GetAppAvailabilityResult GetAppAvailabilityResultFromResponse(
    const base::Value& payload,
    const std::string& app_id);

// Result of a session launch.
struct LaunchSessionResponse {
  enum Result { kOk, kError, kTimedOut, kUnknown };

  LaunchSessionResponse();
  LaunchSessionResponse(LaunchSessionResponse&& other);
  ~LaunchSessionResponse();

  Result result = Result::kUnknown;

  // Populated if |result| is |kOk|.
  base::Optional<base::Value> receiver_status;
};

// Parses |payload| into a LaunchSessionResponse. Returns an empty
// LaunchSessionResponse if |payload| is not a properly formatted launch
// response. |payload| must be from the string payload of a CastMessage.
LaunchSessionResponse GetLaunchSessionResponse(
    const base::DictionaryValue& payload);

}  // namespace cast_channel

#endif  // COMPONENTS_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_
