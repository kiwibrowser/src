// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "compressor_archive_minizip.h"

#include <cerrno>
#include <cstring>

#include "base/time/time.h"
#include "ppapi/cpp/logging.h"

namespace {

uint32_t UnixToDosdate(const int64_t datetime) {
  tm tm_datetime;
  localtime_r(&datetime, &tm_datetime);

  return (tm_datetime.tm_year - 80) << 25 | (tm_datetime.tm_mon + 1) << 21 |
         tm_datetime.tm_mday << 16 | tm_datetime.tm_hour << 11 |
         tm_datetime.tm_min << 5 | (tm_datetime.tm_sec >> 1);
}

};  // namespace

namespace compressor_archive_functions {

// Called when minizip tries to open a zip archive file. We do nothing here
// because JavaScript takes care of file opening operation.
void* CustomArchiveOpen(void* compressor,
                        const char* /*filename*/,
                        int /*mode*/) {
  return compressor;
}

// This function is not called because we don't unpack zip files here.
uint32_t CustomArchiveRead(void* /*compressor*/,
                           void* /*stream*/,
                           void* /*buffur*/,
                           uint32_t /*size*/) {
  return 0 /* Success */;
}

// Called when data chunk must be written on the archive. It copies data
// from the given buffer processed by minizip to an array buffer and passes
// it to compressor_stream.
uint32_t CustomArchiveWrite(void* compressor,
                            void* /*stream*/,
                            const void* zip_buffer,
                            uint32_t zip_length) {
  CompressorArchiveMinizip* compressor_minizip =
      static_cast<CompressorArchiveMinizip*>(compressor);

  int64_t written_bytes = compressor_minizip->compressor_stream()->Write(
      compressor_minizip->offset(), zip_length,
      static_cast<const char*>(zip_buffer));

  if (written_bytes != zip_length)
    return 0 /* Error */;

  // Update offset_ and length_.
  compressor_minizip->set_offset(compressor_minizip->offset() + written_bytes);
  if (compressor_minizip->offset() > compressor_minizip->length())
    compressor_minizip->set_length(compressor_minizip->offset());
  return static_cast<uLong>(written_bytes);
}

// Returns the offset from the beginning of the data.
long CustomArchiveTell(void* compressor, void* /*stream*/) {
  CompressorArchiveMinizip* compressor_minizip =
      static_cast<CompressorArchiveMinizip*>(compressor);
  return static_cast<long>(compressor_minizip->offset_);
}

// Moves the current offset to the specified position.
long CustomArchiveSeek(void* compressor,
                       void* /*stream*/,
                       uint32_t offset,
                       int origin) {
  CompressorArchiveMinizip* compressor_minizip =
      static_cast<CompressorArchiveMinizip*>(compressor);

  if (origin == ZLIB_FILEFUNC_SEEK_CUR) {
    compressor_minizip->set_offset(
        std::min(compressor_minizip->offset() + static_cast<int64_t>(offset),
                 compressor_minizip->length()));
    return 0 /* Success */;
  }
  if (origin == ZLIB_FILEFUNC_SEEK_END) {
    compressor_minizip->set_offset(std::max(
        compressor_minizip->length() - static_cast<int64_t>(offset), 0LL));
    return 0 /* Success */;
  }
  if (origin == ZLIB_FILEFUNC_SEEK_SET) {
    compressor_minizip->set_offset(
        std::min(static_cast<int64_t>(offset), compressor_minizip->length()));
    return 0 /* Success */;
  }
  return -1 /* Error */;
}

// Releases all used resources. compressor points to compressor_minizip and
// it is deleted in the destructor of Compressor, so we don't need to delete
// it here.
int CustomArchiveClose(void* /*compressor*/, void* /*stream*/) {
  return 0 /* Success */;
}

// Returns the last error that happened when writing data. This function always
// returns zero, which means there are no errors.
int CustomArchiveError(void* /*compressor*/, void* /*stream*/) {
  return 0 /* Success */;
}

}  // namespace compressor_archive_functions

CompressorArchiveMinizip::CompressorArchiveMinizip(
    CompressorStream* compressor_stream)
    : CompressorArchive(compressor_stream),
      compressor_stream_(compressor_stream),
      zip_file_(nullptr),
      offset_(0),
      length_(0) {
  destination_buffer_ =
      new char[compressor_stream_constants::kMaximumDataChunkSize];
}

CompressorArchiveMinizip::~CompressorArchiveMinizip() {
  delete destination_buffer_;
}

