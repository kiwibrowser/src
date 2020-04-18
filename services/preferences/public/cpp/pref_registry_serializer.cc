// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/public/cpp/pref_registry_serializer.h"

#include "components/prefs/pref_registry.h"

namespace prefs {

mojom::PrefRegistryPtr SerializePrefRegistry(
    const PrefRegistry& pref_registry) {
  auto registry = mojom::PrefRegistry::New();
  for (auto& pref : pref_registry) {
    auto flags = pref_registry.GetRegistrationFlags(pref.first);
    if (flags & PrefRegistry::PUBLIC) {
      registry->public_registrations.emplace_back(mojom::PrefRegistration::New(
          pref.first, pref.second->Clone(), flags));
    } else {
      registry->private_registrations.push_back(pref.first);
    }
  }
  for (auto& pref : pref_registry.foreign_pref_keys()) {
    registry->foreign_registrations.push_back(pref);
  }
  return registry;
}

}  // namespace prefs
