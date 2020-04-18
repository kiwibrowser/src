// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/cdm_paths.h"

#include <string>

#include "build/build_config.h"

namespace media {

// Name of the ClearKey CDM library.
const char kClearKeyCdmLibraryName[] = "clearkeycdm";

const char kClearKeyCdmBaseDirectory[] = "ClearKeyCdm";
const char kClearKeyCdmDisplayName[] = "Clear Key CDM";
const char kClearKeyCdmGuid[] = "C1A6B4E3-FE48-4D53-9F52-244AEEAD5335";
const char kClearKeyCdmDifferentGuid[] = "747C565D-34EE-4B0D-AC1E-4F9FB17DDB40";

// As the file system was initially used by the CDM running as a pepper plugin,
// this ID is based on the pepper plugin MIME type.
const char kClearKeyCdmFileSystemId[] = "application_x-ppapi-clearkey-cdm";

// Note: This file must be in sync with cdm_paths.gni.
// TODO(xhwang): Improve how we enable platform specific path. See
// http://crbug.com/468584
#if (defined(OS_MACOSX) || defined(OS_WIN)) && \
    (defined(ARCH_CPU_X86) || defined(ARCH_CPU_X86_64))
#define CDM_USE_PLATFORM_SPECIFIC_PATH
#endif

#if defined(CDM_USE_PLATFORM_SPECIFIC_PATH)

// Special path used in chrome components.
const char kPlatformSpecific[] = "_platform_specific";

// Name of the component platform in the manifest.
const char kComponentPlatform[] =
#if defined(OS_MACOSX)
    "mac";
#elif defined(OS_WIN)
    "win";
#elif defined(OS_CHROMEOS)
    "cros";
#elif defined(OS_LINUX)
    "linux";
#else
    "unsupported_platform";
#endif

// Name of the component architecture in the manifest.
const char kComponentArch[] =
#if defined(ARCH_CPU_X86)
    "x86";
#elif defined(ARCH_CPU_X86_64)
    "x64";
#elif defined(ARCH_CPU_ARMEL)
    "arm";
#else
    "unsupported_arch";
#endif

base::FilePath GetPlatformSpecificDirectory(const std::string& cdm_base_path) {
  base::FilePath path;
  const std::string kPlatformArch =
      std::string(kComponentPlatform) + "_" + kComponentArch;
  return path.AppendASCII(cdm_base_path)
      .AppendASCII(kPlatformSpecific)
      .AppendASCII(kPlatformArch);
}

#else  // defined(CDM_USE_PLATFORM_SPECIFIC_PATH)

base::FilePath GetPlatformSpecificDirectory(const std::string& cdm_base_path) {
  return base::FilePath();
}

#endif  // defined(CDM_USE_PLATFORM_SPECIFIC_PATH)

}  // namespace media
