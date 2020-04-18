// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_PUBLIC_CPP_IN_PROCESS_SERVICE_FACTORY_H_
#define SERVICES_PREFERENCES_PUBLIC_CPP_IN_PROCESS_SERVICE_FACTORY_H_

#include <memory>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_value_store.h"
#include "services/service_manager/public/cpp/service.h"

class PersistentPrefStore;
class PrefRegistry;
class PrefStore;

namespace prefs {

class InProcessPrefServiceFactory : public KeyedService {
 public:
  InProcessPrefServiceFactory();
  ~InProcessPrefServiceFactory() override;

  std::unique_ptr<PrefValueStore::Delegate> CreateDelegate();

  base::Callback<std::unique_ptr<service_manager::Service>()>
  CreatePrefServiceFactory();

  std::unique_ptr<service_manager::Service> CreatePrefService();

 private:
  class RegisteringDelegate;

  scoped_refptr<PrefStore> managed_prefs_;
  scoped_refptr<PrefStore> supervised_user_prefs_;
  scoped_refptr<PrefStore> extension_prefs_;
  scoped_refptr<PrefStore> command_line_prefs_;
  scoped_refptr<PersistentPrefStore> user_prefs_;
  scoped_refptr<PersistentPrefStore> incognito_user_prefs_underlay_;
  scoped_refptr<PrefStore> recommended_prefs_;
  scoped_refptr<PrefRegistry> pref_registry_;
  std::vector<const char*> overlay_pref_names_;

  base::OnceClosure quit_closure_;

  base::WeakPtrFactory<InProcessPrefServiceFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InProcessPrefServiceFactory);
};

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_PUBLIC_CPP_IN_PROCESS_SERVICE_FACTORY_H_
