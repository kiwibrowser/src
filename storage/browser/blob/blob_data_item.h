// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_BLOB_DATA_ITEM_H_
#define STORAGE_BROWSER_BLOB_BLOB_DATA_ITEM_H_

#include <stdint.h>

#include <memory>
#include <ostream>
#include <string>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "storage/browser/storage_browser_export.h"
#include "url/gurl.h"

namespace disk_cache {
class Entry;
}

namespace storage {
class BlobDataBuilder;
class BlobStorageContext;
class FileSystemContext;

// Ref counted blob item. This class owns the backing data of the blob item. The
// backing data is immutable, and cannot change after creation. The purpose of
// this class is to allow the resource to stick around in the snapshot even
// after the resource was swapped in the blob (either to disk or to memory) by
// the BlobStorageContext.
class STORAGE_EXPORT BlobDataItem : public base::RefCounted<BlobDataItem> {
 public:
  static constexpr uint64_t kUnknownSize = std::numeric_limits<uint64_t>::max();

  enum class Type {
    kBytes,
    kBytesDescription,
    kFile,
    kFileFilesystem,
    kDiskCacheEntry
  };

  // The DataHandle class is used to persist resources that are needed for
  // reading this BlobDataItem. This object will stay around while any reads are
  // pending. If all blobs with this item are deleted or the item is swapped for
  // a different backend version (mem-to-disk or the reverse), then the item
  // will be destructed after all pending reads are complete.
  class STORAGE_EXPORT DataHandle : public base::RefCounted<DataHandle> {
   protected:
    virtual ~DataHandle() = 0;

   private:
    friend class base::RefCounted<DataHandle>;
  };

  static scoped_refptr<BlobDataItem> CreateBytes(base::span<const char> bytes);
  static scoped_refptr<BlobDataItem> CreateBytesDescription(size_t length);
  static scoped_refptr<BlobDataItem> CreateFile(base::FilePath path);
  static scoped_refptr<BlobDataItem> CreateFile(
      base::FilePath path,
      uint64_t offset,
      uint64_t length,
      base::Time expected_modification_time = base::Time(),
      scoped_refptr<DataHandle> data_handle = nullptr);
  static scoped_refptr<BlobDataItem> CreateFutureFile(uint64_t offset,
                                                      uint64_t length,
                                                      uint64_t file_id);
  static scoped_refptr<BlobDataItem> CreateFileFilesystem(
      const GURL& url,
      uint64_t offset,
      uint64_t length,
      base::Time expected_modification_time,
      scoped_refptr<FileSystemContext> file_system_context);
  static scoped_refptr<BlobDataItem> CreateDiskCacheEntry(
      uint64_t offset,
      uint64_t length,
      scoped_refptr<DataHandle> data_handle,
      disk_cache::Entry* entry,
      int disk_cache_stream_index,
      int disk_cache_side_stream_index);

  Type type() const { return type_; }
  uint64_t offset() const { return offset_; }
  uint64_t length() const { return length_; }

  base::span<const char> bytes() const {
    DCHECK_EQ(type_, Type::kBytes);
    return base::make_span(bytes_);
  }

  const base::FilePath& path() const {
    DCHECK_EQ(type_, Type::kFile);
    return path_;
  }

  const GURL& filesystem_url() const {
    DCHECK_EQ(type_, Type::kFileFilesystem);
    return filesystem_url_;
  }

  FileSystemContext* file_system_context() const {
    DCHECK_EQ(type_, Type::kFileFilesystem);
    return file_system_context_.get();
  }

  const base::Time& expected_modification_time() const {
    DCHECK(type_ == Type::kFile || type_ == Type::kFileFilesystem)
        << static_cast<int>(type_);
    return expected_modification_time_;
  }

  disk_cache::Entry* disk_cache_entry() const {
    DCHECK_EQ(type_, Type::kDiskCacheEntry);
    return disk_cache_entry_;
  }

  int disk_cache_stream_index() const {
    DCHECK_EQ(type_, Type::kDiskCacheEntry);
    return disk_cache_stream_index_;
  }

  int disk_cache_side_stream_index() const {
    DCHECK_EQ(type_, Type::kDiskCacheEntry);
    return disk_cache_side_stream_index_;
  }

  DataHandle* data_handle() const {
    DCHECK(type_ == Type::kFile || type_ == Type::kDiskCacheEntry)
        << static_cast<int>(type_);
    return data_handle_.get();
  }

  // Returns true if this item was created by CreateFutureFile.
  bool IsFutureFileItem() const;
  // Returns |file_id| given to CreateFutureFile.
  uint64_t GetFutureFileID() const;

 private:
  friend class BlobBuilderFromStream;
  friend class BlobDataBuilder;
  friend class BlobStorageContext;
  friend class base::RefCounted<BlobDataItem>;
  friend STORAGE_EXPORT void PrintTo(const BlobDataItem& x, ::std::ostream* os);

  BlobDataItem(Type type, uint64_t offset, uint64_t length);
  virtual ~BlobDataItem();

  base::span<char> mutable_bytes() {
    DCHECK_EQ(type_, Type::kBytes);
    return base::make_span(bytes_);
  }

  void AllocateBytes();
  void PopulateBytes(base::span<const char> data);
  void ShrinkBytes(size_t new_length);

  void PopulateFile(base::FilePath path,
                    base::Time expected_modification_time,
                    scoped_refptr<DataHandle> data_handle);
  void ShrinkFile(uint64_t new_length);
  void GrowFile(uint64_t new_length);
  void SetFileModificationTime(base::Time time) {
    DCHECK_EQ(type_, Type::kFile);
    expected_modification_time_ = time;
  }

  Type type_;
  uint64_t offset_;
  uint64_t length_;

  std::vector<char> bytes_;  // For Type::kBytes.
  base::FilePath path_;      // For Type::kFile.
  GURL filesystem_url_;      // For Type::kFileFilesystem.
  base::Time
      expected_modification_time_;  // For Type::kFile and kFileFilesystem.

  scoped_refptr<DataHandle>
      data_handle_;  // For Type::kFile and kDiskCacheEntry.

  // This naked pointer is safe because the scope is protected by the DataHandle
  // instance for disk cache entries during the lifetime of this BlobDataItem.
  disk_cache::Entry* disk_cache_entry_;  // For Type::kDiskCacheEntry.
  int disk_cache_stream_index_;          // For Type::kDiskCacheEntry.
  int disk_cache_side_stream_index_;     // For Type::kDiskCacheEntry.

  scoped_refptr<FileSystemContext>
      file_system_context_;  // For Type::kFileFilesystem.
};

STORAGE_EXPORT bool operator==(const BlobDataItem& a, const BlobDataItem& b);
STORAGE_EXPORT bool operator!=(const BlobDataItem& a, const BlobDataItem& b);

}  // namespace storage

#endif  // STORAGE_BROWSER_BLOB_BLOB_DATA_ITEM_H_
