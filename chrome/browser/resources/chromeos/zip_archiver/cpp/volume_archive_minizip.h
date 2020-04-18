// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_CPP_VOLUME_ARCHIVE_MINIZIP_H_
#define CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_CPP_VOLUME_ARCHIVE_MINIZIP_H_

#include <string>

#include "third_party/minizip/src/unzip.h"
#include "third_party/minizip/src/zip.h"

#include "volume_archive.h"

// A namespace with constants used by VolumeArchiveMinizip.
namespace volume_archive_constants {

const char kArchiveReadNewError[] = "Could not allocate archive.";
const char kFileNotFound[] = "File not found for read data request.";
const char kVolumeReaderError[] = "VolumeReader failed to retrieve data.";
const char kArchiveOpenError[] = "Failed to open archive.";
const char kArchiveNextHeaderError[] =
    "Failed to open current file in archive.";
const char kArchiveReadDataError[] = "Failed to read archive data.";
const char kArchiveReadFreeError[] = "Failed to close archive.";

// The size of the buffer used to skip unnecessary data. Should be positive and
// UINT16_MAX or less. unzReadCurrentFile in third_party/minizip/src/unzip.c
// supports to read a data up to UINT16_MAX at a time.
const int64_t kDummyBufferSize = UINT16_MAX;  // ~64 KB

// The size of the buffer used by ReadInProgress to decompress data. Should be
// positive and UINT16_MAX or less. unzReadCurrentFile in
// third_party/minizip/src/unzip.c supports to read a data up to UINT16_MAX at a
// time.
const int64_t kDecompressBufferSize = UINT16_MAX;  // ~64 KB.

// The maximum data chunk size for VolumeReader::Read requests.
// Should be positive.
const int64_t kMaximumDataChunkSize = 512 * 1024;  // 512 KB.

// The minimum data chunk size for VolumeReader::Read requests.
// Should be positive.
const int64_t kMinimumDataChunkSize = 32 * 1024;  // 16 KB.

// Maximum length of filename in zip archive.
const int kZipMaxPath = 256;

// The size of the static cache. We need at least 64KB to cache whole
// 'end of central directory' data.
const int64_t kStaticCacheSize = 128 * 1024;

}  // namespace volume_archive_constants

class VolumeArchiveMinizip;

// A namespace with custom functions passed to minizip.
namespace volume_archive_functions {

int64_t DynamicCache(VolumeArchiveMinizip* archive, int64_t unz_size);

uint32_t CustomArchiveRead(void* archive,
                           void* stream,
                           void* buf,
                           uint32_t size);

// Returns the offset from the beginning of the data.
long CustomArchiveTell(void* archive, void* stream);

// Moves the current offset to the specified position.
long CustomArchiveSeek(void* archive,
                       void* stream,
                       uint32_t offset,
                       int origin);

}  // namespace volume_archive_functions

class VolumeArchiveMinizip;

// Defines an implementation of VolumeArchive that wraps all minizip
// operations.
class VolumeArchiveMinizip : public VolumeArchive {
 public:
  explicit VolumeArchiveMinizip(VolumeReader* reader);

  virtual ~VolumeArchiveMinizip();

  // See volume_archive_interface.h.
  virtual bool Init(const std::string& encoding);

  // See volume_archive_interface.h.
  virtual VolumeArchive::Result GetCurrentFileInfo(std::string* path_name,
                                                   bool* isEncodedInUtf8,
                                                   int64_t* size,
                                                   bool* is_directory,
                                                   time_t* modification_time);

  virtual VolumeArchive::Result GoToNextFile();

  // See volume_archive_interface.h.
  virtual bool SeekHeader(const std::string& path_name);

  // See volume_archive_interface.h.
  virtual int64_t ReadData(int64_t offset, int64_t length, const char** buffer);

  // See volume_archive_interface.h.
  virtual void MaybeDecompressAhead();

  // See volume_archive_interface.h.
  virtual bool Cleanup();

  int64_t reader_data_size() const { return reader_data_size_; }

  // Custom functions need to access private variables of
  // CompressorArchiveMinizip frequently.
  friend int64_t volume_archive_functions::DynamicCache(
      VolumeArchiveMinizip* va,
      int64_t unz_size);

