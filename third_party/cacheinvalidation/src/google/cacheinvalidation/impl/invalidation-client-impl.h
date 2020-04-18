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

// Implementation of the Invalidation Client Library (Ticl).

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_INVALIDATION_CLIENT_IMPL_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_INVALIDATION_CLIENT_IMPL_H_

#include <string>
#include <utility>

#include "base/macros.h"
#include "google/cacheinvalidation/include/invalidation-client.h"
#include "google/cacheinvalidation/include/invalidation-listener.h"
#include "google/cacheinvalidation/impl/checking-invalidation-listener.h"
#include "google/cacheinvalidation/impl/invalidation-client-core.h"
#include "google/cacheinvalidation/impl/protocol-handler.h"

namespace invalidation {

class InvalidationClientImpl : public InvalidationClientCore {
 public:
  /* Constructs a client.
   *
   * Arguments:
   * resources - resources to use during execution
   * random - a random number generator (owned by this after the call)
   * client_type - client type code
   * client_name - application identifier for the client
   * config - configuration for the client
   * listener - application callback
   */
  InvalidationClientImpl(
      SystemResources* resources, Random* random, int client_type,
      const string& client_name, const ClientConfigP &config,
      const string& application_name, InvalidationListener* listener);

  // These methods override those in InvalidationClientCore. Their
  // implementations all enqueue an event onto the work queue and
  // then delegate to the InvalidationClientCore method through one
  // of the private DoYYY functions (below).

  virtual void Start();

  virtual void Stop();

  virtual void Register(const ObjectId& object_id);

  virtual void Unregister(const ObjectId& object_id);

  virtual void Register(const vector<ObjectId>& object_ids);

  virtual void Unregister(const vector<ObjectId>& object_ids);

  virtual void Acknowledge(const AckHandle& acknowledge_handle);

  /* Returns the listener that was registered by the caller. */
  InvalidationListener* GetInvalidationListenerForTest() {
    return listener_.get()->delegate();
  }

 protected:
  virtual InvalidationListener* GetListener() {
    return listener_.get();
  }

 private:
  /*
   * All of these methods simply delegate to the superclass implementation. They
   * exist so that NewPermanentCallback objects created in
   * invalidation-client-impl.cc can call superclass methods.
   */
  void DoStart() {
    this->InvalidationClientCore::Start();
  }

  void DoStop() {
    this->InvalidationClientCore::Stop();
  }

  void DoRegister(const ObjectId& object_id) {
    this->InvalidationClientCore::Register(object_id);
  }

  void DoUnregister(const ObjectId& object_id) {
    this->InvalidationClientCore::Unregister(object_id);
  }

  void DoBulkRegister(const vector<ObjectId>& object_ids) {
    this->InvalidationClientCore::Register(object_ids);
  }

  void DoBulkUnregister(const vector<ObjectId>& object_ids) {
    this->InvalidationClientCore::Unregister(object_ids);
  }

  void DoAcknowledge(const AckHandle& acknowledge_handle) {
    this->InvalidationClientCore::Acknowledge(acknowledge_handle);
  }

  /*
   * The listener registered by the application, wrapped in a
   * CheckingInvalidationListener.
   */
  scoped_ptr<CheckingInvalidationListener> listener_;

  DISALLOW_COPY_AND_ASSIGN(InvalidationClientImpl);
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_INVALIDATION_CLIENT_IMPL_H_
