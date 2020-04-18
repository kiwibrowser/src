// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_MODULE_INFO_WIN_H_
#define CHROME_BROWSER_CONFLICTS_MODULE_INFO_WIN_H_

#include <memory>

#include "base/files/file_path.h"
#include "chrome/browser/conflicts/module_info_util_win.h"

// ModuleInfoKey and ModuleInfoData are used in pair by the ModuleDatabase to
// maintain information about a module, usually in a std::map.

// Used by the ModuleDatabase as a unique identifier for a module.
using ModuleId = int;

// This is the constant portion of the module information, and is used to
// uniquely identify one.
struct ModuleInfoKey {
  ModuleInfoKey(const base::FilePath& module_path,
                uint32_t module_size,
                uint32_t module_time_date_stamp,
                uint32_t module_id);

  // Less-than operator allowing this object to be used in std::map.
  bool operator<(const ModuleInfoKey& mi) const;

  // Full path to the module on disk. Part of the key for a ModuleInfo.
  base::FilePath module_path;

  // The module size. Part of the key for a ModuleInfo. This is taken from
  // SizeOfImage from the module's IMAGE_OPTIONAL_HEADER.
  uint32_t module_size;

  // The module time date stamp. Part of the key for a ModuleInfo. Taken from
  // TimeDateStamp from the module's IMAGE_FILE_HEADER.
  uint32_t module_time_date_stamp;

  // The ID of this module. This is a strictly incrementing value, and is used
  // by the ModuleDatabase to tie a module to the list of running processes in
  // which it is found. It is not part of the key for the module, but it is
  // immutable. This is simply the index of the module in the insertion order.
  ModuleId module_id;
};

// Holds more detailed information about a given module. Because all of this
// information is expensive to gather and requires disk access, it should be
// collected via InspectModule() on a task runner that allow blocking.
struct ModuleInspectionResult {
  ModuleInspectionResult();
  ~ModuleInspectionResult();

  // The module path, not including the basename. This is cleaned and normalized
  // so that common paths are converted to their environment variable mappings
  // (ie, %systemroot%). This makes i18n localized paths easily comparable.
  base::string16 location;

  // The basename of the module.
  base::string16 basename;

  // The name of the product the module belongs to.
  base::string16 product_name;

  // The module file description.
  base::string16 description;

  // The module version. This is usually in the form a.b.c.d (where a, b, c and
  // d are integers), but may also have fewer than 4 components.
  base::string16 version;

  // The certificate info for the module.
  CertificateInfo certificate_info;
};

// Contains the inspection result of a module and additional information that is
// useful to the ModuleDatabase.
struct ModuleInfoData {
  // The possible types of module we are dealing with. Used as bit set values.
  enum ModuleType : uint32_t {
    // These modules are or were loaded into one of chrome's process at some
    // point.
    kTypeLoadedModule = 1 << 0,
    // These modules are registered as a shell extension.
    kTypeShellExtension = 1 << 1,
    // These modules are registered as an Input Method Editor.
    kTypeIme = 1 << 2,
  };

  ModuleInfoData();
  ~ModuleInfoData();

  ModuleInfoData(ModuleInfoData&& module_data) noexcept;

  // Set of all process types in which this module has been seen (may not be
  // currently present in a process of that type). This is a conversion of
  // ProcessType enumeration to a bitfield. See "ProcessTypeToBit" and
  // "BitIndexToProcessType" for details.
  uint32_t process_types;

  // Set that describes the type of the module.
  uint32_t module_types;

  // The inspection result obtained via InspectModule().
  std::unique_ptr<ModuleInspectionResult> inspection_result;
};

// Given a module identified by |module_key|, returns a populated
// ModuleInspectionResult that contains detailed information about the module on
// disk. This is a blocking task that requires access to disk.
std::unique_ptr<ModuleInspectionResult> InspectModule(
    const StringMapping& env_variable_mapping,
    const ModuleInfoKey& module_key);

namespace internal {

// Normalizes the information already contained in |inspection_result|. In
// particular:
// - The path is split in 2 parts: The basename and the location.
// - If it uses commas, the version string is modified to use periods.
// - If there is one, the version string suffix is removed.
//
// Exposed for testing.
void NormalizeInspectionResult(ModuleInspectionResult* inspection_result);

}  // namespace internal

#endif  // CHROME_BROWSER_CONFLICTS_MODULE_INFO_WIN_H_
