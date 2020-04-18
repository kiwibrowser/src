// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREFS_OVERLAY_USER_PREF_STORE_H_
#define COMPONENTS_PREFS_OVERLAY_USER_PREF_STORE_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "components/prefs/persistent_pref_store.h"
#include "components/prefs/pref_value_map.h"
#include "components/prefs/prefs_export.h"

// PersistentPrefStore that directs all write operations into an in-memory
// PrefValueMap. Read operations are first answered by the PrefValueMap.
// If the PrefValueMap does not contain a value for the requested key,
// the look-up is passed on to an underlying PersistentPrefStore |underlay_|.
class COMPONENTS_PREFS_EXPORT OverlayUserPrefStore
    : public PersistentPrefStore {
 public:
  explicit OverlayUserPrefStore(PersistentPrefStore* underlay);
  // The |overlay| must already be initialized.
  OverlayUserPrefStore(PersistentPrefStore* overlay,
                       PersistentPrefStore* underlay);

  // Returns true if a value has been set for the |key| in this
  // OverlayUserPrefStore, i.e. if it potentially overrides a value
  // from the |underlay_|.
  virtual bool IsSetInOverlay(const std::string& key) const;

  // Methods of PrefStore.
  void AddObserver(PrefStore::Observer* observer) override;
  void RemoveObserver(PrefStore::Observer* observer) override;
  bool HasObservers() const override;
  bool IsInitializationComplete() const override;
  bool GetValue(const std::string& key,
                const base::Value** result) const override;
  std::unique_ptr<base::DictionaryValue> GetValues() const override;

  // Methods of PersistentPrefStore.
  bool GetMutableValue(const std::string& key, base::Value** result) override;
  void SetValue(const std::string& key,
                std::unique_ptr<base::Value> value,
                uint32_t flags) override;
  void SetValueSilently(const std::string& key,
                        std::unique_ptr<base::Value> value,
                        uint32_t flags) override;
  void RemoveValue(const std::string& key, uint32_t flags) override;
  bool ReadOnly() const override;
  PrefReadError GetReadError() const override;
  PrefReadError ReadPrefs() override;
  void ReadPrefsAsync(ReadErrorDelegate* delegate) override;
  void CommitPendingWrite(base::OnceClosure done_callback) override;
  void SchedulePendingLossyWrites() override;
  void ReportValueChanged(const std::string& key, uint32_t flags) override;

  void RegisterOverlayPref(const std::string& key);

  void ClearMutableValues() override;
  void OnStoreDeletionFromDisk() override;

 protected:
  ~OverlayUserPrefStore() override;

 private:
  typedef std::set<std::string> NamesSet;
  class ObserverAdapter;

  void OnPrefValueChanged(bool overlay, const std::string& key);
  void OnInitializationCompleted(bool overlay, bool succeeded);

  // Returns true if |key| corresponds to a preference that shall be stored in
  // an in-memory PrefStore that is not persisted to disk.
  bool ShallBeStoredInOverlay(const std::string& key) const;

  base::ObserverList<PrefStore::Observer, true> observers_;
  std::unique_ptr<ObserverAdapter> overlay_observer_;
  std::unique_ptr<ObserverAdapter> underlay_observer_;
  scoped_refptr<PersistentPrefStore> overlay_;
  scoped_refptr<PersistentPrefStore> underlay_;
  NamesSet overlay_names_set_;
  NamesSet written_overlay_names_;

  DISALLOW_COPY_AND_ASSIGN(OverlayUserPrefStore);
};

#endif  // COMPONENTS_PREFS_OVERLAY_USER_PREF_STORE_H_
