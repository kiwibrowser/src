// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/public/cpp/pref_service_factory.h"

#include <memory>
#include <utility>

#include "base/callback_helpers.h"
#include "components/prefs/overlay_user_pref_store.h"
#include "components/prefs/persistent_pref_store.h"
#include "components/prefs/pref_notifier_impl.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_value_store.h"
#include "services/preferences/public/cpp/persistent_pref_store_client.h"
#include "services/preferences/public/cpp/pref_registry_serializer.h"
#include "services/preferences/public/cpp/pref_store_client.h"
#include "services/preferences/public/mojom/preferences.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace prefs {
namespace {

// Used to implement a "fire and forget" pattern where we call an interface
// method, with an attached error handler, but don't care to hold on to the
// InterfacePtr after.
template <typename Interface>
class RefCountedInterfacePtr
    : public base::RefCounted<RefCountedInterfacePtr<Interface>> {
 public:
  mojo::InterfacePtr<Interface>& get() { return ptr_; }
  void reset() { ptr_.reset(); }

 private:
  friend class base::RefCounted<RefCountedInterfacePtr<Interface>>;
  ~RefCountedInterfacePtr() = default;

  mojo::InterfacePtr<Interface> ptr_;
};

scoped_refptr<PrefStore> CreatePrefStoreClient(
    PrefValueStore::PrefStoreType store_type,
    base::flat_map<PrefValueStore::PrefStoreType,
                   mojom::PrefStoreConnectionPtr>* connections) {
  auto pref_store_it = connections->find(store_type);
  if (pref_store_it != connections->end()) {
    return base::MakeRefCounted<PrefStoreClient>(
        std::move(pref_store_it->second));
  }
  return nullptr;
}

void OnPrefServiceInit(std::unique_ptr<PrefService> pref_service,
                       ConnectCallback callback,
                       bool success) {
  if (success) {
    callback.Run(std::move(pref_service));
  } else {
    callback.Run(nullptr);
  }
}

void RegisterRemoteDefaults(PrefRegistry* pref_registry,
                            std::vector<mojom::PrefRegistrationPtr> defaults) {
  for (auto& registration : defaults) {
    pref_registry->SetDefaultForeignPrefValue(
        registration->key,
        base::Value::ToUniquePtrValue(std::move(registration->default_value)),
        registration->flags);
  }
}

void OnConnect(
    scoped_refptr<RefCountedInterfacePtr<mojom::PrefStoreConnector>>
        connector_ptr,
    scoped_refptr<PrefRegistry> pref_registry,
    ConnectCallback callback,
    mojom::PersistentPrefStoreConnectionPtr persistent_pref_store_connection,
    mojom::IncognitoPersistentPrefStoreConnectionPtr incognito_connection,
    std::vector<mojom::PrefRegistrationPtr> defaults,
    base::flat_map<PrefValueStore::PrefStoreType, mojom::PrefStoreConnectionPtr>
        connections) {
  scoped_refptr<PrefStore> managed_prefs =
      CreatePrefStoreClient(PrefValueStore::MANAGED_STORE, &connections);
  scoped_refptr<PrefStore> supervised_user_prefs = CreatePrefStoreClient(
      PrefValueStore::SUPERVISED_USER_STORE, &connections);
  scoped_refptr<PrefStore> extension_prefs =
      CreatePrefStoreClient(PrefValueStore::EXTENSION_STORE, &connections);
  scoped_refptr<PrefStore> command_line_prefs =
      CreatePrefStoreClient(PrefValueStore::COMMAND_LINE_STORE, &connections);
  scoped_refptr<PrefStore> recommended_prefs =
      CreatePrefStoreClient(PrefValueStore::RECOMMENDED_STORE, &connections);
  RegisterRemoteDefaults(pref_registry.get(), std::move(defaults));
  scoped_refptr<PersistentPrefStore> persistent_pref_store(
      new PersistentPrefStoreClient(
          std::move(persistent_pref_store_connection)));
  if (incognito_connection) {
    // If in incognito mode, |persistent_pref_store| above will be a connection
    // to an in-memory pref store and |incognito_connection| will refer to the
    // underlying profile's user pref store.
    auto overlay_pref_store = base::MakeRefCounted<OverlayUserPrefStore>(
        persistent_pref_store.get(),
        new PersistentPrefStoreClient(
            std::move(incognito_connection->pref_store_connection)));
    for (const auto& overlay_pref_name :
         incognito_connection->overlay_pref_names) {
      overlay_pref_store->RegisterOverlayPref(overlay_pref_name);
    }
    persistent_pref_store = overlay_pref_store;
  }
  auto pref_notifier = std::make_unique<PrefNotifierImpl>();
  auto pref_value_store = std::make_unique<PrefValueStore>(
      managed_prefs.get(), supervised_user_prefs.get(), extension_prefs.get(),
      command_line_prefs.get(), persistent_pref_store.get(),
      recommended_prefs.get(), pref_registry->defaults().get(),
      pref_notifier.get());
  auto pref_service = std::make_unique<PrefService>(
      std::move(pref_notifier), std::move(pref_value_store),
      persistent_pref_store.get(), pref_registry.get(), base::DoNothing(),
      true);
  switch (pref_service->GetAllPrefStoresInitializationStatus()) {
    case PrefService::INITIALIZATION_STATUS_WAITING:
      pref_service->AddPrefInitObserver(
          base::BindOnce(&OnPrefServiceInit, base::Passed(&pref_service),
                         base::Passed(&callback)));
      break;
    case PrefService::INITIALIZATION_STATUS_SUCCESS:
    case PrefService::INITIALIZATION_STATUS_CREATED_NEW_PREF_STORE:
      callback.Run(std::move(pref_service));
      break;
    case PrefService::INITIALIZATION_STATUS_ERROR:
      callback.Run(nullptr);
      break;
  }
  connector_ptr->reset();
}

void OnConnectError(
    scoped_refptr<RefCountedInterfacePtr<mojom::PrefStoreConnector>>
        connector_ptr,
    ConnectCallback callback) {
  callback.Run(nullptr);
  connector_ptr->reset();
}

}  // namespace

void ConnectToPrefService(mojom::PrefStoreConnectorPtr connector,
                          scoped_refptr<PrefRegistry> pref_registry,
                          ConnectCallback callback) {
  auto connector_ptr =
      base::MakeRefCounted<RefCountedInterfacePtr<mojom::PrefStoreConnector>>();
  connector_ptr->get() = std::move(connector);
  connector_ptr->get().set_connection_error_handler(base::Bind(
      &OnConnectError, connector_ptr, base::Passed(ConnectCallback{callback})));
  auto serialized_pref_registry = SerializePrefRegistry(*pref_registry);
  connector_ptr->get()->Connect(
      std::move(serialized_pref_registry),
      base::BindOnce(&OnConnect, connector_ptr, std::move(pref_registry),
                     std::move(callback)));
}

void ConnectToPrefService(service_manager::Connector* connector,
                          scoped_refptr<PrefRegistry> pref_registry,
                          ConnectCallback callback,
                          base::StringPiece service_name) {
  mojom::PrefStoreConnectorPtr pref_connector;
  connector->BindInterface(service_name.as_string(), &pref_connector);
  ConnectToPrefService(std::move(pref_connector), std::move(pref_registry),
                       std::move(callback));
}

}  // namespace prefs
