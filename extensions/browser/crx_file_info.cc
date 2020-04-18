// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/crx_file_info.h"

#include "base/logging.h"

namespace extensions {

CRXFileInfo::CRXFileInfo() : path() {
}

CRXFileInfo::CRXFileInfo(const std::string& i,
                         const base::FilePath& p,
                         const std::string& h)
    : extension_id(i), path(p), expected_hash(h) {
  DCHECK(!path.empty());
}

CRXFileInfo::CRXFileInfo(const std::string& i, const base::FilePath& p)
    : extension_id(i), path(p), expected_hash() {
  DCHECK(!path.empty());
}

CRXFileInfo::CRXFileInfo(const base::FilePath& p)
    : extension_id(), path(p), expected_hash() {
  DCHECK(!path.empty());
}

bool CRXFileInfo::operator==(const CRXFileInfo& that) const {
  return extension_id == that.extension_id && path == that.path &&
         expected_hash == that.expected_hash;
}

}  // namespace extensions
