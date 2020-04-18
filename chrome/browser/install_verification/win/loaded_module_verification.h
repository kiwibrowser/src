// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALL_VERIFICATION_WIN_LOADED_MODULE_VERIFICATION_H_
#define CHROME_BROWSER_INSTALL_VERIFICATION_WIN_LOADED_MODULE_VERIFICATION_H_

#include <set>

#include "chrome/browser/install_verification/win/module_ids.h"
#include "chrome/browser/install_verification/win/module_verification_common.h"

struct ModuleInfo;

// Verifies the provided |loaded_modules| info against the expected
// |module_ids|. Reports results to |delegate|.
void VerifyLoadedModules(const std::set<ModuleInfo>& loaded_modules,
                         const ModuleIDs& module_ids,
                         ModuleVerificationDelegate* delegate);

#endif  // CHROME_BROWSER_INSTALL_VERIFICATION_WIN_LOADED_MODULE_VERIFICATION_H_
