// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_PERSISTENT_PREF_STORE_IMPL_H_
#define SERVICES_PREFERENCES_PERSISTENT_PREF_STORE_IMPL_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "services/preferences/public/mojom/preferences.mojom.h"
#include "services/preferences/public/mojom/tracked_preference_validation_delegate.mojom.h"

namespace prefs {

class PersistentPrefStoreImpl : public PrefStore::Observer {
 public:
  using ObservedPrefs = std::set<std::string>;

  // If |initialized()| is false after construction, |on_initialized| will be
  // called when it becomes true.
  PersistentPrefStoreImpl(
      scoped_refptr<PersistentPrefStore> backing_pref_store,
      base::OnceClosure on_initialized);

  ~PersistentPrefStoreImpl() override;

  mojom::PersistentPrefStoreConnectionPtr CreateConnection(
      ObservedPrefs observed_prefs);

  bool initialized() { return !initializing_; }

 private:
  class Connection;

  void SetValues(std::vector<mojom::PrefUpdatePtr> updates);
  bool GetValue(const std::string& key, const base::Value** value) const;

  void CommitPendingWrite(base::OnceClosure done_callback);
  void SchedulePendingLossyWrites();
  void ClearMutableValues();
  void OnStoreDeletionFromDisk();

  // PrefStore::Observer:
  void OnPrefValueChanged(const std::string& key) override;
  void OnInitializationCompleted(bool succeeded) override;

  void OnConnectionError(Connection* connection);

  scoped_refptr<PersistentPrefStore> backing_pref_store_;

  bool initializing_ = false;

  std::unordered_map<Connection*, std::unique_ptr<Connection>> connections_;

  base::OnceClosure on_initialized_;

  // If true then a write is in progress and any update notifications should be
  // ignored, as those updates would originate from ourselves.
  bool write_in_progress_ = false;

  DISALLOW_COPY_AND_ASSIGN(PersistentPrefStoreImpl);
};

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_PERSISTENT_PREF_STORE_IMPL_H_
