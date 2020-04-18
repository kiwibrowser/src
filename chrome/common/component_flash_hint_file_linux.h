// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_COMPONENT_FLASH_HINT_FILE_LINUX_H_
#define CHROME_COMMON_COMPONENT_FLASH_HINT_FILE_LINUX_H_

#include "build/build_config.h"

#if !defined(OS_LINUX)
#error "This file only applies to the Linux component update of Flash."
#endif  // !defined(OS_LINUX)

#include <string>

namespace base {
class FilePath;
}

// The APIs in this namespace wraps the component updated flash hint file, which
// lives inside the PepperFlash folder of  the user-data-dir, so that the Linux
// zygote process can preload the right version of flash.
namespace component_flash_hint_file {

// Records a new flash update into the hint file.
// |unpacked_plugin| is the current location of the plugin.
// |moved_plugin| is the location where the plugin will be loaded from.
bool RecordFlashUpdate(const base::FilePath& unpacked_plugin,
                       const base::FilePath& moved_plugin,
                       const std::string& version);

// Reports whether or not a hints file exists.
bool DoesHintFileExist();

// Return the path of the component updated flash plugin, only if the file has
// the correct hash sum.
// |path| will be populated with the path to the flash plugin.
// |version| will be populated with the version of the flash plugin.
bool VerifyAndReturnFlashLocation(base::FilePath* path, std::string* version);

// Test if the specified plugin file can be mapped executable.
// This is useful to test if the flash plugin is in a directory mounted
// noexec, in which case Chrome will not be able to load and use the plugin.
// |path| is the path of the flash plugin that will mapped executable.
bool TestExecutableMapping(const base::FilePath& path);

}  // namespace component_flash_hint_file

#endif  // CHROME_COMMON_COMPONENT_FLASH_HINT_FILE_LINUX_H_
