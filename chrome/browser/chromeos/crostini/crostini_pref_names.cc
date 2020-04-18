// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/crostini/crostini_pref_names.h"

#include "components/prefs/pref_registry_simple.h"

namespace crostini {
namespace prefs {

// A boolean preference representing whether a user has opted in to use
// Crostini (Called "Linux Apps" in UI).
const char kCrostiniEnabled[] = "crostini.enabled";

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kCrostiniEnabled, false);
}

}  // namespace prefs
}  // namespace crostini
