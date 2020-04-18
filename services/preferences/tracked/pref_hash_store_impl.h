// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_TRACKED_PREF_HASH_STORE_IMPL_H_
#define SERVICES_PREFERENCES_TRACKED_PREF_HASH_STORE_IMPL_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "services/preferences/tracked/pref_hash_calculator.h"
#include "services/preferences/tracked/pref_hash_store.h"

// Implements PrefHashStoreImpl by storing preference hashes in a
// HashStoreContents.
class PrefHashStoreImpl : public PrefHashStore {
 public:
  enum StoreVersion {
    // No hashes have been stored in this PrefHashStore yet.
    VERSION_UNINITIALIZED = 0,
    // The hashes in this PrefHashStore were stored before the introduction
    // of a version number and should be re-initialized.
    VERSION_PRE_MIGRATION = 1,
    // The hashes in this PrefHashStore were stored using the latest algorithm.
    VERSION_LATEST = 2,
  };

  // Constructs a PrefHashStoreImpl that calculates hashes using
  // |seed| and |legacy_device_id| and stores them in |contents|.
  //
  // The same |seed| and |legacy_device_id| must be used to load and validate
  // previously stored hashes in |contents|.
  PrefHashStoreImpl(const std::string& seed,
                    const std::string& legacy_device_id,
                    bool use_super_mac);

  ~PrefHashStoreImpl() override;

  // Clears the contents of this PrefHashStore. |IsInitialized()| will return
  // false after this call.
  void Reset();

  // PrefHashStore implementation.
  std::unique_ptr<PrefHashStoreTransaction> BeginTransaction(
      HashStoreContents* storage) override;

  std::string ComputeMac(const std::string& path,
                         const base::Value* new_value) override;
  std::unique_ptr<base::DictionaryValue> ComputeSplitMacs(
      const std::string& path,
      const base::DictionaryValue* split_values) override;

 private:
  class PrefHashStoreTransactionImpl;

  const PrefHashCalculator pref_hash_calculator_;
  bool use_super_mac_;

  DISALLOW_COPY_AND_ASSIGN(PrefHashStoreImpl);
};

#endif  // SERVICES_PREFERENCES_TRACKED_PREF_HASH_STORE_IMPL_H_
