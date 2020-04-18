// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/test/resource_updater.h"

#include <windows.h>
#include <stdint.h>

#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/logging.h"

namespace upgrade_test {

ResourceUpdater::ResourceUpdater() : handle_(NULL) {
}

ResourceUpdater::~ResourceUpdater() {
  if (handle_ != NULL) {
    // An update wasn't committed, so discard it.
    BOOL result = EndUpdateResource(handle_, TRUE);
    DPCHECK(result != FALSE) << "EndUpdateResource failed";
  }
}

bool ResourceUpdater::Initialize(const base::FilePath& pe_image_path) {
  DCHECK(handle_ == NULL);
  handle_ = BeginUpdateResource(pe_image_path.value().c_str(), FALSE);
  if (handle_ == NULL) {
    PLOG(DFATAL)
      << "BeginUpdateResource failed on \"" << pe_image_path.value() << "\"";
    return false;
  }
  return true;
}

bool ResourceUpdater::Update(const std::wstring& name,
                             const std::wstring& type,
                             WORD language_id,
                             const base::FilePath& input_file) {
  DCHECK(handle_ != NULL);
  base::MemoryMappedFile input;

  if (input.Initialize(input_file)) {
    if (UpdateResource(handle_, type.c_str(), name.c_str(), language_id,
                       const_cast<uint8_t*>(input.data()),
                       static_cast<DWORD>(input.length())) != FALSE) {
      return true;
    }
    PLOG(ERROR) << "UpdateResource failed for resource \"" << name
                << "\" of size " << input.length() << " bytes";
  } else {
    PLOG(DFATAL) << "Failed mapping \"" << input_file.value() << "\"";
  }
  return false;
}

bool ResourceUpdater::Commit() {
  DCHECK(handle_ != NULL);
  bool result = true;
  if (EndUpdateResource(handle_, FALSE) == FALSE) {
    PLOG(DFATAL) << "EndUpdateResource failed";
    result = false;
  }
  handle_ = NULL;
  return result;
}

}  // namespace upgrade_test
