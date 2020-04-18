// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_GCAPI_GCAPI_OMAHA_EXPERIMENT_H_
#define CHROME_INSTALLER_GCAPI_GCAPI_OMAHA_EXPERIMENT_H_

#include "base/strings/string16.h"

namespace gcapi_internals {

extern const wchar_t kReactivationLabel[];
extern const wchar_t kRelaunchLabel[];

// Returns the full experiment label to be used by |label| (which is one of the
// labels declared above) for |brand_code|.
base::string16 GetGCAPIExperimentLabel(const wchar_t* brand_code,
                                       const base::string16& label);

}  // namespace gcapi_internals

// Writes a reactivation brand code experiment label in the Chrome product and
// binaries registry keys for |brand_code|. This experiment label will have a
// expiration date of now plus one year. If |shell_mode| is set to
// GCAPI_INVOKED_UAC_ELEVATION, the value will be written to HKLM, otherwise
// HKCU. A user cannot have both a reactivation label and a relaunch label set
// at the same time (they are mutually exclusive).
bool SetReactivationExperimentLabels(const wchar_t* brand_code, int shell_mode);

// Writes a relaunch brand code experiment label in the Chrome product and
// binaries registry keys for |brand_code|. This experiment label will have a
// expiration date of now plus one year. If |shell_mode| is set to
// GCAPI_INVOKED_UAC_ELEVATION, the value will be written to HKLM, otherwise
// HKCU. A user cannot have both a reactivation label and a relaunch label set
// at the same time (they are mutually exclusive).
bool SetRelaunchExperimentLabels(const wchar_t* brand_code, int shell_mode);

#endif  // CHROME_INSTALLER_GCAPI_GCAPI_OMAHA_EXPERIMENT_H_
