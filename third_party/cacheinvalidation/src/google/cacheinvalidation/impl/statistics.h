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

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_STATISTICS_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_STATISTICS_H_

#include <string>
#include <utility>
#include <vector>

#include "google/cacheinvalidation/deps/stl-namespace.h"
#include "google/cacheinvalidation/deps/string_util.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::pair;
using INVALIDATION_STL_NAMESPACE::string;
using INVALIDATION_STL_NAMESPACE::vector;

class Statistics {
 public:
  // Implementation: To classify the statistics a bit better, we have a few
  // enums to track different different types of statistics, e.g., sent message
  // types, errors, etc. For each statistic type, we create a map and provide a
  // method to record an event for each type of statistic.

  /* Types of messages sent to the server: ClientToServerMessage for their
   * description.
   */
  enum SentMessageType {
    SentMessageType_INFO,
    SentMessageType_INITIALIZE,
    SentMessageType_INVALIDATION_ACK,
    SentMessageType_REGISTRATION,
    SentMessageType_REGISTRATION_SYNC,
    SentMessageType_TOTAL,  // Refers to the actual ClientToServerMessage
                            // message sent on the network.
  };
  static const SentMessageType SentMessageType_MIN = SentMessageType_INFO;
  static const SentMessageType SentMessageType_MAX = SentMessageType_TOTAL;
  static const char* SentMessageType_names[];

  /* Types of messages received from the server: ServerToClientMessage for their
   * description.
   */
  enum ReceivedMessageType {
    ReceivedMessageType_INFO_REQUEST,
    ReceivedMessageType_INVALIDATION,
    ReceivedMessageType_REGISTRATION_STATUS,
    ReceivedMessageType_REGISTRATION_SYNC_REQUEST,
    ReceivedMessageType_TOKEN_CONTROL,
    ReceivedMessageType_ERROR,
    ReceivedMessageType_CONFIG_CHANGE,
    ReceivedMessageType_TOTAL,  // Refers to the actual ServerToClientMessage
                                // messages received from the network.
  };
  static const ReceivedMessageType ReceivedMessageType_MIN =
      ReceivedMessageType_INFO_REQUEST;
  static const ReceivedMessageType ReceivedMessageType_MAX =
      ReceivedMessageType_TOTAL;
  static const char* ReceivedMessageType_names[];

  /* Interesting API calls coming from the application (see InvalidationClient).
   */
  enum IncomingOperationType {
    IncomingOperationType_ACKNOWLEDGE,
    IncomingOperationType_REGISTRATION,
    IncomingOperationType_UNREGISTRATION,
  };
  static const IncomingOperationType IncomingOperationType_MIN =
      IncomingOperationType_ACKNOWLEDGE;
  static const IncomingOperationType IncomingOperationType_MAX =
      IncomingOperationType_UNREGISTRATION;
  static const char* IncomingOperationType_names[];

  /* Different types of events issued by the InvalidationListener. */
  enum ListenerEventType {
    ListenerEventType_INFORM_ERROR,
    ListenerEventType_INFORM_REGISTRATION_FAILURE,
    ListenerEventType_INFORM_REGISTRATION_STATUS,
    ListenerEventType_INVALIDATE,
    ListenerEventType_INVALIDATE_ALL,
    ListenerEventType_INVALIDATE_UNKNOWN,
    ListenerEventType_REISSUE_REGISTRATIONS,
  };
  static const ListenerEventType ListenerEventType_MIN =
      ListenerEventType_INFORM_ERROR;
  static const ListenerEventType ListenerEventType_MAX =
      ListenerEventType_REISSUE_REGISTRATIONS;
  static const char* ListenerEventType_names[];

  /* Different types of errors observed by the Ticl. */
  enum ClientErrorType {
    /* Acknowledge call received from client with a bad handle. */
    ClientErrorType_ACKNOWLEDGE_HANDLE_FAILURE,

    /* Incoming message dropped due to parsing, validation problems. */
    ClientErrorType_INCOMING_MESSAGE_FAILURE,

    /* Tried to send an outgoing message that was invalid. */
    ClientErrorType_OUTGOING_MESSAGE_FAILURE,

