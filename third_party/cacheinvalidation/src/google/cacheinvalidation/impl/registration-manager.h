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

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_REGISTRATION_MANAGER_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_REGISTRATION_MANAGER_H_

#include <map>
#include <set>

#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/deps/digest-function.h"
#include "google/cacheinvalidation/deps/scoped_ptr.h"
#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"
#include "google/cacheinvalidation/impl/digest-store.h"
#include "google/cacheinvalidation/impl/proto-helpers.h"
#include "google/cacheinvalidation/impl/statistics.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::map;
using INVALIDATION_STL_NAMESPACE::set;

class RegistrationManager {
 public:
  RegistrationManager(Logger* logger, Statistics* statistics,
                      DigestFunction* digest_function);

  /* Sets the digest store to be digest_store for testing purposes.
   *
   * REQUIRES: This method is called before the Ticl has done any operations on
   * this object.
   */
  void SetDigestStoreForTest(DigestStore<ObjectIdP>* digest_store) {
    desired_registrations_.reset(digest_store);
    GetClientSummary(&last_known_server_summary_);
  }

  void GetRegisteredObjectsForTest(vector<ObjectIdP>* registrations) {
    desired_registrations_->GetElements(kEmptyPrefix, 0, registrations);
  }

  /* (Un)registers for object_ids. When the function returns, oids_to_send will
   * have been modified to contain those object ids for which registration
   * messages must be sent to the server.
   */
  void PerformOperations(const vector<ObjectIdP>& object_ids,
                         RegistrationP::OpType reg_op_type,
                         vector<ObjectIdP>* oids_to_send);

  /* Initializes a registration subtree for registrations where the digest of
   * the object id begins with the prefix digest_prefix of prefix_len bits. This
   * method may also return objects whose digest prefix does not match
   * digest_prefix.
   */
  void GetRegistrations(const string& digest_prefix, int prefix_len,
                        RegistrationSubtree* builder);

  /*
   * Handles registration operation statuses from the server. Modifies |result|
   * to contain one boolean per registration status, that indicates whether the
   * registration operation was both successful and agreed with the desired
   * client state (i.e., for each registration status,
   * (status.optype == register) ==
   * desiredRegistrations.contains(status.objectid)).
   * <p>
   * REQUIRES: the caller subsequently make an informRegistrationStatus or
   * informRegistrationFailure upcall on the listener for each registration in
   * {@code registrationStatuses}.
   */
  void HandleRegistrationStatus(
      const RepeatedPtrField<RegistrationStatus>& registration_statuses,
      vector<bool>* result);

  /*
   * Removes all desired registrations and pending operations. Returns all
   * object ids that were affected.
   * <p>
   * REQUIRES: the caller issue a permanent failure upcall to the listener for
   * all returned object ids.
   */
  void RemoveRegisteredObjects(vector<ObjectIdP>* result) {
    // Add the formerly desired- and pending- registrations to result.
    desired_registrations_->RemoveAll(result);
    map<ObjectIdP, RegistrationP::OpType, ProtoCompareLess>::iterator
        pending_iter = pending_operations_.begin();
    for (; pending_iter != pending_operations_.end(); pending_iter++) {
      result->push_back(pending_iter->first);
    }
    pending_operations_.clear();

    // De-dup result.
    set<ObjectIdP, ProtoCompareLess> unique_oids(result->begin(),
                                                 result->end());
    result->assign(unique_oids.begin(), unique_oids.end());
  }

  //
  // Digest-related methods
  //

  /* Modifies client_summary to contain the summary of the desired
   * registrations (by the client). */
  void GetClientSummary(RegistrationSummary* client_summary);

  /* Modifies server_summary to contain the last known summary from the server.
   * If none, modifies server_summary to contain the summary corresponding
   * to 0 registrations. */
  void GetServerSummary(RegistrationSummary* server_summary) {
    server_summary->CopyFrom(last_known_server_summary_);
  }

  /* Informs the manager of a new registration state summary from the server.
   * Modifies upcalls to contain zero or more RegistrationP. For each added
   * RegistrationP, the caller should make an inform-registration-status upcall
   * on the listener.
   */
  void InformServerRegistrationSummary(const RegistrationSummary& reg_summary,
    vector<RegistrationP>* upcalls) {
    last_known_server_summary_.CopyFrom(reg_summary);
    if (IsStateInSyncWithServer()) {
      // If we are now in sync with the server, then the caller should make
      // inform-reg-status upcalls for all operations that we had pending, if
      // any; they are also no longer pending.
      map<ObjectIdP, RegistrationP::OpType, ProtoCompareLess>::iterator
          pending_iter = pending_operations_.begin();
      for (; pending_iter != pending_operations_.end(); pending_iter++) {
        RegistrationP reg_p;
        ProtoHelpers::InitRegistrationP(pending_iter->first,
            pending_iter->second, &reg_p);
        upcalls->push_back(reg_p);
      }
      pending_operations_.clear();
    }
  }

  /* Returns whether the local registration state and server state agree, based
   * on the last received server summary (from InformServerRegistrationSummary).
   */
  bool IsStateInSyncWithServer() {
    RegistrationSummary summary;
    GetClientSummary(&summary);
    return (last_known_server_summary_.num_registrations() ==
            summary.num_registrations()) &&
        (last_known_server_summary_.registration_digest() ==
         summary.registration_digest());
  }

  string ToString();

  // Empty hash prefix.
  static const char* kEmptyPrefix;

 private:
  /* The set of regisrations that the application has requested for. */
  scoped_ptr<DigestStore<ObjectIdP> > desired_registrations_;

  /* Statistics objects to track number of sent messages, etc. */
  Statistics* statistics_;

  /* Latest known server registration state summary. */
  RegistrationSummary last_known_server_summary_;

  /*
   * Map of object ids and operation types for which we have not yet issued any
   * registration-status upcall to the listener. We need this so that we can
   * synthesize success upcalls if registration sync, rather than a server
   * message, communicates to us that we have a successful (un)registration.
   * <p>
   * This is a map from object id to type, rather than a set of RegistrationP,
   * because a set of RegistrationP would assume that we always get a response
   * for every operation we issue, which isn't necessarily true (i.e., the
   * server might send back an unregistration status in response to a
   * registration request).
   */
  map<ObjectIdP, RegistrationP::OpType, ProtoCompareLess>
      pending_operations_;

  Logger* logger_;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_REGISTRATION_MANAGER_H_
