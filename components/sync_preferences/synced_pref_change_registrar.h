// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_PREFERENCES_SYNCED_PREF_CHANGE_REGISTRAR_H_
#define COMPONENTS_SYNC_PREFERENCES_SYNCED_PREF_CHANGE_REGISTRAR_H_

#include <map>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_preferences/synced_pref_observer.h"

namespace sync_preferences {

// Manages the registration of one or more SyncedPrefObservers on a
// PrefServiceSyncable. This is modeled after base::PrefChangeRegistrar, and
// it should be used whenever it's necessary to determine whether a pref change
// has come from sync or from some other mechanism (managed, UI, external, etc.)
class SyncedPrefChangeRegistrar : public SyncedPrefObserver {
 public:
  // Registered callbacks may optionally take a path argument.
  // The boolean argument indicates whether (true) or not (false)
  // the change was a result of syncing.
  typedef base::Callback<void(bool)> ChangeCallback;
  typedef base::Callback<void(const std::string&, bool)> NamedChangeCallback;

  explicit SyncedPrefChangeRegistrar(PrefServiceSyncable* pref_service);
  virtual ~SyncedPrefChangeRegistrar();

  // Register an observer callback for sync change events on the pref at
  // |path|. Only one callback may be registered per pref.
  void Add(const char* path, const ChangeCallback& callback);
  void Add(const char* path, const NamedChangeCallback& callback);

  // Remove the registered observer for |path|.
  void Remove(const char* path);

  // Remove all registered observers.
  void RemoveAll();

  // Indicates whether or not an observer is already registered for |path|.
  bool IsObserved(const char* path) const;

 private:
  // SyncedPrefObserver implementation
  void OnSyncedPrefChanged(const std::string& path, bool from_sync) override;

  typedef std::map<std::string, NamedChangeCallback> ObserverMap;

  PrefServiceSyncable* pref_service_;
  ObserverMap observers_;

  DISALLOW_COPY_AND_ASSIGN(SyncedPrefChangeRegistrar);
};

}  // namespace sync_preferences

#endif  // COMPONENTS_SYNC_PREFERENCES_SYNCED_PREF_CHANGE_REGISTRAR_H_
