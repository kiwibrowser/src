// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_WRITE_ON_CACHE_FILE_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_WRITE_ON_CACHE_FILE_H_

#include "base/callback_forward.h"
#include "components/drive/file_errors.h"

namespace base {
class FilePath;
}

namespace drive {

class FileSystemInterface;

// Callback type for WriteOnCacheFile.
typedef base::Callback<void (FileError, const base::FilePath& path)>
    WriteOnCacheFileCallback;

// Creates (if needed) a file at |path|, brings it to the local cache,
// and invokes |callback| on blocking thread pool with the cache file path.
// The |callback| can write to the file at the path by normal local file I/O
// operations. After it returns, the written content is synced to the server.
//
// If non-empty |mime_type| is set and the file is created by this function
// call, the mime type for the entry is set to |mime_type|. Otherwise the type
// is automatically determined from |path|.
//
// Must be called from UI thread.
void WriteOnCacheFile(FileSystemInterface* file_system,
                      const base::FilePath& path,
                      const std::string& mime_type,
                      const WriteOnCacheFileCallback& callback);

// Does the same thing as WriteOnCacheFile() and runs |reply| on the UI thread
// after the completion.
void WriteOnCacheFileAndReply(FileSystemInterface* file_system,
                              const base::FilePath& path,
                              const std::string& mime_type,
                              const WriteOnCacheFileCallback& callback,
                              const FileOperationCallback& reply);

}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_WRITE_ON_CACHE_FILE_H_
