// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_FILE_SYSTEM_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_FILE_SYSTEM_HELPER_H_

#include <list>
#include <map>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/browsing_data/browsing_data_file_system_helper.h"

// Mock for BrowsingDataFileSystemHelper.
// Use AddFileSystemSamples() or add directly to response_ list, then call
// Notify().
class MockBrowsingDataFileSystemHelper : public BrowsingDataFileSystemHelper {
 public:
  explicit MockBrowsingDataFileSystemHelper(Profile* profile);

  // BrowsingDataFileSystemHelper implementation.
  void StartFetching(const FetchCallback& callback) override;
  void DeleteFileSystemOrigin(const GURL& origin) override;

  // Adds a specific filesystem.
  void AddFileSystem(const GURL& origin,
                     bool has_persistent,
                     bool has_temporary,
                     bool has_syncable,
                     int64_t size_persistent,
                     int64_t size_temporary,
                     int64_t size_syncable);

  // Adds some FilesystemInfo samples.
  void AddFileSystemSamples();

  // Notifies the callback.
  void Notify();

  // Marks all filesystems as existing.
  void Reset();

  // Returns true if all filesystemss since the last Reset() invocation were
  // deleted.
  bool AllDeleted();

  GURL last_deleted_origin_;

 private:
  ~MockBrowsingDataFileSystemHelper() override;

  FetchCallback callback_;

  // Stores which filesystems exist.
  std::map<const std::string, bool> file_systems_;

  std::list<FileSystemInfo> response_;

  DISALLOW_COPY_AND_ASSIGN(MockBrowsingDataFileSystemHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_FILE_SYSTEM_HELPER_H_
