// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_INVALIDATOR_STORAGE_H_
#define COMPONENTS_INVALIDATION_IMPL_INVALIDATOR_STORAGE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/invalidation/impl/invalidation_state_tracker.h"

class PrefRegistrySimple;
class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace invalidation {

// Wraps PrefService in an InvalidationStateTracker to allow SyncNotifiers
// to use PrefService as persistence for invalidation state. It is not thread
// safe, and lives on the UI thread.
class InvalidatorStorage : public syncer::InvalidationStateTracker {
 public:
  // |pref_service| may not be NULL and must outlive |this|.
  explicit InvalidatorStorage(PrefService* pref_service);
  ~InvalidatorStorage() override;

  // Register prefs to be used by per-Profile instances of this class which
  // store invalidation state in Profile prefs.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Register prefs to be used by a device-global instance of this class which
  // stores invalidation state in local state. This is used on Chrome OS only.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // InvalidationStateTracker implementation.
  void ClearAndSetNewClientId(const std::string& client_id) override;
  std::string GetInvalidatorClientId() const override;
  void SetBootstrapData(const std::string& data) override;
  std::string GetBootstrapData() const override;
  void SetSavedInvalidations(
      const syncer::UnackedInvalidationsMap& map) override;
  syncer::UnackedInvalidationsMap GetSavedInvalidations() const override;
  void Clear() override;

 private:
  base::ThreadChecker thread_checker_;

  PrefService* const pref_service_;

  DISALLOW_COPY_AND_ASSIGN(InvalidatorStorage);
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_IMPL_INVALIDATOR_STORAGE_H_
