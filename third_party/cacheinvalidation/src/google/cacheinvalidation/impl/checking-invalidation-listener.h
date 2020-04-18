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

// InvalidationListener wrapper that ensures that a delegate listener is called
// on the proper thread and calls the listener method on the listener thread.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_CHECKING_INVALIDATION_LISTENER_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_CHECKING_INVALIDATION_LISTENER_H_

#include "google/cacheinvalidation/include/invalidation-client.h"
#include "google/cacheinvalidation/include/invalidation-listener.h"
#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/impl/statistics.h"

namespace invalidation {

class CheckingInvalidationListener : public InvalidationListener {
 public:
  CheckingInvalidationListener(
      InvalidationListener* delegate, Statistics* statistics,
      Scheduler* internal_scheduler, Scheduler* listener_scheduler,
      Logger* logger);

  virtual ~CheckingInvalidationListener() {}

  virtual void Invalidate(
      InvalidationClient* client, const Invalidation& invalidation,
      const AckHandle& ack_handle);

  virtual void InvalidateUnknownVersion(
      InvalidationClient* client, const ObjectId& object_id,
      const AckHandle& ack_handle);

  virtual void InvalidateAll(
      InvalidationClient* client, const AckHandle& ack_handle);

  virtual void InformRegistrationFailure(
      InvalidationClient* client, const ObjectId& object_id,
      bool is_transient, const string& error_message);

  virtual void InformRegistrationStatus(
      InvalidationClient* client, const ObjectId& object_id,
      RegistrationState reg_state);

  virtual void ReissueRegistrations(
      InvalidationClient* client, const string& prefix, int prefix_len);

  virtual void InformError(
      InvalidationClient* client, const ErrorInfo& error_info);

  /* Returns the delegate InvalidationListener. */
  InvalidationListener* delegate() {
    return delegate_;
  }

  virtual void Ready(InvalidationClient* client);

 private:
  /* The actual listener to which this listener delegates. */
  InvalidationListener* delegate_;

  /* Statistics objects to track number of sent messages, etc. */
  Statistics* statistics_;

  /* The scheduler for scheduling internal events in the library. */
  Scheduler* internal_scheduler_;

  /* The scheduler for scheduling events for the delegate. */
  Scheduler* listener_scheduler_;

  Logger* logger_;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_CHECKING_INVALIDATION_LISTENER_H_
