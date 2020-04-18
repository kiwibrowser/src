// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILEAPI_SANDBOX_ORIGIN_DATABASE_INTERFACE_H_
#define STORAGE_BROWSER_FILEAPI_SANDBOX_ORIGIN_DATABASE_INTERFACE_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "storage/browser/storage_browser_export.h"

namespace storage {

class STORAGE_EXPORT SandboxOriginDatabaseInterface {
 public:
  struct STORAGE_EXPORT OriginRecord {
    std::string origin;
    base::FilePath path;

    OriginRecord();
    OriginRecord(const std::string& origin, const base::FilePath& path);
    ~OriginRecord();
  };

  virtual ~SandboxOriginDatabaseInterface() {}

  // Returns true if the origin's path is included in this database.
  virtual bool HasOriginPath(const std::string& origin) = 0;

  // This will produce a unique path and add it to its database, if it's not
  // already present.
  virtual bool GetPathForOrigin(const std::string& origin,
                                base::FilePath* directory) = 0;

  // Removes the origin's path from the database.
  // Returns success if the origin has been successfully removed, or
  // the origin is not found.
  // (This doesn't remove the actual path).
  virtual bool RemovePathForOrigin(const std::string& origin) = 0;

  // Lists all origins in this database.
  virtual bool ListAllOrigins(std::vector<OriginRecord>* origins) = 0;

  // This will release all database resources in use; call it to save memory.
  virtual void DropDatabase() = 0;

 protected:
  SandboxOriginDatabaseInterface() {}
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILEAPI_SANDBOX_ORIGIN_DATABASE_INTERFACE_H_
