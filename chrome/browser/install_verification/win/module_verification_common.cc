// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/install_verification/win/module_verification_common.h"

#include "base/files/file_path.h"
#include "base/md5.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/win_util.h"
#include "chrome/browser/install_verification/win/module_info.h"
#include "chrome/browser/install_verification/win/module_list.h"

std::string CalculateModuleNameDigest(const base::string16& module_name) {
  return base::MD5String(base::ToLowerASCII(base::UTF16ToUTF8(
      base::FilePath(module_name).BaseName().value())));
}

bool GetLoadedModules(std::set<ModuleInfo>* loaded_modules) {
  std::vector<HMODULE> snapshot;
  if (!base::win::GetLoadedModulesSnapshot(::GetCurrentProcess(), &snapshot))
    return false;

  ModuleList::FromLoadedModuleSnapshot(snapshot)->GetModuleInfoSet(
      loaded_modules);
  return true;
}

void ReportModuleMatches(const std::vector<std::string>& module_name_digests,
                         const ModuleIDs& module_ids,
                         ModuleVerificationDelegate* delegate) {
  for (size_t i = 0; i < module_name_digests.size(); ++i) {
    ModuleIDs::const_iterator entry = module_ids.find(module_name_digests[i]);
    if (entry != module_ids.end())
      delegate(entry->second);
  }
}
