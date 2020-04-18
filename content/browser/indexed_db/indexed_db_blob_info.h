// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_BLOB_INFO_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_BLOB_INFO_H_

#include <stdint.h>

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/time/time.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT IndexedDBBlobInfo {
 public:
  // TODO(mek): Use ShareableFileReference::FinalReleaseCallback somehow.
  typedef base::RepeatingCallback<void(const base::FilePath&)> ReleaseCallback;
  IndexedDBBlobInfo();
  // These two are used for Blobs.
  IndexedDBBlobInfo(const std::string& uuid,
                    const base::string16& type,
                    int64_t size);
  IndexedDBBlobInfo(const base::string16& type, int64_t size, int64_t key);
  // These two are used for Files.
  IndexedDBBlobInfo(const std::string& uuid,
                    const base::FilePath& file_path,
                    const base::string16& file_name,
                    const base::string16& type);
  IndexedDBBlobInfo(int64_t key,
                    const base::string16& type,
                    const base::string16& file_name);

  IndexedDBBlobInfo(const IndexedDBBlobInfo& other);
  ~IndexedDBBlobInfo();
  IndexedDBBlobInfo& operator=(const IndexedDBBlobInfo& other);

  bool is_file() const { return is_file_; }
  const std::string& uuid() const { return uuid_; }
  const base::string16& type() const { return type_; }
  int64_t size() const { return size_; }
  const base::string16& file_name() const { return file_name_; }
  int64_t key() const { return key_; }
  const base::FilePath& file_path() const { return file_path_; }
  const base::Time& last_modified() const { return last_modified_; }
  const base::Closure& mark_used_callback() const {
    return mark_used_callback_;
  }
  const ReleaseCallback& release_callback() const { return release_callback_; }

  void set_size(int64_t size);
  void set_uuid(const std::string& uuid);
  void set_file_path(const base::FilePath& file_path);
  void set_last_modified(const base::Time& time);
  void set_key(int64_t key);
  void set_mark_used_callback(const base::Closure& mark_used_callback);
  void set_release_callback(const ReleaseCallback& release_callback);

 private:
  bool is_file_;
  std::string uuid_;          // Always for Blob; sometimes for File.
  base::string16 type_;       // Mime type.
  int64_t size_;              // -1 if unknown for File.
  base::string16 file_name_;  // Only for File.
  base::FilePath file_path_;  // Only for File.
  base::Time last_modified_;  // Only for File; valid only if size is.

  // Valid only when this comes out of the database.
  int64_t key_;
  base::Closure mark_used_callback_;
  ReleaseCallback release_callback_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_BLOB_INFO_H_
