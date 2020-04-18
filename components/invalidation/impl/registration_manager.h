// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A class that manages the registration of types for server-issued
// notifications.

#ifndef COMPONENTS_INVALIDATION_IMPL_REGISTRATION_MANAGER_H_
#define COMPONENTS_INVALIDATION_IMPL_REGISTRATION_MANAGER_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
// For invalidation::InvalidationListener::RegistrationState.
#include "components/invalidation/public/invalidation_export.h"
#include "components/invalidation/public/invalidation_util.h"
#include "google/cacheinvalidation/include/invalidation-listener.h"
#include "google/cacheinvalidation/include/types.h"

namespace syncer {

using ::invalidation::InvalidationListener;

// Manages the details of registering types for invalidation.
// Implements exponential backoff for repeated registration attempts
// to the invalidation client.
//
// TODO(akalin): Consolidate exponential backoff code.  Other
// implementations include the syncer thread (both versions) and XMPP
// retries.  The most sophisticated one is URLRequestThrottler; making
// that generic should work for everyone.
class INVALIDATION_EXPORT RegistrationManager {
 public:
  // Constants for exponential backoff (used by tests).
  static const int kInitialRegistrationDelaySeconds;
  static const int kRegistrationDelayExponent;
  static const double kRegistrationDelayMaxJitter;
  static const int kMinRegistrationDelaySeconds;
  static const int kMaxRegistrationDelaySeconds;

  // Types used by testing functions.
  struct PendingRegistrationInfo {
    PendingRegistrationInfo();

    // Last time a registration request was actually sent.
    base::Time last_registration_request;
    // Time the registration was attempted.
    base::Time registration_attempt;
    // The calculated delay of the pending registration (which may be
    // negative).
    base::TimeDelta delay;
    // The delay of the timer, which should be max(delay, 0).
    base::TimeDelta actual_delay;
  };
  // Map of object IDs with pending registrations to info about the
  // pending registration.
  typedef std::map<invalidation::ObjectId,
                   PendingRegistrationInfo,
                   ObjectIdLessThan>
      PendingRegistrationMap;

  // Does not take ownership of |invalidation_client_|.
  explicit RegistrationManager(
      invalidation::InvalidationClient* invalidation_client);

  virtual ~RegistrationManager();

  // Registers all object IDs included in the given set (that are not
  // already disabled) and unregisters all other object IDs. The return value is
  // the set of IDs that was unregistered.
  ObjectIdSet UpdateRegisteredIds(const ObjectIdSet& ids);

  // Marks the registration for the |id| lost and re-registers
  // it (unless it's disabled).
  void MarkRegistrationLost(const invalidation::ObjectId& id);

  // Marks registrations lost for all enabled object IDs and re-registers them.
  void MarkAllRegistrationsLost();

  // Marks the registration for the |id| permanently lost and blocks any future
  // registration attempts.
  void DisableId(const invalidation::ObjectId& id);

  // Calculate exponential backoff.  |jitter| must be Uniform[-1.0, 1.0].
  static double CalculateBackoff(double retry_interval,
                                 double initial_retry_interval,
                                 double min_retry_interval,
                                 double max_retry_interval,
                                 double backoff_exponent,
                                 double jitter,
                                 double max_jitter);

  // The functions below should only be used in tests.

  // Gets all currently registered ids.
  ObjectIdSet GetRegisteredIdsForTest() const;

  // Gets all pending registrations and their next min delays.
  PendingRegistrationMap GetPendingRegistrationsForTest() const;

  // Run pending registrations immediately.
  void FirePendingRegistrationsForTest();

 protected:
  // Overrideable for testing purposes.
  virtual double GetJitter();

 private:
  struct RegistrationStatus {
    RegistrationStatus(const invalidation::ObjectId& id,
                       RegistrationManager* manager);
    ~RegistrationStatus();

    // Calls registration_manager->DoRegister(model_type). (needed by
    // |registration_timer|).  Should only be called if |enabled| is
    // true.
    void DoRegister();

    // Sets |enabled| to false and resets other variables.
    void Disable();

    // The object for which this is the status.
    const invalidation::ObjectId id;
    // The parent registration manager.
    RegistrationManager* const registration_manager;

    // Whether this data type should be registered.  Set to false if
    // we get a non-transient registration failure.
    bool enabled;
    // The current registration state.
    InvalidationListener::RegistrationState state;
    // When we last sent a registration request.
    base::Time last_registration_request;
    // When we last tried to register.
    base::Time last_registration_attempt;
    // The calculated delay of any pending registration (which may be
    // negative).
    base::TimeDelta delay;
    // The minimum time to wait until any next registration attempt.
    // Increased after each consecutive failure.
    base::TimeDelta next_delay;
    // The actual timer for registration.
    base::OneShotTimer registration_timer;

    DISALLOW_COPY_AND_ASSIGN(RegistrationStatus);
  };

  // Does nothing if the given id is disabled.  Otherwise, if
  // |is_retry| is not set, registers the given type immediately and
  // resets all backoff parameters.  If |is_retry| is set, registers
  // the given type at some point in the future and increases the
  // delay until the next retry.
  void TryRegisterId(const invalidation::ObjectId& id,
                     bool is_retry);

  // Registers the given id, which must be valid, immediately.
  // Updates |last_registration| in the appropriate
  // RegistrationStatus.  Should only be called by
  // RegistrationStatus::DoRegister().
  void DoRegisterId(const invalidation::ObjectId& id);

  // Unregisters the given object ID.
  void UnregisterId(const invalidation::ObjectId& id);

  // Gets all currently registered ids.
  ObjectIdSet GetRegisteredIds() const;

  // Returns true iff the given object ID is registered.
  bool IsIdRegistered(const invalidation::ObjectId& id) const;

  std::map<invalidation::ObjectId,
           std::unique_ptr<RegistrationStatus>,
           ObjectIdLessThan>
      registration_statuses_;
  // Weak pointer.
  invalidation::InvalidationClient* invalidation_client_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(RegistrationManager);
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_REGISTRATION_MANAGER_H_
