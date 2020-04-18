// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_PUBLIC_CPP_PREF_REGISTRY_SERIALIZER_H_
#define SERVICES_PREFERENCES_PUBLIC_CPP_PREF_REGISTRY_SERIALIZER_H_

#include "services/preferences/public/mojom/preferences.mojom.h"

class PrefRegistry;

namespace prefs {

mojom::PrefRegistryPtr SerializePrefRegistry(const PrefRegistry& pref_registry);

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_PUBLIC_CPP_PREF_REGISTRY_SERIALIZER_H_
