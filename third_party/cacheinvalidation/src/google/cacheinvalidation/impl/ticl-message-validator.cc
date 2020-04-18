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

// Validator for v2 protocol messages.

#include "google/cacheinvalidation/impl/ticl-message-validator.h"

#include <stdint.h>

#include "google/cacheinvalidation/impl/log-macro.h"
#include "google/cacheinvalidation/impl/proto-helpers.h"
#include "google/cacheinvalidation/include/system-resources.h"

namespace invalidation {

// High-level design: validation works via the collaboration of a set of macros
// and template method specializations that obey a specific protocol.  A
// validator for a particular type is defined by a specialization of the method:
//
// template<typename T>
// void TiclMessageValidator::Validate(const T& message, bool* result);
//
// A macro, DEFINE_VALIDATOR(type) is defined below to help prevent mistakes in
// these definitions and to improve code readability.  For example, to define
// the validator for the type ObjectIdP, we'd write:
//
// DEFINE_VALIDATOR(ObjectIdP) { /* validation constraints ... */ }
//
// The choice of the names |message| and |result| is significant, as many of the
// macros assume that these refer respectively to the message being validated
// and the address in which the validation result is to be stored.
//
// When a validator is called, |*result| is initially |true|.  To reject the
// message, the validator sets |*result| to |false| and returns.  Otherwise, it
// simply allows control flow to continue; if no reason is found to reject the
// message, control eventually returns to the caller with |*result| still set to
// |true|, indicating that the message is acceptable.  This protocol keeps the
// bodies of the validation methods clean--otherwise they would all need need to
// end with explicit |return| statements.
//
// A validator typically consists of a collection of constraints, at least one
// per field in the message.  Several macros are defined for common constraints,
// including:
//
// REQUIRE(field): requires that (optional) |field| be present and valid.
// ALLOW(field): allows (optional) |field| if valid.
// ZERO_OR_MORE(field): validates each element of the (repeated) |field|.
// ONE_OR_MORE(field): like ZERO_OR_MORE, but requires at least one element.
// NON_EMPTY(field): checks that the string |field| is non-empty (if present).
// NON_NEGATIVE(field): checks that the integral |field| is >= 0 (if present).
//
// For custom constraints, the CONDITION(expr) macro allows an arbitrary boolean
// expression, which will generally refer to |message|.
//
// Note that REQUIRE, ALLOW, ZERO_OR_MORE, and ONE_OR_MORE all perform recursive
// validation of the mentioned fields.  A validation method must therefore be
// defined for the type of the field, or there will be a link-time error.


// Macros:

// Macro to define a specialization of the |Validate| method for the given
// |type|.  This must be followed by a method body in curly braces defining
// constraints on |message|, which is bound to a value of the given type.  If
// |message| is valid, no action is necessary; if invalid, a diagnostic message
// should be logged via |logger_|, and |*result| should be set to false.
#define DEFINE_VALIDATOR(type)                                          \
  template<>                                                            \
  void TiclMessageValidator::Validate(const type& message, bool* result)

// Expands into a conditional that checks whether |field| is present in
// |message| and valid.
#define REQUIRE(field)                                                  \
  if (!message.has_##field()) {                                         \
    TLOG(logger_, SEVERE, "required field " #field " missing from %s",  \
         ProtoHelpers::ToString(message).c_str());                      \
    *result = false;                                                    \
    return;                                                             \
  }                                                                     \
  ALLOW(field);

// Expands into a conditional that checks whether |field| is present in
// |message|.  If so, validates |message.field()|; otherwise, does nothing.
#define ALLOW(field)                                                    \
  if (message.has_##field()) {                                          \
    Validate(message.field(), result);                                  \
    if (!*result) {                                                     \
      TLOG(logger_, SEVERE, "field " #field " failed validation in %s", \
           ProtoHelpers::ToString(message).c_str());                    \
      return;                                                           \
    }                                                                   \
  }

// Expands into a conditional that checks that, if |field| is present in
// |message|, then it is greater than or equal to |value|.
#define GREATER_OR_EQUAL(field, value)                                  \
  if (message.has_##field() && (message.field() < value)) {             \
    TLOG(logger_, SEVERE,                                               \
         #field " must be greater than or equal to %d; was %d",         \
         value, message.field());                                       \
    *result = false;                                                    \
    return;                                                             \
  }

// Expands into a conditional that checks that, if the specified numeric |field|
// is present, that it is non-negative.
#define NON_NEGATIVE(field) GREATER_OR_EQUAL(field, 0)

// Expands into a conditional that checks that, if the specified string |field|
// is present, that it is non-empty.
#define NON_EMPTY(field)                                                \
  if (message.has_##field() && message.field().empty()) {               \
    TLOG(logger_, SEVERE, #field " must be non-empty");                 \
    *result = false;                                                    \
    return;                                                             \
  }

// Expands into a loop that checks that all elements of the repeated |field| are
// valid.
#define ZERO_OR_MORE(field)                                             \
  for (int i = 0; i < message.field##_size(); ++i) {                    \
    Validate(message.field(i), result);                                 \
    if (!*result) {                                                     \
      TLOG(logger_, SEVERE, "field " #field " #%d failed validation in %s", \
           i, ProtoHelpers::ToString(message).c_str());                 \
      *result = false;                                                  \
      return;                                                           \
    }                                                                   \
  }

// Expands into a loop that checks that there is at least one element of the
// repeated |field|, and that all are valid.
#define ONE_OR_MORE(field)                                              \
  if (message.field##_size() == 0) {                                    \
    TLOG(logger_, SEVERE, "at least one " #field " required in %s",     \
         ProtoHelpers::ToString(message).c_str());                      \
    *result = false;                                                    \
    return;                                                             \
  }                                                                     \
  ZERO_OR_MORE(field)

// Expands into code that checks that the arbitrary condition |expr| is true.
#define CONDITION(expr)                                 \
  *result = expr;                                       \
  if (!*result) {                                       \
    TLOG(logger_, SEVERE, #expr " not satisfied by %s", \
       ProtoHelpers::ToString(message).c_str());        \
    return;                                             \
  }


// Validators:

// No constraints on primitive types by default.
DEFINE_VALIDATOR(bool) {}
DEFINE_VALIDATOR(int) {}
DEFINE_VALIDATOR(int64_t) {}
DEFINE_VALIDATOR(string) {}

// Similarly, for now enum values are always considered valid.
DEFINE_VALIDATOR(ErrorMessage::Code) {}
DEFINE_VALIDATOR(InfoRequestMessage::InfoType) {}
DEFINE_VALIDATOR(InitializeMessage::DigestSerializationType) {}
DEFINE_VALIDATOR(RegistrationP::OpType) {}
DEFINE_VALIDATOR(StatusP::Code) {}

DEFINE_VALIDATOR(Version) {
  REQUIRE(major_version);
  NON_NEGATIVE(major_version);
  REQUIRE(minor_version);
  NON_NEGATIVE(minor_version);
}

DEFINE_VALIDATOR(ProtocolVersion) {
  REQUIRE(version);
}

DEFINE_VALIDATOR(ObjectIdP) {
  REQUIRE(name);
  REQUIRE(source);
  NON_NEGATIVE(source);
}

DEFINE_VALIDATOR(InvalidationP) {
  REQUIRE(object_id);
  REQUIRE(is_known_version);
  REQUIRE(version);
  NON_NEGATIVE(version);
  ALLOW(payload);
}

DEFINE_VALIDATOR(RegistrationP) {
  REQUIRE(object_id);
  REQUIRE(op_type);
}

DEFINE_VALIDATOR(RegistrationSummary) {
  REQUIRE(num_registrations);
  NON_NEGATIVE(num_registrations);
  REQUIRE(registration_digest);
  NON_EMPTY(registration_digest);
}

DEFINE_VALIDATOR(InvalidationMessage) {
  ONE_OR_MORE(invalidation);
}

DEFINE_VALIDATOR(ClientHeader) {
  REQUIRE(protocol_version);
  ALLOW(client_token);
  NON_EMPTY(client_token);
  ALLOW(registration_summary);
  REQUIRE(client_time_ms);
  REQUIRE(max_known_server_time_ms);
  ALLOW(message_id);
  ALLOW(client_type);
}

DEFINE_VALIDATOR(ApplicationClientIdP) {
  REQUIRE(client_type);
  REQUIRE(client_name);
  NON_EMPTY(client_name);
}

DEFINE_VALIDATOR(InitializeMessage) {
  REQUIRE(client_type);
  REQUIRE(nonce);
  NON_EMPTY(nonce);
  REQUIRE(digest_serialization_type);
  REQUIRE(application_client_id);
}

DEFINE_VALIDATOR(RegistrationMessage) {
  ONE_OR_MORE(registration);
}

DEFINE_VALIDATOR(ClientVersion) {
  REQUIRE(version);
  REQUIRE(platform);
  REQUIRE(language);
  REQUIRE(application_info);
}

DEFINE_VALIDATOR(PropertyRecord) {
  REQUIRE(name);
  REQUIRE(value);
}

DEFINE_VALIDATOR(RateLimitP) {
  REQUIRE(window_ms);
  GREATER_OR_EQUAL(window_ms, 1000);
  CONDITION(message.window_ms() > message.count());
  REQUIRE(count);
}

DEFINE_VALIDATOR(ProtocolHandlerConfigP) {
  ALLOW(batching_delay_ms);
  ZERO_OR_MORE(rate_limit);
}

DEFINE_VALIDATOR(ClientConfigP) {
  REQUIRE(version);
  ALLOW(network_timeout_delay_ms);
  ALLOW(write_retry_delay_ms);
  ALLOW(heartbeat_interval_ms);
  ALLOW(perf_counter_delay_ms);
  ALLOW(max_exponential_backoff_factor);
  ALLOW(smear_percent);
  ALLOW(is_transient);
  ALLOW(initial_persistent_heartbeat_delay_ms);
  ALLOW(channel_supports_offline_delivery);
  REQUIRE(protocol_handler_config);
  ALLOW(offline_heartbeat_threshold_ms);
  ALLOW(allow_suppression);
}

DEFINE_VALIDATOR(InfoMessage) {
  REQUIRE(client_version);
  ZERO_OR_MORE(config_parameter);
  ZERO_OR_MORE(performance_counter);
  ALLOW(client_config);
  ALLOW(server_registration_summary_requested);
}

DEFINE_VALIDATOR(RegistrationSubtree) {
  ZERO_OR_MORE(registered_object);
}

DEFINE_VALIDATOR(RegistrationSyncMessage) {
  ONE_OR_MORE(subtree);
}

DEFINE_VALIDATOR(ClientToServerMessage) {
  REQUIRE(header);
  ALLOW(info_message);
  ALLOW(initialize_message);
  ALLOW(invalidation_ack_message);
  ALLOW(registration_message);
  ALLOW(registration_sync_message);
  CONDITION(message.has_initialize_message() ^
            message.header().has_client_token());
}

DEFINE_VALIDATOR(ServerHeader) {
  REQUIRE(protocol_version);
  REQUIRE(client_token);
  NON_EMPTY(client_token);
  ALLOW(registration_summary);
  REQUIRE(server_time_ms);
  NON_NEGATIVE(server_time_ms);
  ALLOW(message_id);
  NON_EMPTY(message_id);
}

DEFINE_VALIDATOR(StatusP) {
  REQUIRE(code);
  ALLOW(description);
}

DEFINE_VALIDATOR(TokenControlMessage) {
  ALLOW(new_token);
}

DEFINE_VALIDATOR(ErrorMessage) {
  REQUIRE(code);
  REQUIRE(description);
}

DEFINE_VALIDATOR(RegistrationStatus) {
  REQUIRE(registration);
  REQUIRE(status);
}

DEFINE_VALIDATOR(RegistrationStatusMessage) {
  ONE_OR_MORE(registration_status);
}

DEFINE_VALIDATOR(RegistrationSyncRequestMessage) {}

DEFINE_VALIDATOR(InfoRequestMessage) {
  ONE_OR_MORE(info_type);
}

DEFINE_VALIDATOR(ConfigChangeMessage) {
  ALLOW(next_message_delay_ms);
  GREATER_OR_EQUAL(next_message_delay_ms, 1);
}

DEFINE_VALIDATOR(ServerToClientMessage) {
  REQUIRE(header);
  ALLOW(token_control_message);
  ALLOW(invalidation_message);
  ALLOW(registration_status_message);
  ALLOW(registration_sync_request_message);
  ALLOW(config_change_message);
  ALLOW(info_request_message);
  ALLOW(error_message);
}

}  // namespace invalidation
