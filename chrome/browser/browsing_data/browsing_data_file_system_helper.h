// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_FILE_SYSTEM_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_FILE_SYSTEM_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "storage/common/fileapi/file_system_types.h"
#include "url/gurl.h"

namespace storage {
class FileSystemContext;
}

class Profile;

// Defines an interface for classes that deal with aggregating and deleting
// browsing data stored in an origin's file systems.
// BrowsingDataFileSystemHelper instances for a specific profile should be
// created via the static Create method. Each instance will lazily fetch file
// system data when a client calls StartFetching from the UI thread, and will
// notify the client via a supplied callback when the data is available.
//
// The client's callback is passed a list of FileSystemInfo objects containing
// usage information for each origin's temporary and persistent file systems.
//
// Clients may remove an origin's file systems at any time (even before fetching
// data) by calling DeleteFileSystemOrigin() on the UI thread. Calling
// DeleteFileSystemOrigin() for an origin that doesn't have any is safe; it's
// just an expensive NOOP.
class BrowsingDataFileSystemHelper
    : public base::RefCountedThreadSafe<BrowsingDataFileSystemHelper> {
 public:
  // Detailed information about a file system, including it's origin GURL,
  // the amount of data (in bytes) for each sandboxed filesystem type.
  struct FileSystemInfo {
    explicit FileSystemInfo(const GURL& origin);
    FileSystemInfo(const FileSystemInfo& other);
    ~FileSystemInfo();

    // The origin for which the information is relevant.
    GURL origin;
    // FileSystemType to usage (in bytes) map.
    std::map<storage::FileSystemType, int64_t> usage_map;
  };

  using FetchCallback = base::Callback<void(const std::list<FileSystemInfo>&)>;

  // Creates a BrowsingDataFileSystemHelper instance for the file systems
  // stored in |profile|'s user data directory. The BrowsingDataFileSystemHelper
  // object will hold a reference to the Profile that's passed in, but is not
  // responsible for destroying it.
  //
  // The BrowsingDataFileSystemHelper will not change the profile itself, but
  // can modify data it contains (by removing file systems).
  static BrowsingDataFileSystemHelper* Create(
      storage::FileSystemContext* file_system_context);

  // Starts the process of fetching file system data, which will call |callback|
  // upon completion, passing it a constant list of FileSystemInfo objects.
  // StartFetching must be called only in the UI thread; the provided Callback1
  // will likewise be executed asynchronously on the UI thread.
  //
  // BrowsingDataFileSystemHelper takes ownership of the Callback1, and is
  // responsible for deleting it once it's no longer needed.
  virtual void StartFetching(const FetchCallback& callback) = 0;

  // Deletes any temporary or persistent file systems associated with |origin|
  // from the disk. Deletion will occur asynchronously on the FILE thread, but
  // this function must be called only on the UI thread.
  virtual void DeleteFileSystemOrigin(const GURL& origin) = 0;

 protected:
  friend class base::RefCountedThreadSafe<BrowsingDataFileSystemHelper>;

  BrowsingDataFileSystemHelper() {}
  virtual ~BrowsingDataFileSystemHelper() {}
};

// An implementation of the BrowsingDataFileSystemHelper interface that can
// be manually populated with data, rather than fetching data from the file
// systems created in a particular Profile.
class CannedBrowsingDataFileSystemHelper
    : public BrowsingDataFileSystemHelper {
 public:
  // |profile| is unused in this canned implementation, but it's the interface
  // we're writing to, so we'll accept it, but not store it.
  explicit CannedBrowsingDataFileSystemHelper(Profile* profile);

  // Manually adds a filesystem to the set of canned file systems that this
  // helper returns via StartFetching. If an origin contains both a temporary
  // and a persistent filesystem, AddFileSystem must be called twice (once for
  // each file system type).
  void AddFileSystem(const GURL& origin,
                     storage::FileSystemType type,
                     int64_t size);

  // Clear this helper's list of canned filesystems.
  void Reset();

  // True if no filesystems are currently stored.
  bool empty() const;

  // Returns the number of currently stored filesystems.
  size_t GetFileSystemCount() const;

  // Returns the current list of filesystems.
  const std::list<FileSystemInfo>& GetFileSystemInfo() {
    return file_system_info_;
  }

  // BrowsingDataFileSystemHelper implementation.
  void StartFetching(const FetchCallback& callback) override;

  // Note that this doesn't actually have an implementation for this canned
  // class. It hasn't been necessary for anything that uses the canned
  // implementation, as the canned class is only used in tests, or in read-only
  // contexts (like the non-modal cookie dialog).
  void DeleteFileSystemOrigin(const GURL& origin) override {}

 private:
  ~CannedBrowsingDataFileSystemHelper() override;

  // Holds the current list of filesystems returned to the client.
  std::list<FileSystemInfo> file_system_info_;

  DISALLOW_COPY_AND_ASSIGN(CannedBrowsingDataFileSystemHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_FILE_SYSTEM_HELPER_H_