  friend uint32_t volume_archive_functions::CustomArchiveRead(void* archive,
                                                              void* stream,
                                                              void* buf,
                                                              uint32_t size);

  friend long volume_archive_functions::CustomArchiveTell(void* archive,
                                                          void* stream);

  friend long volume_archive_functions::CustomArchiveSeek(void* archive,
                                                          void* stream,
                                                          uint32_t offset,
                                                          int origin);

 private:
  // Decompress length bytes of data starting from offset.
  void DecompressData(int64_t offset, int64_t length);

  // The size of the requested data from VolumeReader.
  int64_t reader_data_size_;

  // The minizip correspondent archive object.
  zipFile zip_file_;

  // We use two kinds of cache strategies here: dynamic and static.
  // Dynamic cache is a common cache strategy used in most of IO streams such as
  // fread. When a file chunk is requested and if the size of the requested
  // chunk is small, we load larger size of bytes from the archive and cache
  // them in dynamic_cache_. If the range of the next requested chunk is within
  // the cache, we don't read the archive and just return the data in the cache.
  char dynamic_cache_[volume_archive_constants::kMaximumDataChunkSize];

  // The offset from which dynamic_cache_ has the data of the archive.
  int64_t dynamic_cache_offset_;

  // The size of the data in dynamic_cache_.
  int64_t dynamic_cache_size_;

  // Although dynamic cache works in most situations, it doesn't work when
  // MiniZip is looking for the front index of the central directory. Since
  // MiniZip reads the data little by little backwards from the end to find the
  // index, dynamic_cache will be reloaded every time. To avoid this, we first
  // cache a certain length of data from the end into static_cache_. The data
  // in this buffer is also used when the data in the central directory is
  // requested by MiniZip later.
  char static_cache_[volume_archive_constants::kStaticCacheSize];

  // The offset from which static_cache_ has the data of the archive.
  int64_t static_cache_offset_;

  // The size of the data in static_cache_. The End Of Central Directory header
  // is guaranteed to be in the last 64(global comment) + 1(other fields) of the
  // file. This cache is used to store the header.
  int64_t static_cache_size_;

  // The data offset, which will be offset + length after last read
  // operation, where offset and length are method parameters for
  // VolumeArchiveMinizip::ReadData. Data offset is used to improve
  // performance for consecutive calls to VolumeArchiveMinizip::ReadData.
  //
  // Intead of starting the read from the beginning for every
  // VolumeArchiveMinizip::ReadData, the next call will start
  // from last_read_data_offset_ in case the offset parameter of
  // VolumeArchiveMinizip::ReadData has the same value as
  // last_read_data_offset_. This avoids decompressing again the bytes at
  // the begninning of the file, which is the average case scenario.
  // But in case the offset parameter is different than last_read_data_offset_,
  // then dummy_buffer_ will be used to ignore unused bytes.
  int64_t last_read_data_offset_;

  // The length of the last VolumeArchiveMinizip::ReadData. Used for
  // decompress ahead.
  int64_t last_read_data_length_;

  // Dummy buffer for unused data read using VolumeArchiveMinizip::ReadData.
  // Sometimes VolumeArchiveMinizip::ReadData can require reading from
  // offsets different from last_read_data_offset_. In this case some bytes
  // must be skipped. Because seeking is not possible inside compressed files,
  // the bytes will be discarded using this buffer.
  char dummy_buffer_[volume_archive_constants::kDummyBufferSize];

  // The address where the decompressed data starting from
  // decompressed_offset_ is stored. It should point to a valid location
  // inside decompressed_data_buffer_. Necesssary in order to NOT throw
  // away unused decompressed bytes as throwing them away would mean in some
  // situations restarting decompressing the file from the beginning.
  char* decompressed_data_;

  // The actual buffer that contains the decompressed data.
  char decompressed_data_buffer_
      [volume_archive_constants::kDecompressBufferSize];

  // The size of valid data starting from decompressed_data_ that is stored
  // inside decompressed_data_buffer_.
  int64_t decompressed_data_size_;

  // True if VolumeArchiveMinizip::DecompressData failed.
  bool decompressed_error_;

  // The password cache to access password protected files.
  std::unique_ptr<std::string> password_cache_;
};

#endif  // CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_CPP_VOLUME_ARCHIVE_MINIZIP_H_
