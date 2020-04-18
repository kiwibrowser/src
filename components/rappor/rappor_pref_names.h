// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_RAPPOR_PREF_NAMES_H_
#define COMPONENTS_RAPPOR_RAPPOR_PREF_NAMES_H_

namespace rappor {
namespace prefs {

// Alphabetical list of preference names specific to the Rappor
// component. Keep alphabetized, and document each in the .cc file.
extern const char kRapporCohortDeprecated[];
extern const char kRapporCohortSeed[];
extern const char kRapporLastDailySample[];
extern const char kRapporSecret[];

}  // namespace prefs
}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_RAPPOR_PREF_NAMES_H_
