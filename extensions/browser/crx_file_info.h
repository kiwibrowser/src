// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_CRX_FILE_INFO_H_
#define EXTENSIONS_BROWSER_CRX_FILE_INFO_H_

#include <string>

#include "base/files/file_path.h"

namespace extensions {

// CRXFileInfo holds general information about a cached CRX file
struct CRXFileInfo {
  CRXFileInfo();
  CRXFileInfo(const std::string& extension_id,
              const base::FilePath& path,
              const std::string& hash);
  CRXFileInfo(const std::string& extension_id, const base::FilePath& path);
  explicit CRXFileInfo(const base::FilePath& path);

  bool operator==(const CRXFileInfo& that) const;

  // The only mandatory field is the file path, whereas extension_id and hash
  // are only being checked if those are non-empty.
  std::string extension_id;
  base::FilePath path;
  std::string expected_hash;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_CRX_FILE_INFO_H_
