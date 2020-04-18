// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_data_item.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "storage/browser/fileapi/file_system_context.h"

namespace storage {

namespace {
const base::FilePath::CharType kFutureFileName[] =
    FILE_PATH_LITERAL("_future_name_");
}

constexpr uint64_t BlobDataItem::kUnknownSize;

BlobDataItem::DataHandle::~DataHandle() = default;

// static
scoped_refptr<BlobDataItem> BlobDataItem::CreateBytes(
    base::span<const char> bytes) {
  auto item =
      base::WrapRefCounted(new BlobDataItem(Type::kBytes, 0, bytes.size()));
  item->bytes_.assign(bytes.begin(), bytes.end());
  return item;
}

// static
scoped_refptr<BlobDataItem> BlobDataItem::CreateBytesDescription(
    size_t length) {
  return base::WrapRefCounted(
      new BlobDataItem(Type::kBytesDescription, 0, length));
}

// static
scoped_refptr<BlobDataItem> BlobDataItem::CreateFile(base::FilePath path) {
  return CreateFile(path, 0, kUnknownSize);
}

// static
scoped_refptr<BlobDataItem> BlobDataItem::CreateFile(
    base::FilePath path,
    uint64_t offset,
    uint64_t length,
    base::Time expected_modification_time,
    scoped_refptr<DataHandle> data_handle) {
  auto item =
      base::WrapRefCounted(new BlobDataItem(Type::kFile, offset, length));
  item->path_ = std::move(path);
  item->expected_modification_time_ = std::move(expected_modification_time);
  item->data_handle_ = std::move(data_handle);
  // TODO(mek): DCHECK(!item->IsFutureFileItem()) when BlobDataBuilder has some
  // other way of slicing a future file.
  return item;
}

// static
scoped_refptr<BlobDataItem> BlobDataItem::CreateFutureFile(uint64_t offset,
                                                           uint64_t length,
                                                           uint64_t file_id) {
  auto item =
      base::WrapRefCounted(new BlobDataItem(Type::kFile, offset, length));
  std::string file_id_str = base::NumberToString(file_id);
  item->path_ = base::FilePath(kFutureFileName)
                    .AddExtension(base::FilePath::StringType(
                        file_id_str.begin(), file_id_str.end()));
  return item;
}

// static
scoped_refptr<BlobDataItem> BlobDataItem::CreateFileFilesystem(
    const GURL& url,
    uint64_t offset,
    uint64_t length,
    base::Time expected_modification_time,
    scoped_refptr<FileSystemContext> file_system_context) {
  auto item = base::WrapRefCounted(
      new BlobDataItem(Type::kFileFilesystem, offset, length));
  item->filesystem_url_ = url;
  item->expected_modification_time_ = std::move(expected_modification_time);
  item->file_system_context_ = std::move(file_system_context);
  return item;
}

// static
scoped_refptr<BlobDataItem> BlobDataItem::CreateDiskCacheEntry(
    uint64_t offset,
    uint64_t length,
    scoped_refptr<DataHandle> data_handle,
    disk_cache::Entry* entry,
    int disk_cache_stream_index,
    int disk_cache_side_stream_index) {
  auto item = base::WrapRefCounted(
      new BlobDataItem(Type::kDiskCacheEntry, offset, length));
  item->data_handle_ = std::move(data_handle);
  item->disk_cache_entry_ = entry;
  item->disk_cache_stream_index_ = disk_cache_stream_index;
  item->disk_cache_side_stream_index_ = disk_cache_side_stream_index;
  return item;
}

bool BlobDataItem::IsFutureFileItem() const {
  if (type_ != Type::kFile)
    return false;
  const base::FilePath::StringType prefix(kFutureFileName);
  // The prefix shouldn't occur unless the user used "AppendFutureFile". We
  // DCHECK on AppendFile to make sure no one appends a future file.
  return base::StartsWith(path().value(), prefix, base::CompareCase::SENSITIVE);
}

uint64_t BlobDataItem::GetFutureFileID() const {
  DCHECK(IsFutureFileItem());
  uint64_t id = 0;
  bool success = base::StringToUint64(path().Extension().substr(1), &id);
  DCHECK(success) << path().Extension();
  return id;
}

BlobDataItem::BlobDataItem(Type type, uint64_t offset, uint64_t length)
    : type_(type), offset_(offset), length_(length) {}

BlobDataItem::~BlobDataItem() = default;

void BlobDataItem::AllocateBytes() {
  DCHECK_EQ(type_, Type::kBytesDescription);
  bytes_.resize(length_);
  type_ = Type::kBytes;
}

void BlobDataItem::PopulateBytes(base::span<const char> data) {
  DCHECK_EQ(type_, Type::kBytesDescription);
  DCHECK_EQ(length_, data.size());
  type_ = Type::kBytes;
  bytes_.assign(data.begin(), data.end());
}

void BlobDataItem::ShrinkBytes(size_t new_length) {
  DCHECK_EQ(type_, Type::kBytes);
  length_ = new_length;
  bytes_.resize(length_);
}

void BlobDataItem::PopulateFile(base::FilePath path,
                                base::Time expected_modification_time,
                                scoped_refptr<DataHandle> data_handle) {
  DCHECK_EQ(type_, Type::kFile);
  DCHECK(IsFutureFileItem());
  path_ = std::move(path);
  expected_modification_time_ = std::move(expected_modification_time);
  data_handle_ = std::move(data_handle);
}

void BlobDataItem::ShrinkFile(uint64_t new_length) {
  DCHECK_EQ(type_, Type::kFile);
  DCHECK_LE(new_length, length_);
  length_ = new_length;
}

void BlobDataItem::GrowFile(uint64_t new_length) {
  DCHECK_EQ(type_, Type::kFile);
  DCHECK_GE(new_length, length_);
  length_ = new_length;
}

void PrintTo(const BlobDataItem& x, ::std::ostream* os) {
  const uint64_t kMaxDataPrintLength = 40;
  DCHECK(os);
  *os << "<BlobDataItem>{type: ";
  switch (x.type()) {
    case BlobDataItem::Type::kBytes: {
      uint64_t length = std::min(x.length(), kMaxDataPrintLength);
      *os << "kBytes, data: ["
          << base::HexEncode(x.bytes().data(), static_cast<size_t>(length));
      if (length < x.length()) {
        *os << "<...truncated due to length...>";
      }
      *os << "]";
      break;
    }
    case BlobDataItem::Type::kBytesDescription:
      *os << "kBytesDescription";
      break;
    case BlobDataItem::Type::kFile:
      *os << "kFile, path: " << x.path().AsUTF8Unsafe()
          << ", expected_modification_time: " << x.expected_modification_time();
      break;
    case BlobDataItem::Type::kFileFilesystem:
      *os << "kFileFilesystem, url: " << x.filesystem_url();
      break;
    case BlobDataItem::Type::kDiskCacheEntry:
      *os << "kDiskCacheEntry"
          << ", disk_cache_entry_ptr: " << x.disk_cache_entry_
          << ", disk_cache_stream_index_: " << x.disk_cache_stream_index_
          << "}";
      break;
  }
  *os << ", length: " << x.length() << ", offset: " << x.offset()
      << ", has_data_handle: " << (x.data_handle_.get() ? "true" : "false");
}

bool operator==(const BlobDataItem& a, const BlobDataItem& b) {
  if (a.type() != b.type() || a.offset() != b.offset() ||
      a.length() != b.length())
    return false;
  switch (a.type()) {
    case BlobDataItem::Type::kBytes:
      return a.bytes() == b.bytes();
    case BlobDataItem::Type::kBytesDescription:
      return true;
    case BlobDataItem::Type::kFile:
      return a.path() == b.path() &&
             a.expected_modification_time() == b.expected_modification_time();
    case BlobDataItem::Type::kFileFilesystem:
      return a.filesystem_url() == b.filesystem_url();
    case BlobDataItem::Type::kDiskCacheEntry:
      return a.disk_cache_entry() == b.disk_cache_entry() &&
             a.disk_cache_stream_index() == b.disk_cache_stream_index() &&
             a.disk_cache_side_stream_index() ==
                 b.disk_cache_side_stream_index();
  }
  NOTREACHED();
  return false;
}

bool operator!=(const BlobDataItem& a, const BlobDataItem& b) {
  return !(a == b);
}

}  // namespace storage
