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

// Object to track desired client registrations. This class belongs to caller
// (e.g., InvalidationClientImpl) and is not thread-safe - the caller has to use
// this class in a thread-safe manner.

#include "google/cacheinvalidation/impl/registration-manager.h"

#include <stddef.h>

#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"
#include "google/cacheinvalidation/impl/log-macro.h"
#include "google/cacheinvalidation/impl/proto-helpers.h"
#include "google/cacheinvalidation/impl/simple-registration-store.h"

namespace invalidation {

RegistrationManager::RegistrationManager(
    Logger* logger, Statistics* statistics, DigestFunction* digest_function)
    : desired_registrations_(new SimpleRegistrationStore(digest_function)),
      statistics_(statistics),
      logger_(logger) {
  // Initialize the server summary with a 0 size and the digest corresponding to
  // it.  Using defaultInstance would wrong since the server digest will not
  // match unnecessarily and result in an info message being sent.
  GetClientSummary(&last_known_server_summary_);
}

void RegistrationManager::PerformOperations(
    const vector<ObjectIdP>& object_ids, RegistrationP::OpType reg_op_type,
    vector<ObjectIdP>* oids_to_send) {
  // Record that we have pending operations on the objects.
  vector<ObjectIdP>::const_iterator iter = object_ids.begin();
  for (; iter != object_ids.end(); iter++) {
    pending_operations_[*iter] = reg_op_type;
  }
  // Update the digest appropriately.
  if (reg_op_type == RegistrationP_OpType_REGISTER) {
    desired_registrations_->Add(object_ids, oids_to_send);
  } else {
    desired_registrations_->Remove(object_ids, oids_to_send);
  }
}

void RegistrationManager::GetRegistrations(
    const string& digest_prefix, int prefix_len, RegistrationSubtree* builder) {
  vector<ObjectIdP> oids;
  desired_registrations_->GetElements(digest_prefix, prefix_len, &oids);
  for (size_t i = 0; i < oids.size(); ++i) {
    builder->add_registered_object()->CopyFrom(oids[i]);
  }
}

void RegistrationManager::HandleRegistrationStatus(
    const RepeatedPtrField<RegistrationStatus>& registration_statuses,
    vector<bool>* success_status) {

  // Local-processing result code for each element of
  // registrationStatuses. Indicates whether the registration status was
  // compatible with the client's desired state (e.g., a successful unregister
  // from the server when we desire a registration is incompatible).
  for (int i = 0; i < registration_statuses.size(); ++i) {
    const RegistrationStatus& registration_status =
        registration_statuses.Get(i);
    const ObjectIdP& object_id_proto =
        registration_status.registration().object_id();

    // The object is no longer pending, since we have received a server status
    // for it, so remove it from the pendingOperations map. (It may or may not
    // have existed in the map, since we can receive spontaneous status messages
    // from the server.)
    pending_operations_.erase(object_id_proto);

    // We start off with the local-processing set as success, then potentially
    // fail.
    bool is_success = true;

    // if the server operation succeeded, then local processing fails on
    // "incompatibility" as defined above.
    if (registration_status.status().code() == StatusP_Code_SUCCESS) {
      bool app_wants_registration =
          desired_registrations_->Contains(object_id_proto);
      bool is_op_registration =
          (registration_status.registration().op_type() ==
           RegistrationP_OpType_REGISTER);
      bool discrepancy_exists = is_op_registration ^ app_wants_registration;
      if (discrepancy_exists) {
        // Remove the registration and set isSuccess to false, which will cause
        // the caller to issue registration-failure to the application.
        desired_registrations_->Remove(object_id_proto);
        statistics_->RecordError(
            Statistics::ClientErrorType_REGISTRATION_DISCREPANCY);
        TLOG(logger_, INFO,
             "Ticl discrepancy detected: registered = %d, requested = %d. "
             "Removing %s from requested",
             is_op_registration, app_wants_registration,
             ProtoHelpers::ToString(object_id_proto).c_str());
        is_success = false;
      }
    } else {
      // If the server operation failed, then local processing also fails.
      desired_registrations_->Remove(object_id_proto);
      TLOG(logger_, FINE, "Removing %s from committed",
           ProtoHelpers::ToString(object_id_proto).c_str());
      is_success = false;
    }
    success_status->push_back(is_success);
  }
}

void RegistrationManager::GetClientSummary(RegistrationSummary* summary) {
  summary->set_num_registrations(desired_registrations_->size());
  summary->set_registration_digest(desired_registrations_->GetDigest());
}

string RegistrationManager::ToString() {
  return StringPrintf(
      "Last known digest: %s, Requested regs: %s",
      ProtoHelpers::ToString(last_known_server_summary_).c_str(),
      desired_registrations_->ToString().c_str());
}

const char* RegistrationManager::kEmptyPrefix = "";

}  // namespace invalidation
