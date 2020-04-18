// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_PREF_STORE_MANAGER_IMPL_H_
#define SERVICES_PREFERENCES_PREF_STORE_MANAGER_IMPL_H_

#include <memory>
#include <set>
#include <vector>

#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/prefs/pref_value_store.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "services/preferences/public/mojom/preferences.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

class PrefRegistry;

namespace service_manager {
struct BindSourceInfo;
}

namespace prefs {
class SharedPrefRegistry;
class PersistentPrefStoreImpl;
class PrefStoreImpl;
class ScopedPrefConnectionBuilder;

// This class mediates the connection of clients who wants to read preferences
// and the pref stores that store those preferences. Pref stores use the
// |PrefStoreRegistry| interface to register themselves with the manager and
// clients use the |PrefStoreConnector| interface to connect to these stores.
class PrefStoreManagerImpl : public service_manager::Service {
 public:
  PrefStoreManagerImpl(PrefStore* managed_prefs,
                       PrefStore* supervised_user_prefs,
                       PrefStore* extension_prefs,
                       PrefStore* command_line_prefs,
                       PersistentPrefStore* user_prefs,
                       PersistentPrefStore* incognito_user_prefs_underlay,
                       PrefStore* recommended_prefs,
                       PrefRegistry* pref_registry,
                       std::vector<const char*> overlay_pref_names);
  ~PrefStoreManagerImpl() override;

  base::OnceClosure ShutDownClosure();

 private:
  class ConnectorConnection;

  void BindPrefStoreConnectorRequest(
      prefs::mojom::PrefStoreConnectorRequest request,
      const service_manager::BindSourceInfo& source_info);

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  void OnPersistentPrefStoreReady();
  void OnIncognitoPersistentPrefStoreReady();

  void RegisterPrefStore(PrefValueStore::PrefStoreType type,
                         PrefStore* pref_store);

  void ShutDown();

  base::flat_map<PrefValueStore::PrefStoreType, std::unique_ptr<PrefStoreImpl>>
      read_only_pref_stores_;

  mojo::StrongBindingSet<mojom::PrefStoreConnector> connector_bindings_;
  std::unique_ptr<PersistentPrefStoreImpl> persistent_pref_store_;
  std::unique_ptr<PersistentPrefStoreImpl>
      incognito_persistent_pref_store_underlay_;
  std::vector<const char*> overlay_pref_names_;

  const std::unique_ptr<SharedPrefRegistry> shared_pref_registry_;

  std::vector<scoped_refptr<ScopedPrefConnectionBuilder>>
      pending_persistent_connections_;
  std::vector<scoped_refptr<ScopedPrefConnectionBuilder>>
      pending_persistent_incognito_connections_;

  service_manager::BinderRegistryWithArgs<
      const service_manager::BindSourceInfo&>
      registry_;

  base::WeakPtrFactory<PrefStoreManagerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefStoreManagerImpl);
};

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_PREF_STORE_MANAGER_IMPL_H_
