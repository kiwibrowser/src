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

// Statistics for the Ticl, e.g., number of registration calls, number of token
// mismatches, etc.

#include "google/cacheinvalidation/impl/statistics.h"

namespace invalidation {

const char* Statistics::SentMessageType_names[] = {
  "INFO",
  "INITIALIZE",
  "INVALIDATION_ACK",
  "REGISTRATION",
  "REGISTRATION_SYNC",
  "TOTAL",
};

const char* Statistics::ReceivedMessageType_names[] = {
  "INFO_REQUEST",
  "INVALIDATION",
  "REGISTRATION_STATUS",
  "REGISTRATION_SYNC_REQUEST",
  "TOKEN_CONTROL",
  "ERROR",
  "CONFIG_CHANGE",
  "TOTAL",
};

const char* Statistics::IncomingOperationType_names[] = {
  "ACKNOWLEDGE",
  "REGISTRATION",
  "UNREGISTRATION",
};

const char* Statistics::ListenerEventType_names[] = {
  "INFORM_ERROR",
  "INFORM_REGISTRATION_FAILURE",
  "INFORM_REGISTRATION_STATUS",
  "INVALIDATE",
  "INVALIDATE_ALL",
  "INVALIDATE_UNKNOWN",
  "REISSUE_REGISTRATIONS",
};

const char* Statistics::ClientErrorType_names[] = {
  "ACKNOWLEDGE_HANDLE_FAILURE",
  "INCOMING_MESSAGE_FAILURE",
  "OUTGOING_MESSAGE_FAILURE",
  "PERSISTENT_DESERIALIZATION_FAILURE",
  "PERSISTENT_READ_FAILURE",
  "PERSISTENT_WRITE_FAILURE",
  "PROTOCOL_VERSION_FAILURE",
  "REGISTRATION_DISCREPANCY",
  "NONCE_MISMATCH",
  "TOKEN_MISMATCH",
  "TOKEN_MISSING_FAILURE",
  "TOKEN_TRANSIENT_FAILURE",
};

Statistics::Statistics() {
  InitializeMap(sent_message_types_, SentMessageType_MAX + 1);
  InitializeMap(received_message_types_, ReceivedMessageType_MAX + 1);
  InitializeMap(incoming_operation_types_, IncomingOperationType_MAX + 1);
  InitializeMap(listener_event_types_, ListenerEventType_MAX + 1);
  InitializeMap(client_error_types_, ClientErrorType_MAX + 1);
}

void Statistics::GetNonZeroStatistics(
    vector<pair<string, int> >* performance_counters) {
  // Add the non-zero values from the different maps to performance_counters.
  FillWithNonZeroStatistics(
      sent_message_types_, SentMessageType_MAX + 1, SentMessageType_names,
      "SentMessageType.", performance_counters);
  FillWithNonZeroStatistics(
      received_message_types_, ReceivedMessageType_MAX + 1,
      ReceivedMessageType_names, "ReceivedMessageType.",
      performance_counters);
  FillWithNonZeroStatistics(
      incoming_operation_types_, IncomingOperationType_MAX + 1,
      IncomingOperationType_names, "IncomingOperationType.",
      performance_counters);
  FillWithNonZeroStatistics(
      listener_event_types_, ListenerEventType_MAX + 1, ListenerEventType_names,
      "ListenerEventType.", performance_counters);
  FillWithNonZeroStatistics(
      client_error_types_, ClientErrorType_MAX + 1, ClientErrorType_names,
      "ClientErrorType.", performance_counters);
}

/* Modifies result to contain those statistics from map whose value is > 0. */
void Statistics::FillWithNonZeroStatistics(
    int map[], int size, const char* names[], const char* prefix,
    vector<pair<string, int> >* destination) {
  for (int i = 0; i < size; ++i) {
    if (map[i] > 0) {
      destination->push_back(
          make_pair(StringPrintf("%s%s", prefix, names[i]), map[i]));
    }
  }
}

void Statistics::InitializeMap(int map[], int size) {
  for (int i = 0; i < size; ++i) {
    map[i] = 0;
  }
}

}  // namespace invalidation
