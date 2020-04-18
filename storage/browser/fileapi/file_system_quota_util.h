// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_QUOTA_UTIL_H_
#define STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_QUOTA_UTIL_H_

#include <stdint.h>

#include <set>
#include <string>

#include "base/files/file.h"
#include "storage/browser/storage_browser_export.h"
#include "storage/common/fileapi/file_system_types.h"
#include "url/gurl.h"

namespace storage {
class QuotaManagerProxy;
}

namespace storage {

class FileSystemContext;
class QuotaReservation;

// An abstract interface that provides common quota-related utility functions
// for file_system_quota_client.
// All the methods of this class are synchronous and need to be called on
// the thread that the method name implies.
class STORAGE_EXPORT FileSystemQuotaUtil {
 public:
  virtual ~FileSystemQuotaUtil() {}

  // Deletes the data on the origin and reports the amount of deleted data
  // to the quota manager via |proxy|.
  virtual base::File::Error DeleteOriginDataOnFileTaskRunner(
      FileSystemContext* context,
      storage::QuotaManagerProxy* proxy,
      const GURL& origin_url,
      FileSystemType type) = 0;

  virtual void GetOriginsForTypeOnFileTaskRunner(storage::FileSystemType type,
                                                 std::set<GURL>* origins) = 0;

  virtual void GetOriginsForHostOnFileTaskRunner(storage::FileSystemType type,
                                                 const std::string& host,
                                                 std::set<GURL>* origins) = 0;

  // Returns the amount of data used for the origin for usage tracking.
  virtual int64_t GetOriginUsageOnFileTaskRunner(
      storage::FileSystemContext* file_system_context,
      const GURL& origin_url,
      storage::FileSystemType type) = 0;

  // Creates new reservation object for the origin and the type.
  virtual scoped_refptr<QuotaReservation>
      CreateQuotaReservationOnFileTaskRunner(
          const GURL& origin_url,
          FileSystemType type) = 0;
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_QUOTA_UTIL_H_