bool CompressorArchiveMinizip::CreateArchive() {
  // Set up archive object.
  zlib_filefunc_def zip_funcs;
  zip_funcs.zopen_file = compressor_archive_functions::CustomArchiveOpen;
  zip_funcs.zread_file = compressor_archive_functions::CustomArchiveRead;
  zip_funcs.zwrite_file = compressor_archive_functions::CustomArchiveWrite;
  zip_funcs.ztell_file = compressor_archive_functions::CustomArchiveTell;
  zip_funcs.zseek_file = compressor_archive_functions::CustomArchiveSeek;
  zip_funcs.zclose_file = compressor_archive_functions::CustomArchiveClose;
  zip_funcs.zerror_file = compressor_archive_functions::CustomArchiveError;
  zip_funcs.opaque = this;

  zip_file_ = zipOpen2(nullptr /* pathname */, APPEND_STATUS_CREATE,
                       nullptr /* globalcomment */, &zip_funcs);
  if (!zip_file_) {
    set_error_message(compressor_archive_constants::kCreateArchiveError);
    return false /* Error */;
  }
  return true /* Success */;
}

bool CompressorArchiveMinizip::AddToArchive(const std::string& filename,
                                            int64_t file_size,
                                            int64_t modification_time,
                                            bool is_directory) {
  // Minizip takes filenames that end with '/' as directories.
  std::string normalized_filename = filename;
  if (is_directory)
    normalized_filename += "/";

  // Fill zipfileMetadata with modification_time.
  zip_fileinfo zipfileMetadata;
  // modification_time is millisecond-based, while FromTimeT takes seconds.
  zipfileMetadata.dos_date = UnixToDosdate((int64_t)modification_time / 1000);

  // Section 4.4.4 http://www.pkware.com/documents/casestudies/APPNOTE.TXT
  // Setting the Language encoding flag so the file is told to be in utf-8.
  const uLong LANGUAGE_ENCODING_FLAG = 0x1 << 11;

  // Indicates the compatibility of the file attribute information.
  // Attributes of files are not avaiable in the FileSystem API. Therefore
  // we don't store file attributes to an archive. However, other apps may use
  // this field to determine the line record format for text files etc.
  const int HOST_SYSTEM_CODE = 3;  // UNIX

  // PKWARE .ZIP File Format Specification version 6.3.x
  const int ZIP_SPECIFICATION_VERSION_CODE = 63;

  const int VERSION_MADE_BY =
      HOST_SYSTEM_CODE << 8 | ZIP_SPECIFICATION_VERSION_CODE;

  int open_result =
      zipOpenNewFileInZip4(zip_file_,                    // file
                           normalized_filename.c_str(),  // filename
                           &zipfileMetadata,             // zipfi
                           nullptr,                      // extrafield_local
                           0u,                       // size_extrafield_local
                           nullptr,                  // extrafield_global
                           0u,                       // size_extrafield_global
                           nullptr,                  // comment
                           Z_DEFLATED,               // method
                           Z_DEFAULT_COMPRESSION,    // level
                           0,                        // raw
                           -MAX_WBITS,               // windowBits
                           DEF_MEM_LEVEL,            // memLevel
                           Z_DEFAULT_STRATEGY,       // strategy
                           nullptr,                  // password
                           0,                        // crcForCrypting
                           VERSION_MADE_BY,          // versionMadeBy
                           LANGUAGE_ENCODING_FLAG);  // flagBase
  if (open_result != ZIP_OK) {
    CloseArchive(true /* has_error */);
    set_error_message(compressor_archive_constants::kAddToArchiveError);
    return false /* Error */;
  }

  bool has_error = false;
  if (!is_directory) {
    int64_t remaining_size = file_size;
    while (remaining_size > 0) {
      int64_t chunk_size = std::min(
          remaining_size, compressor_stream_constants::kMaximumDataChunkSize);
      PP_DCHECK(chunk_size > 0);

      int64_t read_bytes =
          compressor_stream_->Read(chunk_size, destination_buffer_);
      // Negative read_bytes indicates an error occurred when reading chunks.
      // 0 just means there is no more data available, but here we need positive
      // length of bytes, so this is also an error here.
      if (read_bytes <= 0) {
        has_error = true;
        break;
      }

      if (canceled_) {
        break;
      }

      if (zipWriteInFileInZip(zip_file_, destination_buffer_, read_bytes) !=
          ZIP_OK) {
        has_error = true;
        break;
      }
      remaining_size -= read_bytes;
    }
  }

  if (!has_error && zipCloseFileInZip(zip_file_) != ZIP_OK)
    has_error = true;

  if (has_error) {
    CloseArchive(true /* has_error */);
    set_error_message(compressor_archive_constants::kAddToArchiveError);
    return false /* Error */;
  }

  if (canceled_) {
    CloseArchive(true /* has_error */);
    return false /* Error */;
  }

  return true /* Success */;
}

bool CompressorArchiveMinizip::CloseArchive(bool has_error) {
  if (zipClose(zip_file_, nullptr /* global_comment */) != ZIP_OK) {
    set_error_message(compressor_archive_constants::kCloseArchiveError);
    return false /* Error */;
  }
  if (!has_error) {
    if (compressor_stream()->Flush() < 0) {
      set_error_message(compressor_archive_constants::kCloseArchiveError);
      return false /* Error */;
    }
  }
  return true /* Success */;
}

void CompressorArchiveMinizip::CancelArchive() {
  canceled_ = true;
}
