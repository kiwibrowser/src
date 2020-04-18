// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Useful utility functions for the TICL

#include "google/cacheinvalidation/impl/proto-helpers.h"

#include <stddef.h>

#include <sstream>

#include "google/cacheinvalidation/client_test_internal.pb.h"
#include "google/cacheinvalidation/deps/string_util.h"

namespace invalidation {

using ::ipc::invalidation::RegistrationManagerStateP;

// Defines a ToString template method specialization for the given type.
#define DEFINE_TO_STRING(type)                                          \
  template<>                                                            \
  string ProtoHelpers::ToString(const type& message)

// Creates a stringstream |stream| and emits a leading "{ " to it.
#define BEGIN()                                 \
  std::stringstream stream;                     \
  stream << "{ "

// Emits a closing " }" on |stream| and returns the string that has been built.
#define END()                                   \
  stream << " }";                               \
  return stream.str()

// Defines a trivial ToString method for a type (which just returns "<type>").
#define DEFINE_TRIVIAL_TO_STRING(type)                                  \
  DEFINE_TO_STRING(type) {                                              \
    return "<" #type ">";                                               \
  }

// Emits "field: <field value as string>" if |field| is present in |message|.
#define OPTIONAL(field)                                                 \
  if (message.has_##field()) {                                          \
    stream << #field << ": " << ToString(message.field()) << " ";       \
  }

// Emits "field: <field value as string>" for each instance of field in message.
#define REPEATED(field)                                                 \
  for (int i = 0; i < message.field##_size(); ++i) {                    \
    stream << #field << ": " << ToString(message.field(i)) << " ";      \
  }

// Expands to a case branch that returns "name" if the implicitly tested
// expression is equal to the enum constant |name| in the given |type|.
#define ENUM_VALUE(type, name) case type##_##name: return #name

// Expands to a default case branch that returns the string representation of
// |message|.
#define ENUM_UNKNOWN() default: return SimpleItoa(message)

DEFINE_TO_STRING(bool) {
  return message ? "true" : "false";
}

DEFINE_TO_STRING(int) {
  std::stringstream stream;
  stream << message;
  return stream.str();
}

DEFINE_TO_STRING(int64_t) {
  std::stringstream stream;
  stream << message;
  return stream.str();
}

/*
 * Three arrays that store the representation of each character from 0 to 255.
 * The ith number's octal representation is: CHAR_OCTAL_STRINGS1[i],
 * CHAR_OCTAL_STRINGS2[i], CHAR_OCTAL_STRINGS3[i]
 * <p>
 * E.g., if the number 128, these arrays contain 2, 0, 0 at index 128. We use
 * 3 char arrays instead of an array of strings since the code path for a
 * character append operation is quite a bit shorterthan the append operation
 * for strings.
 */
char ProtoHelpers::CHAR_OCTAL_STRINGS1[];
char ProtoHelpers::CHAR_OCTAL_STRINGS2[];
char ProtoHelpers::CHAR_OCTAL_STRINGS3[];
bool ProtoHelpers::is_initialized = false;

template<>
string ProtoHelpers::ToString(const string& bytes) {
  // This is a racy initialization but that is ok since we are initializing to
  // the same values.
  if (!is_initialized) {
    // Initialize the array with the Octal string values so that we do not have
    //  to do string.format for every byte during runtime.
    for (int i = 0; i < ProtoHelpers::NUM_CHARS; i++) {
      string value = StringPrintf("%03o", i);
      ProtoHelpers::CHAR_OCTAL_STRINGS1[i] = value[0];
      ProtoHelpers::CHAR_OCTAL_STRINGS2[i] = value[1];
      ProtoHelpers::CHAR_OCTAL_STRINGS3[i] = value[2];
    }
    is_initialized = true;
  }
  string builder;
  builder.reserve(3 * bytes.length() + 2);
  builder += "\"";
  for (size_t i = 0; i < bytes.length(); i++) {
    char c = bytes[i];
    switch (c) {
      case '\n': builder += '\\'; builder += 'n'; break;
      case '\r': builder += '\\'; builder += 'r'; break;
      case '\t': builder += '\\'; builder += 't'; break;
      case '\"': builder += '\\'; builder += '"'; break;
      case '\\': builder += '\\'; builder += '\\'; break;
      default:
        if ((c >= 32) && (c < 127) && c != '\'') {
          builder += c;
        } else {
          int byteValue = c;
          if (c < 0) {
            byteValue = c + 256;
          }
          builder += '\\';
          builder += CHAR_OCTAL_STRINGS1[byteValue];
          builder += CHAR_OCTAL_STRINGS2[byteValue];
          builder += CHAR_OCTAL_STRINGS3[byteValue];
        }
        break;
    }
  }
  builder += "\"";
  return builder;
}

void ProtoHelpers::InitRegistrationP(const ObjectIdP& oid,
    RegistrationP::OpType op_type, RegistrationP* reg) {
  reg->mutable_object_id()->CopyFrom(oid);
  reg->set_op_type(op_type);
}

void ProtoHelpers::InitRateLimitP(int window_ms, int count,
    RateLimitP *rate_limit) {
  rate_limit->set_window_ms(window_ms);
  rate_limit->set_count(count);
}

void ProtoHelpers::InitInitializeMessage(
    const ApplicationClientIdP& application_client_id, const string& nonce,
    InitializeMessage* init_msg) {
  init_msg->set_client_type(application_client_id.client_type());
  init_msg->mutable_application_client_id()->CopyFrom(
      application_client_id);
  init_msg->set_nonce(nonce);
  init_msg->set_digest_serialization_type(
      InitializeMessage_DigestSerializationType_BYTE_BASED);
}

void ProtoHelpers::InitClientVersion(const string& platform,
    const string& application_info, ClientVersion* client_version) {
  Version* version = client_version->mutable_version();
  version->set_major_version(Constants::kClientMajorVersion);
  version->set_minor_version(Constants::kClientMinorVersion);
  client_version->set_platform(platform);
  client_version->set_language("C++");
  client_version->set_application_info(application_info);
}

void ProtoHelpers::InitProtocolVersion(ProtocolVersion* protocol_version) {
  Version* version = protocol_version->mutable_version();
  version->set_major_version(Constants::kProtocolMajorVersion);
  version->set_minor_version(Constants::kProtocolMinorVersion);
}

void ProtoHelpers::InitConfigVersion(Version* config_version) {
  config_version->set_major_version(Constants::kConfigMajorVersion);
  config_version->set_minor_version(Constants::kConfigMinorVersion);
}

DEFINE_TO_STRING(ErrorMessage::Code) {
  switch (message) {
    ENUM_VALUE(ErrorMessage_Code, AUTH_FAILURE);
    ENUM_VALUE(ErrorMessage_Code, UNKNOWN_FAILURE);
    ENUM_UNKNOWN();
  }
}
DEFINE_TO_STRING(InfoRequestMessage::InfoType) {
  switch (message) {
    ENUM_VALUE(InfoRequestMessage_InfoType, GET_PERFORMANCE_COUNTERS);
    ENUM_UNKNOWN();
  }
}

DEFINE_TO_STRING(InitializeMessage::DigestSerializationType) {
  switch (message) {
    ENUM_VALUE(InitializeMessage_DigestSerializationType, BYTE_BASED);
    ENUM_VALUE(InitializeMessage_DigestSerializationType, NUMBER_BASED);
    ENUM_UNKNOWN();
  }
}

DEFINE_TO_STRING(StatusP::Code) {
  switch (message) {
    ENUM_VALUE(StatusP_Code, SUCCESS);
    ENUM_VALUE(StatusP_Code, TRANSIENT_FAILURE);
    ENUM_VALUE(StatusP_Code, PERMANENT_FAILURE);
    ENUM_UNKNOWN();
  }
}

DEFINE_TO_STRING(RegistrationP::OpType) {
  switch (message) {
    ENUM_VALUE(RegistrationP_OpType, REGISTER);
    ENUM_VALUE(RegistrationP_OpType, UNREGISTER);
    ENUM_UNKNOWN();
  }
}

DEFINE_TO_STRING(RegistrationSyncRequestMessage) {
  BEGIN();
  END();
}

DEFINE_TO_STRING(Version) {
  BEGIN();
  OPTIONAL(major_version);
  OPTIONAL(minor_version);
  END();
}

DEFINE_TO_STRING(ClientVersion) {
  BEGIN();
  OPTIONAL(version);
  OPTIONAL(platform);
  OPTIONAL(language);
  OPTIONAL(application_info);
  END();
}

DEFINE_TO_STRING(ProtocolVersion) {
  BEGIN();
  OPTIONAL(version);
  END();
}

DEFINE_TO_STRING(InfoRequestMessage) {
  BEGIN();
  REPEATED(info_type);
  END();
}

DEFINE_TO_STRING(ConfigChangeMessage) {
  BEGIN();
  OPTIONAL(next_message_delay_ms);
  END();
}

DEFINE_TO_STRING(PropertyRecord) {
  BEGIN();
  OPTIONAL(name);
  OPTIONAL(value);
  END();
}

DEFINE_TO_STRING(RateLimitP) {
  BEGIN();
  OPTIONAL(window_ms);
  OPTIONAL(count);
  END();
}

DEFINE_TO_STRING(ProtocolHandlerConfigP) {
  BEGIN();
  OPTIONAL(batching_delay_ms);
  REPEATED(rate_limit);
  END();
}

DEFINE_TO_STRING(ClientConfigP) {
  BEGIN();
  OPTIONAL(version);
  OPTIONAL(network_timeout_delay_ms);
  OPTIONAL(write_retry_delay_ms);
  OPTIONAL(heartbeat_interval_ms);
  OPTIONAL(perf_counter_delay_ms);
  OPTIONAL(max_exponential_backoff_factor);
  OPTIONAL(smear_percent);
  OPTIONAL(is_transient);
  OPTIONAL(initial_persistent_heartbeat_delay_ms);
  OPTIONAL(protocol_handler_config);
  END();
}

DEFINE_TO_STRING(InfoMessage) {
  BEGIN();
  OPTIONAL(client_version);
  REPEATED(config_parameter);
  REPEATED(performance_counter);
  OPTIONAL(server_registration_summary_requested);
  END();
}

DEFINE_TO_STRING(ErrorMessage) {
  BEGIN();
  OPTIONAL(code);
  OPTIONAL(description);
  END();
}

DEFINE_TO_STRING(RegistrationSummary) {
  BEGIN();
  OPTIONAL(num_registrations);
  OPTIONAL(registration_digest);
  END();
}

DEFINE_TO_STRING(ObjectIdP) {
  BEGIN();
  OPTIONAL(source);
  OPTIONAL(name);
  END();
}

DEFINE_TO_STRING(InvalidationP) {
  BEGIN();
  OPTIONAL(object_id);
  OPTIONAL(is_known_version);
  OPTIONAL(version);
  OPTIONAL(is_trickle_restart);
  OPTIONAL(payload);
  END();
}

DEFINE_TO_STRING(AckHandleP) {
  BEGIN();
  OPTIONAL(invalidation);
  END();
}

DEFINE_TO_STRING(ApplicationClientIdP) {
  BEGIN();
  OPTIONAL(client_type);
  OPTIONAL(client_name);
  END();
}

DEFINE_TO_STRING(StatusP) {
  BEGIN();
  OPTIONAL(code);
  OPTIONAL(description);
  END();
}

DEFINE_TO_STRING(RegistrationP) {
  BEGIN();
  OPTIONAL(object_id);
  OPTIONAL(op_type);
  END();
}

DEFINE_TO_STRING(RegistrationStatus) {
  BEGIN();
  OPTIONAL(registration);
  OPTIONAL(status);
  END();
}

DEFINE_TO_STRING(ClientHeader) {
  BEGIN();
  OPTIONAL(protocol_version);
  OPTIONAL(client_token);
  OPTIONAL(registration_summary);
  OPTIONAL(client_time_ms);
  OPTIONAL(max_known_server_time_ms);
  OPTIONAL(message_id);
  END();
}

DEFINE_TO_STRING(InitializeMessage) {
  BEGIN();
  OPTIONAL(client_type);
  OPTIONAL(nonce);
  OPTIONAL(application_client_id);
  OPTIONAL(digest_serialization_type);
  END();
}

DEFINE_TO_STRING(RegistrationMessage) {
  BEGIN();
  REPEATED(registration);
  END();
}

DEFINE_TO_STRING(InvalidationMessage) {
  BEGIN();
  REPEATED(invalidation);
  END();
}

DEFINE_TO_STRING(RegistrationSubtree) {
  BEGIN();
  REPEATED(registered_object);
  END();
}
DEFINE_TO_STRING(RegistrationSyncMessage) {
  BEGIN();
  REPEATED(subtree);
  END();
}

DEFINE_TO_STRING(ClientToServerMessage) {
  BEGIN();
  OPTIONAL(header);
  OPTIONAL(initialize_message);
  OPTIONAL(registration_message);
  OPTIONAL(registration_sync_message);
  OPTIONAL(invalidation_ack_message);
  OPTIONAL(info_message);
  END();
}

DEFINE_TO_STRING(ServerHeader) {
  BEGIN();
  OPTIONAL(protocol_version);
  OPTIONAL(client_token);
  OPTIONAL(registration_summary);
  OPTIONAL(server_time_ms);
  OPTIONAL(message_id);
  END();
}

DEFINE_TO_STRING(TokenControlMessage) {
  BEGIN();
  OPTIONAL(new_token);
  END();
}

DEFINE_TO_STRING(RegistrationStatusMessage) {
  BEGIN();
  REPEATED(registration_status);
  END();
}

DEFINE_TO_STRING(ServerToClientMessage) {
  BEGIN();
  OPTIONAL(header);
  OPTIONAL(token_control_message);
  OPTIONAL(invalidation_message);
  OPTIONAL(registration_status_message);
  OPTIONAL(registration_sync_request_message);
  OPTIONAL(info_request_message);
  END();
}

DEFINE_TO_STRING(RegistrationManagerStateP) {
  BEGIN();
  OPTIONAL(client_summary);
  OPTIONAL(server_summary);
  REPEATED(registered_objects);
  END();
}

}  // namespace invalidation
