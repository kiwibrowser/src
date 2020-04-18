// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/library_cdm_test_helper.h"

#include "base/command_line.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "content/public/browser/cdm_registry.h"
#include "content/public/common/cdm_info.h"
#include "media/base/media_switches.h"
#include "media/cdm/cdm_paths.h"

void RegisterClearKeyCdm(base::CommandLine* command_line,
                         bool use_wrong_cdm_path) {
  base::FilePath cdm_path;
  base::PathService::Get(base::DIR_MODULE, &cdm_path);
  std::string cdm_library_name =
      use_wrong_cdm_path ? "invalidcdmname" : media::kClearKeyCdmLibraryName;
  cdm_path = cdm_path
                 .Append(media::GetPlatformSpecificDirectory(
                     media::kClearKeyCdmBaseDirectory))
                 .AppendASCII(base::GetLoadableModuleName(cdm_library_name));

  // Append the switch to register the Clear Key CDM path.
  command_line->AppendSwitchNative(switches::kClearKeyCdmPathForTesting,
                                   cdm_path.value());
}

bool IsLibraryCdmRegistered(const std::string& cdm_guid) {
  std::vector<content::CdmInfo> cdm_info_vector =
      content::CdmRegistry::GetInstance()->GetAllRegisteredCdms();
  for (const auto& cdm_info : cdm_info_vector) {
    if (cdm_info.guid == cdm_guid) {
      DVLOG(2) << "CDM registered for " << cdm_guid << " with path "
               << cdm_info.path.value();
      return true;
    }
  }

  return false;
}
