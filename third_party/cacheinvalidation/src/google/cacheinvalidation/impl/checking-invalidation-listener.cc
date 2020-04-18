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

#include "google/cacheinvalidation/impl/checking-invalidation-listener.h"
#include "google/cacheinvalidation/impl/log-macro.h"

namespace invalidation {

CheckingInvalidationListener::CheckingInvalidationListener(
    InvalidationListener* delegate, Statistics* statistics,
    Scheduler* internal_scheduler, Scheduler* listener_scheduler,
    Logger* logger)
    : delegate_(delegate),
      statistics_(statistics),
      internal_scheduler_(internal_scheduler),
      listener_scheduler_(listener_scheduler),
      logger_(logger) {
  CHECK(delegate != NULL);
  CHECK(statistics != NULL);
  CHECK(internal_scheduler_ != NULL);
  CHECK(listener_scheduler != NULL);
  CHECK(logger != NULL);
}

void CheckingInvalidationListener::Invalidate(
    InvalidationClient* client, const Invalidation& invalidation,
    const AckHandle& ack_handle) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  statistics_->RecordListenerEvent(Statistics::ListenerEventType_INVALIDATE);
  listener_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          delegate_, &InvalidationListener::Invalidate, client, invalidation,
          ack_handle));
}

void CheckingInvalidationListener::InvalidateUnknownVersion(
    InvalidationClient* client, const ObjectId& object_id,
    const AckHandle& ack_handle) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  statistics_->RecordListenerEvent(
      Statistics::ListenerEventType_INVALIDATE_UNKNOWN);
  listener_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          delegate_, &InvalidationListener::InvalidateUnknownVersion, client,
          object_id, ack_handle));
}

void CheckingInvalidationListener::InvalidateAll(
    InvalidationClient* client, const AckHandle& ack_handle) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  statistics_->RecordListenerEvent(
      Statistics::ListenerEventType_INVALIDATE_ALL);
  listener_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          delegate_, &InvalidationListener::InvalidateAll, client,
          ack_handle));
}

void CheckingInvalidationListener::InformRegistrationFailure(
    InvalidationClient* client, const ObjectId& object_id,
    bool is_transient, const string& error_message) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  statistics_->RecordListenerEvent(
      Statistics::ListenerEventType_INFORM_REGISTRATION_FAILURE);
  listener_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          delegate_, &InvalidationListener::InformRegistrationFailure, client,
          object_id, is_transient, error_message));
}

void CheckingInvalidationListener::InformRegistrationStatus(
    InvalidationClient* client, const ObjectId& object_id,
    RegistrationState reg_state) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  statistics_->RecordListenerEvent(
      Statistics::ListenerEventType_INFORM_REGISTRATION_STATUS);
  listener_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          delegate_, &InvalidationListener::InformRegistrationStatus, client,
          object_id, reg_state));
}

void CheckingInvalidationListener::ReissueRegistrations(
    InvalidationClient* client, const string& prefix, int prefix_len) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  statistics_->RecordListenerEvent(
      Statistics::ListenerEventType_REISSUE_REGISTRATIONS);
  listener_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          delegate_, &InvalidationListener::ReissueRegistrations,
          client, prefix, prefix_len));
}

void CheckingInvalidationListener::InformError(
    InvalidationClient* client, const ErrorInfo& error_info) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  statistics_->RecordListenerEvent(
      Statistics::ListenerEventType_INFORM_ERROR);
  listener_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          delegate_, &InvalidationListener::InformError, client, error_info));
}

void CheckingInvalidationListener::Ready(InvalidationClient* client) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  TLOG(logger_, INFO, "Informing app that ticl is ready");
  listener_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(delegate_, &InvalidationListener::Ready, client));
}

}  // namespace invalidation
