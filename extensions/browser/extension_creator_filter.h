// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_CREATOR_FILTER_H_
#define EXTENSIONS_BROWSER_EXTENSION_CREATOR_FILTER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace base {
class FilePath;
}

namespace extensions {

// Determines which files should be included in a packaged extension.
// Designed specifically to operate with the callback in chrome/common/zip.
class ExtensionCreatorFilter : public base::RefCounted<ExtensionCreatorFilter> {
 public:
  ExtensionCreatorFilter() {}

  // Returns true if the given base::FilePath should be included in a
  // packed extension.
  bool ShouldPackageFile(const base::FilePath& file_path);

 private:
  friend class base::RefCounted<ExtensionCreatorFilter>;
  ~ExtensionCreatorFilter() {}
  DISALLOW_COPY_AND_ASSIGN(ExtensionCreatorFilter);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_CREATOR_FILTER_H_
