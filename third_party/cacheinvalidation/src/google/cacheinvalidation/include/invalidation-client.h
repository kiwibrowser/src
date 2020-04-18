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

// Interface for the invalidation client library.

#ifndef GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_CLIENT_H_
#define GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_CLIENT_H_

#include <vector>

#include "google/cacheinvalidation/deps/stl-namespace.h"

namespace invalidation {

using ::INVALIDATION_STL_NAMESPACE::vector;

class AckHandle;
class Invalidation;
class ObjectId;

class InvalidationClient {
 public:
  virtual ~InvalidationClient() {}

  /* Starts the client. This method must be called before any other method is
   * invoked. The client is considered to be started after
   * InvalidationListener::Ready has received by the application.
   *
   * REQUIRES: Start has not already been called.
   * The resources given to the client must have been started by the caller.
   */
  virtual void Start() = 0;

  /* Stops the client. After this method has been called, it is an error to call
   * any other method.
   *
   * REQUIRES: Start has already been called.
   * Does not stop the resources bound to this client.
   */
  virtual void Stop() = 0;

  /* Requests that the Ticl register to receive notifications for the object
   * with id object_id. The library guarantees that the caller will be informed
   * of the results of this call either via
   * InvalidationListener::InformRegistrationStatus or
   * InvalidationListener::InformRegistrationFailure unless the library informs
   * the caller of a connection failure via
   * InvalidationListener::InformError. The caller should consider the
   * registration to have succeeded only if it gets a call
   * InvalidationListener::InformRegistrationStatus for object_id with
   * InvalidationListener::RegistrationState::REGISTERED.  Note that if the
   * network is disconnected, the listener events will probably show up when the
   * network connection is repaired.
   *
   * REQUIRES: Start has been called and and InvalidationListener::Ready has
   * been received by the application's listener.
   */
  virtual void Register(const ObjectId& object_id) = 0;

  /* Registrations for multiple objects. See the specs on Register(const
   * ObjectId&) for more details. If the caller needs to register for a number
   * of object ids, this method is more efficient than calling Register in a
   * loop.
   */
  virtual void Register(const vector<ObjectId>& object_ids) = 0;

  /* Requests that the Ticl unregister for notifications for the object with id
   * object_id.  The library guarantees that the caller will be informed of the
   * results of this call either via
   * InvalidationListener::InformRegistrationStatus or
   * InvalidationListener::InformRegistrationFailure unless the library informs
   * the caller of a connection failure via
   * InvalidationListener::InformError. The caller should consider the
   * unregistration to have succeeded only if it gets a call
   * InvalidationListener::InformRegistrationStatus for object_id with
   * InvalidationListener::RegistrationState::UNREGISTERED.  Note that if the
   * network is disconnected, the listener events will probably show up when the
   * network connection is repaired.
   *
   * REQUIRES: Start has been called and and InvalidationListener::Ready has
   * been receiveed by the application's listener.
   */
  virtual void Unregister(const ObjectId& object_id) = 0;

  /* Unregistrations for multiple objects. See the specs on Unregister(const
   * ObjectId&) for more details. If the caller needs to unregister for a number
   * of object ids, this method is more efficient than calling Unregister in a
   * loop.
   */
  virtual void Unregister(const vector<ObjectId>& object_ids) = 0;

  /* Acknowledges the InvalidationListener event that was delivered with the
   * provided acknowledgement handle. This indicates that the client has
   * accepted responsibility for processing the event and it does not need to be
   * redelivered later.
   *
   * REQUIRES: Start been called and and InvalidationListener::Ready has been
   * received by the application's listener.
   */
  virtual void Acknowledge(const AckHandle& ackHandle) = 0;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_INCLUDE_INVALIDATION_CLIENT_H_
