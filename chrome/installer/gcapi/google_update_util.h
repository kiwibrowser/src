// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_GCAPI_GOOGLE_UPDATE_UTIL_H_
#define CHROME_INSTALLER_GCAPI_GOOGLE_UPDATE_UTIL_H_

#include "base/strings/string16.h"

namespace gcapi_internals {

extern const wchar_t kChromeRegClientsKey[];
extern const wchar_t kChromeRegClientStateKey[];
extern const wchar_t kChromeRegClientStateMediumKey[];

// Reads Chrome's brand from HKCU or HKLM. Returns true if |value| is populated
// with the brand.
bool GetBrand(base::string16* value);

// Reads Chrome's experiment lables into |experiment_lables|.
bool ReadExperimentLabels(bool system_install,
                          base::string16* experiment_labels);

// Sets Chrome's experiment lables to |experiment_lables|.
bool SetExperimentLabels(bool system_install,
                         const base::string16& experiment_labels);

}  // namespace gcapi_internals

#endif  // CHROME_INSTALLER_GCAPI_GOOGLE_UPDATE_UTIL_H_
