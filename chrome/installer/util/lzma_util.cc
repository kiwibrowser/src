// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/lzma_util.h"

#include <stddef.h>

#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/installer/util/lzma_file_allocator.h"

extern "C" {
#include "third_party/lzma_sdk/7z.h"
#include "third_party/lzma_sdk/7zAlloc.h"
#include "third_party/lzma_sdk/7zCrc.h"
#include "third_party/lzma_sdk/7zFile.h"
}

namespace {

SRes LzmaReadFile(HANDLE file, void* data, size_t* size) {
  if (*size == 0)
    return SZ_OK;

  size_t processedSize = 0;
  DWORD maxSize = *size;
  do {
    DWORD processedLoc = 0;
    BOOL res = ReadFile(file, data, maxSize, &processedLoc, NULL);
    data = (void*)((unsigned char*)data + processedLoc);
    maxSize -= processedLoc;
    processedSize += processedLoc;
    if (processedLoc == 0) {
      if (res)
        return SZ_ERROR_READ;
      else
        break;
    }
  } while (maxSize > 0);

  *size = processedSize;
  return SZ_OK;
}

SRes SzFileSeekImp(void* object, Int64* pos, ESzSeek origin) {
  CFileInStream* s = (CFileInStream*)object;
  LARGE_INTEGER value;
  value.LowPart = (DWORD)*pos;
  value.HighPart = (LONG)((UInt64)*pos >> 32);
  DWORD moveMethod;
  switch (origin) {
    case SZ_SEEK_SET:
      moveMethod = FILE_BEGIN;
      break;
    case SZ_SEEK_CUR:
      moveMethod = FILE_CURRENT;
      break;
    case SZ_SEEK_END:
      moveMethod = FILE_END;
      break;
    default:
      return SZ_ERROR_PARAM;
  }
  value.LowPart = SetFilePointer(s->file.handle, value.LowPart, &value.HighPart,
                                 moveMethod);
  *pos = ((Int64)value.HighPart << 32) | value.LowPart;
  return ((value.LowPart == 0xFFFFFFFF) && (GetLastError() != ERROR_SUCCESS))
             ? SZ_ERROR_FAIL
             : SZ_OK;
}

SRes SzFileReadImp(void* object, void* buffer, size_t* size) {
  CFileInStream* s = (CFileInStream*)object;
  return LzmaReadFile(s->file.handle, buffer, size);
}

// Returns EXCEPTION_EXECUTE_HANDLER and populates |status| with the underlying
// NTSTATUS code for paging errors encountered while accessing file-backed
// mapped memory. Otherwise, return EXCEPTION_CONTINUE_SEARCH.
DWORD FilterPageError(const LzmaFileAllocator& file_allocator,
                      DWORD exception_code,
                      const EXCEPTION_POINTERS* info,
                      int32_t* status) {
  if (exception_code != EXCEPTION_IN_PAGE_ERROR)
    return EXCEPTION_CONTINUE_SEARCH;

  const EXCEPTION_RECORD* exception_record = info->ExceptionRecord;
  if (!file_allocator.IsAddressMapped(
          exception_record->ExceptionInformation[1])) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  // Cast NTSTATUS to int32_t to avoid including winternl.h
  *status = exception_record->ExceptionInformation[2];

  return EXCEPTION_EXECUTE_HANDLER;
}

}  // namespace

DWORD UnPackArchive(const base::FilePath& archive,
                    const base::FilePath& output_dir,
                    base::FilePath* output_file,
                    UnPackStatus* unpack_status,
                    int32_t* ntstatus) {
  VLOG(1) << "Opening archive " << archive.value();
  LzmaUtilImpl lzma_util;
  DWORD ret;
  if ((ret = lzma_util.OpenArchive(archive)) != ERROR_SUCCESS) {
    PLOG(ERROR) << "Unable to open install archive: " << archive.value();
  } else {
    VLOG(1) << "Uncompressing archive to path " << output_dir.value();
    if ((ret = lzma_util.UnPack(output_dir, output_file)) != ERROR_SUCCESS)
      PLOG(ERROR) << "Unable to uncompress archive: " << archive.value();
    lzma_util.CloseArchive();
  }
  if (unpack_status)
    *unpack_status = lzma_util.GetUnPackStatus();
  if (ntstatus)
    *ntstatus = lzma_util.GetNTSTATUSCode();
  return ret;
}

LzmaUtilImpl::LzmaUtilImpl() {}
LzmaUtilImpl::~LzmaUtilImpl() {
  CloseArchive();
}

