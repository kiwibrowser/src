// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALL_VERIFICATION_WIN_MODULE_VERIFICATION_COMMON_H_
#define CHROME_BROWSER_INSTALL_VERIFICATION_WIN_MODULE_VERIFICATION_COMMON_H_

#include <stddef.h>

#include <set>
#include <string>
#include <vector>
#include "base/strings/string16.h"
#include "chrome/browser/install_verification/win/module_ids.h"

struct ModuleInfo;

// Calculates a canonical digest for |module_name|. Ignores case and strips path
// information if present.
std::string CalculateModuleNameDigest(const base::string16& module_name);

// Retrieves a ModuleInfo set representing all currenly loaded modules. Returns
// false in case of failure.
bool GetLoadedModules(std::set<ModuleInfo>* loaded_modules);

// Receives notification of a module verification result.
typedef void (ModuleVerificationDelegate)(size_t module_id);

// For each module in |module_name_digests|, reports the associated ID from
// |module_ids|, if any, to |delegate|.
void ReportModuleMatches(const std::vector<std::string>& module_name_digests,
                         const ModuleIDs& module_ids,
                         ModuleVerificationDelegate* delegate);

#endif  // CHROME_BROWSER_INSTALL_VERIFICATION_WIN_MODULE_VERIFICATION_COMMON_H_