    /* Persistent state failed to deserialize correctly. */
    ClientErrorType_PERSISTENT_DESERIALIZATION_FAILURE,

    /* Read of blob from persistent state failed. */
    ClientErrorType_PERSISTENT_READ_FAILURE,

    /* Write of blob from persistent state failed. */
    ClientErrorType_PERSISTENT_WRITE_FAILURE,

    /* Message received with incompatible protocol version. */
    ClientErrorType_PROTOCOL_VERSION_FAILURE,

    /* Registration at client and server is different, e.g., client thinks it is
     * registered while the server says it is unregistered (of course, sync will
     * fix it).
     */
    ClientErrorType_REGISTRATION_DISCREPANCY,

    /* The nonce from the server did not match the current nonce by the client.
     */
    ClientErrorType_NONCE_MISMATCH,

    /* The current token at the client is different from the token in the
     * incoming message.
     */
    ClientErrorType_TOKEN_MISMATCH,

    /* No message sent due to token missing. */
    ClientErrorType_TOKEN_MISSING_FAILURE,

    /* Received a message with a token (transient) failure. */
    ClientErrorType_TOKEN_TRANSIENT_FAILURE,
  };
  static const ClientErrorType ClientErrorType_MIN =
      ClientErrorType_ACKNOWLEDGE_HANDLE_FAILURE;
  static const ClientErrorType ClientErrorType_MAX =
      ClientErrorType_TOKEN_TRANSIENT_FAILURE;
  static const char* ClientErrorType_names[];

  // Arrays for each type of Statistic to keep track of how many times each
  // event has occurred.

  Statistics();

  /* Returns the counter value for client_error_type. */
  int GetClientErrorCounterForTest(ClientErrorType client_error_type) {
    return client_error_types_[client_error_type];
  }

  /* Returns the counter value for sent_message_type. */
  int GetSentMessageCounterForTest(SentMessageType sent_message_type) {
    return sent_message_types_[sent_message_type];
  }

  /* Returns the counter value for received_message_type. */
  int GetReceivedMessageCounterForTest(
      ReceivedMessageType received_message_type) {
    return received_message_types_[received_message_type];
  }

  /* Records the fact that a message of type sent_message_type has been sent. */
  void RecordSentMessage(SentMessageType sent_message_type) {
    ++sent_message_types_[sent_message_type];
  }

  /* Records the fact that a message of type received_message_type has been
   * received.
   */
  void RecordReceivedMessage(ReceivedMessageType received_message_type) {
    ++received_message_types_[received_message_type];
  }

  /* Records the fact that the application has made a call of type
   * incoming_operation_type.
   */
  void RecordIncomingOperation(IncomingOperationType incoming_operation_type) {
    ++incoming_operation_types_[incoming_operation_type];
  }

  /* Records the fact that the listener has issued an event of type
   * listener_event_type.
   */
  void RecordListenerEvent(ListenerEventType listener_event_type) {
    ++listener_event_types_[listener_event_type];
  }

  /* Records the fact that the client has observed an error of type
   * client_error_type.
   */
  void RecordError(ClientErrorType client_error_type) {
    ++client_error_types_[client_error_type];
  }

  /* Modifies performance_counters to contain all the statistics that are
   * non-zero. Each pair has the name of the statistic event and the number of
   * times that event has occurred since the client started.
   */
  void GetNonZeroStatistics(vector<pair<string, int> >* performance_counters);

  /* Modifies result to contain those statistics from map whose value is > 0. */
  static void FillWithNonZeroStatistics(
      int map[], int size, const char* names[], const char* prefix,
      vector<pair<string, int> >* destination);

  /* Initialzes all values for keys in map to be 0. */
  static void InitializeMap(int map[], int size);

 private:
  int sent_message_types_[SentMessageType_MAX + 1];
  int received_message_types_[ReceivedMessageType_MAX + 1];
  int incoming_operation_types_[IncomingOperationType_MAX + 1];
  int listener_event_types_[ListenerEventType_MAX + 1];
  int client_error_types_[ClientErrorType_MAX + 1];
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_STATISTICS_H_
