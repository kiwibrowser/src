// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_SCOPED_PREF_CONNECTION_BUILDER_H_
#define SERVICES_PREFERENCES_SCOPED_PREF_CONNECTION_BUILDER_H_

#include <set>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "services/preferences/public/mojom/preferences.mojom.h"

namespace prefs {
class PersistentPrefStoreImpl;
class PrefStoreImpl;

// A builder for connections to pref stores. When all references are released,
// the connection is created.
class ScopedPrefConnectionBuilder
    : public base::RefCounted<ScopedPrefConnectionBuilder> {
 public:
  ScopedPrefConnectionBuilder(
      std::vector<std::string> observed_prefs,
      mojom::PrefStoreConnector::ConnectCallback callback);

  void ProvidePrefStoreConnections(
      const base::flat_map<PrefValueStore::PrefStoreType,
                           std::unique_ptr<PrefStoreImpl>>& pref_stores);

  void ProvidePrefStoreConnection(PrefValueStore::PrefStoreType type,
                                  PrefStoreImpl* ptr);

  void ProvidePersistentPrefStore(
      PersistentPrefStoreImpl* persistent_pref_store);

  void ProvideIncognitoPersistentPrefStoreUnderlay(
      PersistentPrefStoreImpl* persistent_pref_store,
      const std::vector<const char*>& overlay_pref_names);

  void ProvideDefaults(std::vector<mojom::PrefRegistrationPtr> defaults);

 private:
  friend class base::RefCounted<ScopedPrefConnectionBuilder>;
  ~ScopedPrefConnectionBuilder();

  mojom::PrefStoreConnector::ConnectCallback callback_;
  std::vector<std::string> observed_prefs_;

  base::flat_map<PrefValueStore::PrefStoreType, mojom::PrefStoreConnectionPtr>
      connections_;

  std::vector<mojom::PrefRegistrationPtr> defaults_;

  mojom::PersistentPrefStoreConnectionPtr persistent_pref_store_connection_;
  mojom::IncognitoPersistentPrefStoreConnectionPtr incognito_connection_;

  DISALLOW_COPY_AND_ASSIGN(ScopedPrefConnectionBuilder);
};

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_SCOPED_PREF_CONNECTION_BUILDER_H_
