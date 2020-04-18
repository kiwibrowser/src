// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/install_verification/win/loaded_module_verification.h"

#include <algorithm>
#include <iterator>
#include <string>
#include "chrome/browser/install_verification/win/module_ids.h"
#include "chrome/browser/install_verification/win/module_info.h"

namespace {

std::string ExtractModuleNameDigest(const ModuleInfo& module_info) {
  return CalculateModuleNameDigest(module_info.name);
}

}  // namespace

void VerifyLoadedModules(const std::set<ModuleInfo>& loaded_modules,
                         const ModuleIDs& module_ids,
                         ModuleVerificationDelegate* delegate) {
  std::vector<std::string> module_name_digests;
  std::transform(loaded_modules.begin(),
                 loaded_modules.end(),
                 std::back_inserter(module_name_digests),
                 &ExtractModuleNameDigest);
  ReportModuleMatches(module_name_digests, module_ids, delegate);
}
