// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PREFS_PREF_CONNECTOR_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_PREFS_PREF_CONNECTOR_SERVICE_H_

#include <vector>

#include "ash/public/interfaces/pref_connector.mojom.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_value_store.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/preferences/public/mojom/preferences.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

// A |ash::mojom::PrefConnector| implementation that provides ash with access to
// a |prefs::mojom::PrefStoreConnector| for a requested profile.
//
// TODO(http://crbug.com/705347): Once mash can connect to per-profile services
// via service manager, remove this class and the ash_pref_connector service.
class AshPrefConnector : public ash::mojom::PrefConnector,
                         public service_manager::Service {
 public:
  AshPrefConnector();
  ~AshPrefConnector() override;

 private:
  // ash::mojom::PrefConnector:
  void GetPrefStoreConnectorForSigninScreen(
      prefs::mojom::PrefStoreConnectorRequest request) override;
  void GetPrefStoreConnectorForUser(
      const AccountId& account_id,
      prefs::mojom::PrefStoreConnectorRequest request) override;

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  void BindConnectorRequest(ash::mojom::PrefConnectorRequest request);

  prefs::mojom::PrefStoreConnector& GetPrefStoreConnector();

  service_manager::BinderRegistry registry_;
  mojo::BindingSet<ash::mojom::PrefConnector> connector_bindings_;

  base::WeakPtrFactory<AshPrefConnector> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AshPrefConnector);
};

#endif  // CHROME_BROWSER_CHROMEOS_PREFS_PREF_CONNECTOR_SERVICE_H_
