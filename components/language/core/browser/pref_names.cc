// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/browser/pref_names.h"

namespace language {
namespace prefs {

// The JSON representation of the user's language profile. Used as an input to
// the user language model (i.e. for determining which languages a user
// understands).
const char kUserLanguageProfile[] = "language_profile";

}  // namespace prefs
}  // namespace language
