// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CATALOG_PUBLIC_CPP_RESOURCE_LOADER_H_
#define SERVICES_CATALOG_PUBLIC_CPP_RESOURCE_LOADER_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "components/services/filesystem/public/interfaces/directory.mojom.h"

namespace base {
class File;
}

namespace catalog {

// ResourceLoader asks the catalog (synchronously) to open the provided paths
// and return the file handles. Use TakeFile() to retrieve a base::File to use
// in client code.
class ResourceLoader {
 public:
  ResourceLoader();
  ~ResourceLoader();

  // (Synchronously) opens all of the files in |paths| for reading. Use
  // TakeFile() subsequently to obtain base::Files to use. Returns true if the
  // sync operation completed, false if it did not.
  bool OpenFiles(filesystem::mojom::DirectoryPtr directory,
                 const std::set<std::string>& paths);

  // Releases and returns the file wrapping the handle.
  base::File TakeFile(const std::string& path);

 private:
  using ResourceMap = std::map<std::string, std::unique_ptr<base::File>>;

  ResourceMap resource_map_;

  DISALLOW_COPY_AND_ASSIGN(ResourceLoader);
};

}  // namespace

#endif  // SERVICES_CATALOG_PUBLIC_CPP_RESOURCE_LOADER_H_
