// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/test_support.h"

#include <stdio.h>
#include <string.h>

#include "core/fdrm/crypto/fx_crypt.h"
#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/fx_string.h"
#include "testing/utils/path_service.h"

#ifdef PDF_ENABLE_V8
#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"
#endif

namespace {

#ifdef PDF_ENABLE_V8
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
// Returns the full path for an external V8 data file based on either
// the currect exectuable path or an explicit override.
std::string GetFullPathForSnapshotFile(const std::string& exe_path,
                                       const std::string& bin_dir,
                                       const std::string& filename) {
  std::string result;
  if (!bin_dir.empty()) {
    result = bin_dir;
    if (*bin_dir.rbegin() != PATH_SEPARATOR) {
      result += PATH_SEPARATOR;
    }
  } else if (!exe_path.empty()) {
    size_t last_separator = exe_path.rfind(PATH_SEPARATOR);
    if (last_separator != std::string::npos) {
      result = exe_path.substr(0, last_separator + 1);
    }
  }
  result += filename;
  return result;
}

bool GetExternalData(const std::string& exe_path,
                     const std::string& bin_dir,
                     const std::string& filename,
                     v8::StartupData* result_data) {
  std::string full_path =
      GetFullPathForSnapshotFile(exe_path, bin_dir, filename);
  size_t data_length = 0;
  std::unique_ptr<char, pdfium::FreeDeleter> data_buffer =
      GetFileContents(full_path.c_str(), &data_length);
  if (!data_buffer)
    return false;

  result_data->data = data_buffer.release();
  result_data->raw_size = data_length;
  return true;
}
#endif  // V8_USE_EXTERNAL_STARTUP_DATA

std::unique_ptr<v8::Platform> InitializeV8Common(const std::string& exe_path) {
  v8::V8::InitializeICUDefaultLocation(exe_path.c_str());

  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());

  // By enabling predictable mode, V8 won't post any background tasks.
  // By enabling GC, it makes it easier to chase use-after-free.
  const char v8_flags[] = "--predictable --expose-gc";
  v8::V8::SetFlagsFromString(v8_flags, static_cast<int>(strlen(v8_flags)));
  v8::V8::Initialize();
  return platform;
}
#endif  // PDF_ENABLE_V8

}  // namespace

std::unique_ptr<char, pdfium::FreeDeleter> GetFileContents(const char* filename,
                                                           size_t* retlen) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Failed to open: %s\n", filename);
    return nullptr;
  }
  (void)fseek(file, 0, SEEK_END);
  size_t file_length = ftell(file);
  if (!file_length) {
    return nullptr;
  }
  (void)fseek(file, 0, SEEK_SET);
  std::unique_ptr<char, pdfium::FreeDeleter> buffer(
      static_cast<char*>(malloc(file_length)));
  if (!buffer) {
    return nullptr;
  }
  size_t bytes_read = fread(buffer.get(), 1, file_length, file);
  (void)fclose(file);
  if (bytes_read != file_length) {
    fprintf(stderr, "Failed to read: %s\n", filename);
    return nullptr;
  }
  *retlen = bytes_read;
  return buffer;
}

std::string GetPlatformString(FPDF_WIDESTRING wstr) {
  WideString wide_string =
      WideString::FromUTF16LE(wstr, WideString::WStringLength(wstr));
  return std::string(wide_string.UTF8Encode().c_str());
}

std::wstring GetPlatformWString(FPDF_WIDESTRING wstr) {
  if (!wstr)
    return nullptr;

  size_t characters = 0;
  while (wstr[characters])
    ++characters;

  std::wstring platform_string(characters, L'\0');
  for (size_t i = 0; i < characters + 1; ++i) {
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&wstr[i]);
    platform_string[i] = ptr[0] + 256 * ptr[1];
  }
  return platform_string;
}

std::vector<std::string> StringSplit(const std::string& str, char delimiter) {
  std::vector<std::string> result;
  size_t pos = 0;
  while (1) {
    size_t found = str.find(delimiter, pos);
    if (found == std::string::npos)
      break;

    result.push_back(str.substr(pos, found - pos));
    pos = found + 1;
  }
  result.push_back(str.substr(pos));
  return result;
}

std::unique_ptr<unsigned short, pdfium::FreeDeleter> GetFPDFWideString(
    const std::wstring& wstr) {
  size_t length = sizeof(uint16_t) * (wstr.length() + 1);
  std::unique_ptr<unsigned short, pdfium::FreeDeleter> result(
      static_cast<unsigned short*>(malloc(length)));
  char* ptr = reinterpret_cast<char*>(result.get());
  size_t i = 0;
  for (wchar_t w : wstr) {
    ptr[i++] = w & 0xff;
    ptr[i++] = (w >> 8) & 0xff;
  }
  ptr[i++] = 0;
  ptr[i] = 0;
  return result;
}

std::string CryptToBase16(const uint8_t* digest) {
  static char const zEncode[] = "0123456789abcdef";
  std::string ret;
  ret.resize(32);
  for (int i = 0, j = 0; i < 16; i++, j += 2) {
    uint8_t a = digest[i];
    ret[j] = zEncode[(a >> 4) & 0xf];
    ret[j + 1] = zEncode[a & 0xf];
  }
  return ret;
}

std::string GenerateMD5Base16(const uint8_t* data, uint32_t size) {
  uint8_t digest[16];
  CRYPT_MD5Generate(data, size, digest);
  return CryptToBase16(digest);
}

#ifdef PDF_ENABLE_V8
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
std::unique_ptr<v8::Platform> InitializeV8ForPDFiumWithStartupData(
    const std::string& exe_path,
    const std::string& bin_dir,
    v8::StartupData* natives_blob,
    v8::StartupData* snapshot_blob) {
  std::unique_ptr<v8::Platform> platform = InitializeV8Common(exe_path);
  if (natives_blob && snapshot_blob) {
    if (!GetExternalData(exe_path, bin_dir, "natives_blob.bin", natives_blob))
      return nullptr;
    if (!GetExternalData(exe_path, bin_dir, "snapshot_blob.bin", snapshot_blob))
      return nullptr;
    v8::V8::SetNativesDataBlob(natives_blob);
    v8::V8::SetSnapshotDataBlob(snapshot_blob);
  }
  return platform;
}
#else   // V8_USE_EXTERNAL_STARTUP_DATA
std::unique_ptr<v8::Platform> InitializeV8ForPDFium(
    const std::string& exe_path) {
  return InitializeV8Common(exe_path);
}
#endif  // V8_USE_EXTERNAL_STARTUP_DATA
#endif  // PDF_ENABLE_V8

TestLoader::TestLoader(const char* pBuf, size_t len)
    : m_pBuf(pBuf), m_Len(len) {
}

// static
int TestLoader::GetBlock(void* param,
                         unsigned long pos,
                         unsigned char* pBuf,
                         unsigned long size) {
  TestLoader* pLoader = static_cast<TestLoader*>(param);
  if (pos + size < pos || pos + size > pLoader->m_Len)
    return 0;

  memcpy(pBuf, pLoader->m_pBuf + pos, size);
  return 1;
}
