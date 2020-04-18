// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_SYNC_PREFS_H_
#define COMPONENTS_SYNC_BASE_SYNC_PREFS_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/prefs/pref_member.h"
#include "components/sync/base/model_type.h"
#include "components/sync/protocol/sync.pb.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace syncer {

class SyncPrefObserver {
 public:
  // Called whenever the pref that controls whether sync is managed
  // changes.
  virtual void OnSyncManagedPrefChange(bool is_sync_managed) = 0;

 protected:
  virtual ~SyncPrefObserver();
};

// Use this for the unique machine tag used for session sync.
class SessionSyncPrefs {
 public:
  virtual ~SessionSyncPrefs();
  virtual std::string GetSyncSessionsGUID() const = 0;
  virtual void SetSyncSessionsGUID(const std::string& guid) = 0;
};

// SyncPrefs is a helper class that manages getting, setting, and
// persisting global sync preferences.  It is not thread-safe, and
// lives on the UI thread.
//
// TODO(akalin): Some classes still read the prefs directly.  Consider
// passing down a pointer to SyncPrefs to them.  A list of files:
//
//   profile_sync_service_startup_unittest.cc
//   profile_sync_service.cc
//   sync_setup_flow.cc
//   sync_setup_wizard.cc
//   sync_setup_wizard_unittest.cc
//   two_client_preferences_sync_test.cc
class SyncPrefs : public SessionSyncPrefs,
                  public base::SupportsWeakPtr<SyncPrefs> {
 public:
  // |pref_service| may not be null.
  // Does not take ownership of |pref_service|.
  explicit SyncPrefs(PrefService* pref_service);

  // For testing.
  SyncPrefs();

  ~SyncPrefs() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  void AddSyncPrefObserver(SyncPrefObserver* sync_pref_observer);
  void RemoveSyncPrefObserver(SyncPrefObserver* sync_pref_observer);

  // Clears important sync preferences.
  void ClearPreferences();

  // Getters and setters for global sync prefs.

  bool IsFirstSetupComplete() const;
  void SetFirstSetupComplete();

  bool SyncHasAuthError() const;
  void SetSyncAuthError(bool error);

  bool IsSyncRequested() const;
  void SetSyncRequested(bool is_requested);

  base::Time GetLastSyncedTime() const;
  void SetLastSyncedTime(base::Time time);

  base::Time GetLastPollTime() const;
  void SetLastPollTime(base::Time time);

  base::TimeDelta GetShortPollInterval() const;
  void SetShortPollInterval(base::TimeDelta interval);

  base::TimeDelta GetLongPollInterval() const;
  void SetLongPollInterval(base::TimeDelta interval);

  bool HasKeepEverythingSynced() const;
  void SetKeepEverythingSynced(bool keep_everything_synced);

  // The returned set is guaranteed to be a subset of
  // |registered_types|.  Returns |registered_types| directly if
  // HasKeepEverythingSynced() is true.
  ModelTypeSet GetPreferredDataTypes(ModelTypeSet registered_types) const;
  // |preferred_types| should be a subset of |registered_types|.  All
  // types in |preferred_types| are marked preferred, and all types in
  // |registered_types| \ |preferred_types| are marked not preferred.
  // Changes are still made to the prefs even if
  // HasKeepEverythingSynced() is true, but won't be visible until
  // SetKeepEverythingSynced(false) is called.
  void SetPreferredDataTypes(ModelTypeSet registered_types,
                             ModelTypeSet preferred_types);

  // This pref is set outside of sync.
  bool IsManaged() const;

  // Use this encryption bootstrap token if we're using an explicit passphrase.
  std::string GetEncryptionBootstrapToken() const;
  void SetEncryptionBootstrapToken(const std::string& token);

  // Use this keystore bootstrap token if we're not using an explicit
  // passphrase.
  std::string GetKeystoreEncryptionBootstrapToken() const;
  void SetKeystoreEncryptionBootstrapToken(const std::string& token);

  // Use this for the unique machine tag used for session sync.
  std::string GetSyncSessionsGUID() const override;
  void SetSyncSessionsGUID(const std::string& guid) override;

  // Maps |type| to its corresponding preference name.
  static const char* GetPrefNameForDataType(ModelType type);

#if defined(OS_CHROMEOS)
  // Use this spare bootstrap token only when setting up sync for the first
  // time.
  std::string GetSpareBootstrapToken() const;
  void SetSpareBootstrapToken(const std::string& token);
#endif

  // Get/set/clear first sync time of current user. Used to roll back browsing
  // data later when user signs out.
  base::Time GetFirstSyncTime() const;
  void SetFirstSyncTime(base::Time time);
  void ClearFirstSyncTime();

  // Out of band sync passphrase prompt getter/setter.
  bool IsPassphrasePrompted() const;
  void SetPassphrasePrompted(bool value);

  // For testing.
  void SetManagedForTest(bool is_managed);

  // Get/Set number of memory warnings received.
  int GetMemoryPressureWarningCount() const;
  void SetMemoryPressureWarningCount(int value);

  // Check if the previous shutdown was clean.
  bool DidSyncShutdownCleanly() const;

  // Set whether the last shutdown was clean.
  void SetCleanShutdown(bool value);

  // Get/set for the last known sync invalidation versions.
  void GetInvalidationVersions(
      std::map<ModelType, int64_t>* invalidation_versions) const;
  void UpdateInvalidationVersions(
      const std::map<ModelType, int64_t>& invalidation_versions);

  // Will return the contents of the LastRunVersion preference. This may be an
  // empty string if no version info was present, and is only valid at
  // Sync startup time (after which the LastRunVersion preference will have been
  // updated to the current version).
  std::string GetLastRunVersion() const;
  void SetLastRunVersion(const std::string& current_version);

  // Get/set for flag indicating that passphrase encryption transition is in
  // progress.
  void SetPassphraseEncryptionTransitionInProgress(bool value);
  bool GetPassphraseEncryptionTransitionInProgress() const;

  // Get/set for saved Nigori specifics that must be passed to backend
  // initialization after transition.
  void SetNigoriSpecificsForPassphraseTransition(
      const sync_pb::NigoriSpecifics& nigori_specifics);
  void GetNigoriSpecificsForPassphraseTransition(
      sync_pb::NigoriSpecifics* nigori_specifics) const;

  // Gets the local sync backend enabled state and its database location.
  bool IsLocalSyncEnabled() const;
  base::FilePath GetLocalSyncBackendDir() const;

  // Returns a ModelTypeSet based on |types| expanded to include pref groups
  // (see |pref_groups_|), but as a subset of |registered_types|.
  ModelTypeSet ResolvePrefGroups(ModelTypeSet registered_types,
                                 ModelTypeSet types) const;

 private:
  void RegisterPrefGroups();

  static void RegisterDataTypePreferredPref(
      user_prefs::PrefRegistrySyncable* prefs,
      ModelType type,
      bool is_preferred);
  bool GetDataTypePreferred(ModelType type) const;
  void SetDataTypePreferred(ModelType type, bool is_preferred);

  void OnSyncManagedPrefChanged();

  // May be null.
  PrefService* const pref_service_;

  base::ObserverList<SyncPrefObserver> sync_pref_observers_;

  // The preference that controls whether sync is under control by
  // configuration management.
  BooleanPrefMember pref_sync_managed_;

  bool local_sync_enabled_;

  // Groups of prefs that always have the same value as a "master" pref.
  // For example, the APPS group has {APP_NOTIFICATIONS, APP_SETTINGS}
  // (as well as APPS, but that is implied), so
  //   pref_groups_[APPS] =       { APP_NOTIFICATIONS,
  //                                          APP_SETTINGS }
  //   pref_groups_[EXTENSIONS] = { EXTENSION_SETTINGS }
  // etc.
  using PrefGroupsMap = std::map<ModelType, ModelTypeSet>;
  PrefGroupsMap pref_groups_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SyncPrefs);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_SYNC_PREFS_H_
