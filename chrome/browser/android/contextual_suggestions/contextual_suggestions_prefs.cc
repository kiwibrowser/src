// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/contextual_suggestions/contextual_suggestions_prefs.h"

#include "components/prefs/pref_registry_simple.h"

namespace contextual_suggestions {

namespace prefs {

const char kContextualSuggestionsEnabled[] = "contextual_suggestions.enabled";

}  // namespace prefs

// static
void ContextualSuggestionsPrefs::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kContextualSuggestionsEnabled, true);
}

}  // namespace contextual_suggestions
