// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_FX_STREAM_H_
#define CORE_FXCRT_FX_STREAM_H_

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"

#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
#include <direct.h>

class CFindFileDataA;
typedef CFindFileDataA FX_FileHandle;

#else  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef DIR FX_FileHandle;
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_

FX_FileHandle* FX_OpenFolder(const char* path);
bool FX_GetNextFile(FX_FileHandle* handle, ByteString* filename, bool* bFolder);
void FX_CloseFolder(FX_FileHandle* handle);

#define FX_FILEMODE_ReadOnly 1
#define FX_FILEMODE_Truncate 2

class IFX_WriteStream : virtual public Retainable {
 public:
  virtual bool WriteBlock(const void* pData, size_t size) = 0;
  virtual bool WriteString(const ByteStringView& str) = 0;
};

class IFX_ArchiveStream : public IFX_WriteStream {
 public:
  virtual bool WriteByte(uint8_t byte) = 0;
  virtual bool WriteDWord(uint32_t i) = 0;
  virtual FX_FILESIZE CurrentOffset() const = 0;
};

class IFX_ReadStream : virtual public Retainable {
 public:
  virtual bool IsEOF() = 0;
  virtual FX_FILESIZE GetPosition() = 0;
  virtual size_t ReadBlock(void* buffer, size_t size) = 0;
};

class IFX_SeekableWriteStream : public IFX_WriteStream {
 public:
  // IFX_WriteStream:
  bool WriteBlock(const void* pData, size_t size) override;

  virtual FX_FILESIZE GetSize() = 0;
  virtual bool Flush() = 0;
  virtual bool WriteBlock(const void* pData,
                          FX_FILESIZE offset,
                          size_t size) = 0;
};

class IFX_SeekableReadStream : public IFX_ReadStream {
 public:
  static RetainPtr<IFX_SeekableReadStream> CreateFromFilename(
      const char* filename);

  // IFX_ReadStream:
  bool IsEOF() override;
  FX_FILESIZE GetPosition() override;
  size_t ReadBlock(void* buffer, size_t size) override;

  virtual bool ReadBlock(void* buffer, FX_FILESIZE offset, size_t size) = 0;
  virtual FX_FILESIZE GetSize() = 0;
};

class IFX_SeekableStream : public IFX_SeekableReadStream,
                           public IFX_SeekableWriteStream {
 public:
  static RetainPtr<IFX_SeekableStream> CreateFromFilename(const char* filename,
                                                          uint32_t dwModes);

  static RetainPtr<IFX_SeekableStream> CreateFromFilename(
      const wchar_t* filename,
      uint32_t dwModes);

  // IFX_SeekableReadStream:
  bool IsEOF() override = 0;
  FX_FILESIZE GetPosition() override = 0;
  size_t ReadBlock(void* buffer, size_t size) override = 0;
  bool ReadBlock(void* buffer, FX_FILESIZE offset, size_t size) override = 0;
  FX_FILESIZE GetSize() override = 0;

  // IFX_SeekableWriteStream:
  bool WriteBlock(const void* buffer,
                  FX_FILESIZE offset,
                  size_t size) override = 0;
  bool WriteBlock(const void* buffer, size_t size) override;
  bool WriteString(const ByteStringView& str) override;

  bool Flush() override = 0;
};

#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
class CFindFileData {
 public:
  virtual ~CFindFileData() {}
  HANDLE m_Handle;
  bool m_bEnd;
};

class CFindFileDataA : public CFindFileData {
 public:
  ~CFindFileDataA() override {}
  WIN32_FIND_DATAA m_FindData;
};
#endif

#endif  // CORE_FXCRT_FX_STREAM_H_
