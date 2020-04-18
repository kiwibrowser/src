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

// Interface through which invalidation-related events are delivered by the
// library to the application. Each event must be acknowledged by the
// application. Each includes an AckHandle that the application must use to call
// InvalidationClient::Acknowledge after it is done handling that event.

#ifndef GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_LISTENER_H_
#define GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_LISTENER_H_

#include <string>

#include "google/cacheinvalidation/deps/stl-namespace.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::string;

class AckHandle;
class ErrorInfo;
class Invalidation;
class InvalidationClient;
class ObjectId;

class InvalidationListener {
 public:
  /* Possible registration states for an object. */
  enum RegistrationState {
    REGISTERED,
    UNREGISTERED
  };

  virtual ~InvalidationListener() {}

  /* Called in response to the InvalidationClient::Start call. Indicates that
   * the InvalidationClient is now ready for use, i.e., calls such as
   * register/unregister can be performed on that object.
   *
   * Arguments:
   *     client - the InvalidationClient invoking the listener
   */
  virtual void Ready(InvalidationClient* client) = 0;

  /* Indicates that an object has been updated to a particular version.
   *
   * The Ticl guarantees that this callback will be invoked at least once for
   * every invalidation that it guaranteed to deliver. It does not guarantee
   * exactly-once delivery or in-order delivery (with respect to the version
   * number).
   *
   * The application should acknowledge this event by calling
   * InvalidationClient::Acknowledge(const AckHandle&) with the provided
   * ack_handle otherwise the event may be redelivered.
   *
   * Arguments:
   *     client - the InvalidationClient invoking the listener
   *     ack_handle - event acknowledgement handle
   */
  virtual void Invalidate(InvalidationClient* client,
                          const Invalidation& invalidation,
                          const AckHandle& ack_handle) = 0;

  /* As Invalidate, but for an unknown application store version. The object may
   * or may not have been updated - to ensure that the application does not miss
   * an update from its backend, the application must check and/or fetch the
   * latest version from its store.
   */
  virtual void InvalidateUnknownVersion(InvalidationClient* client,
                                        const ObjectId& object_id,
                                        const AckHandle& ack_handle) = 0;

  /* Indicates that the application should consider all objects to have changed.
   * This event is generally sent when the client has been disconnected from the
   * network for too long a period and has been unable to resynchronize with the
   * update stream, but it may be invoked arbitrarily (although the service
   * tries hard not to invoke it under normal circumstances).
   *
   * The application should acknowledge this event by calling
   * InvalidationClient::Acknowledge(const AckHandle&) with the provided
   * ack_handle otherwise the event may be redelivered.
   *
   * Arguments:
   *     client - the InvalidationClient invoking the listener
   *     ack_handle - event acknowledgement handle
   */
  virtual void InvalidateAll(InvalidationClient* client,
                             const AckHandle& ack_handle) = 0;

  /* Indicates that the registration state of an object has changed.
   *
   * The application should acknowledge this event by calling
   * InvalidationClient::Acknowledge(AckHandle) with the provided ack_handle;
   * otherwise the event may be redelivered.
   *
   * Arguments:
   *     client - the InvalidationClient invoking the listener
   *     object_id - the id of the object whose state changed
   *     reg_state - the new state
   */
  virtual void InformRegistrationStatus(InvalidationClient* client,
                                        const ObjectId& object_id,
                                        RegistrationState reg_state) = 0;

  /* Indicates that an object registration or unregistration operation may have
   * failed.
   *
   * The application should acknowledge this event by calling
   * InvalidationClient::acknowledge(AckHandle) with the provided ack_handle;
   * otherwise the event may be redelivered.
   *
   * For transient failures, the application can retry the registration later -
   * if it chooses to do so, it must use a sensible backoff policy such as
   * exponential backoff. For permanent failures, it must not automatically
   * retry without fixing the situation (e.g., by presenting a dialog box to the
   * user).
   *
   * Arguments:
   *     client - the {@link InvalidationClient} invoking the listener
   *     object_id - the id of the object whose state changed
   *     is_transient - whether the error is transient or permanent
   *     errorMessage - extra information about the message
   */
  virtual void InformRegistrationFailure(InvalidationClient* client,
                                         const ObjectId& object_id,
                                         bool is_transient,
                                         const string& error_message) = 0;

  /* Indicates that the all registrations for the client are in an unknown state
   * (e.g., they could have been removed). The application MUST inform the
   * InvalidationClient of its registrations once it receives this event.  The
   * requested objects are those for which the digest of their serialized object
   * ids matches a particular prefix bit-pattern. The digest for an object id is
   * computed as following (the digest chosen for this method is SHA-1):
   *
   *   Digest digest();
   *   digest.Update(Little endian encoding of object source type)
   *   digest.Update(object name)
   *   digest.GetDigestSummary()
   *
   * For a set of objects, digest is computed by sorting lexicographically based
   * on their digests and then performing the update process given above (i.e.,
   * calling digest.update on each object's digest and then calling
   * getDigestSummary at the end).
   *
   * IMPORTANT: A client can always register for more objects than what is
   * requested here. For example, in response to this call, the client can
   * ignore the prefix parameters and register for all its objects.
   *
   * The application should acknowledge this event by calling
   * InvalidationClient::Acknowledge(const AckHandle&) with the provided
   * ack_handle otherwise the event may be redelivered. The acknowledge using
   * ack_handle must be called after all the InvalidationClient::Register calls
   * have been made.
   *
   * Arguments:
   *     client - the InvalidationClient invoking the listener
   *     prefix - prefix of the object ids as described above.
   *     prefix_length - number of bits in prefix to consider.
   */
  virtual void ReissueRegistrations(InvalidationClient* client,
                                    const string& prefix,
                                    int prefix_length) = 0;

  /* Informs the listener about errors that have occurred in the backend, e.g.,
   * authentication, authorization problems.
   *
   * The application should acknowledge this event by calling
   * InvalidationClient::Acknowledge(const AckHandle&) with the provided
   * ack_handle otherwise the event may be redelivered.
   *
   * Arguments:
   *     client - the InvalidationClient invoking the listener
   *     error_info - information about the error
   */
  virtual void InformError(InvalidationClient* client,
                           const ErrorInfo& error_info) = 0;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_LISTENER_H_
