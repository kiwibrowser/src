// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/suggestions/suggestions_pref_names.h"

namespace suggestions {
namespace prefs {

// A cache of a suggestions blacklist, represented as a serialized
// SuggestionsBlacklist protobuf.
const char kSuggestionsBlacklist[] = "suggestions.blacklist";

// A cache of suggestions represented as a serialized SuggestionsProfile
// protobuf.
const char kSuggestionsData[] = "suggestions.data";

}  // namespace prefs
}  // namespace suggestions