DWORD LzmaUtilImpl::OpenArchive(const base::FilePath& archivePath) {
  // Make sure file is not already open.
  CloseArchive();

  DWORD ret = ERROR_SUCCESS;
  archive_handle_ =
      CreateFile(archivePath.value().c_str(), GENERIC_READ, FILE_SHARE_READ,
                 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (archive_handle_ == INVALID_HANDLE_VALUE) {
    archive_handle_ = NULL;  // The rest of the code only checks for NULL.
    ret = GetLastError();
    if (ret == ERROR_FILE_NOT_FOUND)
      unpack_status_ = UNPACK_ARCHIVE_NOT_FOUND;
    else
      unpack_status_ = UNPACK_ARCHIVE_CANNOT_OPEN;
  }
  return ret;
}

DWORD LzmaUtilImpl::UnPack(const base::FilePath& location) {
  return UnPack(location, NULL);
}

DWORD LzmaUtilImpl::UnPack(const base::FilePath& location,
                           base::FilePath* output_file) {
  if (!archive_handle_)
    return ERROR_INVALID_HANDLE;

  CFileInStream archiveStream;
  CLookToRead lookStream;
  CSzArEx db;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;
  DWORD ret = ERROR_SUCCESS;
  SRes sz_res = SZ_OK;

  archiveStream.file.handle = archive_handle_;
  archiveStream.s.Read = SzFileReadImp;
  archiveStream.s.Seek = SzFileSeekImp;
  LookToRead_CreateVTable(&lookStream, false);
  lookStream.realStream = &archiveStream.s;

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;
  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  CrcGenerateTable();
  SzArEx_Init(&db);
  if ((sz_res = SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp)) !=
      SZ_OK) {
    LOG(ERROR) << L"Error returned by SzArchiveOpen: " << sz_res;

    unpack_status_ = UNPACK_SZAREX_OPEN_ERROR;
    return ERROR_INVALID_HANDLE;
  }

  Byte* outBuffer = 0;  // it must be 0 before first call for each new archive
  UInt32 blockIndex = 0xFFFFFFFF;  // can have any value if outBuffer = 0
  size_t outBufferSize = 0;        // can have any value if outBuffer = 0

  LzmaFileAllocator fileAllocator(location);

  unpack_status_ = UNPACK_NO_ERROR;

  for (unsigned int i = 0; i < db.NumFiles; i++) {
    DWORD written;
    size_t offset = 0;
    size_t outSizeProcessed = 0;

    int32_t status = 0;  // STATUS_SUCCESS
    __try {
      if ((sz_res =
               SzArEx_Extract(&db, &lookStream.s, i, &blockIndex, &outBuffer,
                              &outBufferSize, &offset, &outSizeProcessed,
                              &fileAllocator, &allocTempImp)) != SZ_OK) {
        LOG(ERROR) << L"Error returned by SzExtract: " << sz_res;

        ret = ERROR_INVALID_HANDLE;
        unpack_status_ = UNPACK_EXTRACT_ERROR;
      }
    } __except(FilterPageError(fileAllocator, GetExceptionCode(),
                                GetExceptionInformation(), &status)) {
      ret = ERROR_IO_DEVICE;
      ntstatus_ = status;
      LOG(ERROR) << L"EXCEPTION_IN_PAGE_ERROR while accessing mapped memory; "
                    L"NTSTATUS = "
                 << ntstatus_;
      unpack_status_ = UNPACK_EXTRACT_EXCEPTION;
    }
    if (ret != ERROR_SUCCESS)
      break;

    size_t file_name_length = SzArEx_GetFileNameUtf16(&db, i, NULL);
    if (file_name_length < 1) {
      LOG(ERROR) << L"Couldn't get file name";
      ret = ERROR_INVALID_HANDLE;
      unpack_status_ = UNPACK_NO_FILENAME_ERROR;
      break;
    }

    std::vector<UInt16> file_name(file_name_length);
    SzArEx_GetFileNameUtf16(&db, i, &file_name[0]);
    // |file_name| is NULL-terminated.
    base::FilePath file_path = location.Append(
        base::FilePath::StringType(file_name.begin(), file_name.end() - 1));

    if (output_file)
      *output_file = file_path;

    // If archive entry is directory create it and move on to the next entry.
    if (SzArEx_IsDir(&db, i)) {
      CreateDirectory(file_path);
      continue;
    }

    CreateDirectory(file_path.DirName());

    HANDLE hFile;
    hFile = CreateFile(file_path.value().c_str(), GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      ret = GetLastError();
      LOG(ERROR) << L"Error returned by CreateFile: " << ret;
      unpack_status_ = UNPACK_CREATE_FILE_ERROR;
      break;
    }

    if ((!WriteFile(hFile, outBuffer + offset, (DWORD)outSizeProcessed,
                    &written, NULL)) ||
        (written != outSizeProcessed)) {
      ret = GetLastError();
      PLOG(ERROR) << L"Error returned by WriteFile";
      CloseHandle(hFile);
      unpack_status_ = UNPACK_WRITE_FILE_ERROR;
      break;
    }

    if (SzBitWithVals_Check(&db.MTime, i)) {
      if (!SetFileTime(hFile, NULL, NULL,
                       (const FILETIME*)(&db.MTime.Vals[i]))) {
        ret = GetLastError();
        PLOG(ERROR) << L"Error returned by SetFileTime";
        CloseHandle(hFile);
        unpack_status_ = UNPACK_SET_FILE_TIME_ERROR;
        break;
      }
    }
    if (!CloseHandle(hFile)) {
      ret = GetLastError();
      PLOG(ERROR) << L"Error returned by CloseHandle";
      unpack_status_ = UNPACK_CLOSE_FILE_ERROR;
      break;
    }
  }  // for loop
  IAlloc_Free(&fileAllocator, outBuffer);
  SzArEx_Free(&db, &allocImp);
  DCHECK_EQ(ret == static_cast<DWORD>(ERROR_SUCCESS),
            unpack_status_ == UNPACK_NO_ERROR);

  return ret;
}

void LzmaUtilImpl::CloseArchive() {
  if (archive_handle_) {
    CloseHandle(archive_handle_);
    archive_handle_ = NULL;
  }
}

bool LzmaUtilImpl::CreateDirectory(const base::FilePath& dir) {
  bool ret = true;
  if (directories_created_.find(dir.value()) == directories_created_.end()) {
    ret = base::CreateDirectory(dir);
    if (ret)
      directories_created_.insert(dir.value());
  }
  return ret;
}
