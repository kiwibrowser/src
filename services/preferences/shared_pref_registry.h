// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_SHARED_PREF_REGISTRY_H_
#define SERVICES_PREFERENCES_SHARED_PREF_REGISTRY_H_

#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "services/preferences/public/mojom/preferences.mojom.h"

class PrefRegistry;

namespace service_manager {
class Identity;
}

namespace prefs {
class ScopedPrefConnectionBuilder;

// A registry of default pref values for public prefs. It records public default
// pref values from connections and provides public default pref values to
// connections that require them.
class SharedPrefRegistry {
 public:
  explicit SharedPrefRegistry(scoped_refptr<PrefRegistry> registry);
  ~SharedPrefRegistry();

  scoped_refptr<ScopedPrefConnectionBuilder> CreateConnectionBuilder(
      mojom::PrefRegistryPtr pref_registry,
      const service_manager::Identity& identity,
      mojom::PrefStoreConnector::ConnectCallback callback);

 private:
  class PendingConnection;

  std::set<std::string> GetPendingForeignPrefs(
      std::vector<std::string> foreign_prefs,
      std::vector<std::string>* observed_prefs) const;

  void ProcessPublicPrefs(
      std::vector<mojom::PrefRegistrationPtr> public_pref_registrations,
      bool enforce_single_owner_check,
      std::vector<std::string>* observed_prefs);

  void ProvideDefaultPrefs(ScopedPrefConnectionBuilder* connection,
                           std::vector<std::string> foreign_prefs);

  scoped_refptr<PrefRegistry> registry_;

  std::set<std::string> public_pref_keys_;

  std::set<service_manager::Identity> connected_services_;

  std::vector<PendingConnection> pending_connections_;

#if DCHECK_IS_ON()
  // The set of all registered pref keys. This enforces that only one service
  // claims ownership over any pref. This is relatively expensive so skip
  // unless dchecks are enabled.
  std::set<std::string> all_registered_pref_keys_;

  // Maps from service identity to the set of pref keys registered by that
  // service. This enforces that each service registers the same keys for each
  // connect. This is relatively expensive so skip unless dchecks are enabled.
  std::map<service_manager::Identity, std::set<std::string>>
      per_service_registered_pref_keys_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SharedPrefRegistry);
};

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_SHARED_PREF_REGISTRY_H_
