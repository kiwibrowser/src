// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>  // NOLINT
#include <fcntl.h>  // for _O_* constants
#include <fdi.h>
#include <stddef.h>
#include <stdlib.h>

#include "chrome/installer/mini_installer/decompress.h"

namespace {

FNALLOC(Alloc) {
  return ::HeapAlloc(::GetProcessHeap(), 0, cb);
}

FNFREE(Free) {
  ::HeapFree(::GetProcessHeap(), 0, pv);
}

// Converts a wide string to utf8.  Set |len| to -1 if |str| is zero terminated
// and you want to convert the entire string.
// The returned string will have been allocated with Alloc(), so free it
// with a call to Free().
char* WideToUtf8(const wchar_t* str, int len) {
  char* ret = NULL;
  int size = WideCharToMultiByte(CP_UTF8, 0, str, len, NULL, 0, NULL, NULL);
  if (size) {
    if (len != -1)
      ++size;  // include space for the terminator.
    ret = reinterpret_cast<char*>(Alloc(size * sizeof(ret[0])));
    if (ret) {
      WideCharToMultiByte(CP_UTF8, 0, str, len, ret, size, NULL, NULL);
      if (len != -1)
        ret[size - 1] = '\0';  // terminate the string
    }
  }
  return ret;
}

wchar_t* Utf8ToWide(const char* str) {
  wchar_t* ret = NULL;
  int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  if (size) {
    ret = reinterpret_cast<wchar_t*>(Alloc(size * sizeof(ret[0])));
    if (ret)
      MultiByteToWideChar(CP_UTF8, 0, str, -1, ret, size);
  }
  return ret;
}

template <typename T>
class scoped_ptr {
 public:
  explicit scoped_ptr(T* a) : a_(a) {
  }
  ~scoped_ptr() {
    if (a_)
      Free(a_);
  }
  operator T*() {
    return a_;
  }
 private:
  T* a_;
};

FNOPEN(Open) {
  DWORD access = 0;
  DWORD disposition = 0;

  if (oflag & _O_RDWR) {
    access = GENERIC_READ | GENERIC_WRITE;
  } else if (oflag & _O_WRONLY) {
    access = GENERIC_WRITE;
  } else {
    access = GENERIC_READ;
  }

  if (oflag & _O_CREAT) {
    disposition = CREATE_ALWAYS;
  } else {
    disposition = OPEN_EXISTING;
  }

  scoped_ptr<wchar_t> path(Utf8ToWide(pszFile));
  HANDLE file = CreateFileW(path, access, FILE_SHARE_READ, NULL, disposition,
                            FILE_ATTRIBUTE_NORMAL, NULL);
  return reinterpret_cast<INT_PTR>(file);
}

FNREAD(Read) {
  DWORD read = 0;
  if (!::ReadFile(reinterpret_cast<HANDLE>(hf), pv, cb, &read, NULL))
    read = static_cast<DWORD>(-1L);
  return read;
}

FNWRITE(Write) {
  DWORD written = 0;
  if (!::WriteFile(reinterpret_cast<HANDLE>(hf), pv, cb, &written, NULL))
    written = static_cast<DWORD>(-1L);
  return written;
}

FNCLOSE(Close) {
  return ::CloseHandle(reinterpret_cast<HANDLE>(hf)) ? 0 : -1;
}

FNSEEK(Seek) {
  return ::SetFilePointer(reinterpret_cast<HANDLE>(hf), dist, NULL, seektype);
}

FNFDINOTIFY(Notify) {
  INT_PTR result = 0;

  // Since we will only ever be decompressing a single file at a time
  // we take a shortcut and provide a pointer to the wide destination file
  // of the file we want to write.  This way we don't have to bother with
  // utf8/wide conversion and concatenation of directory and file name.
  const wchar_t* destination = reinterpret_cast<const wchar_t*>(pfdin->pv);

  switch (fdint) {
    case fdintCOPY_FILE: {
      result = reinterpret_cast<INT_PTR>(::CreateFileW(destination,
          GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL, NULL));
      break;
    }

    case fdintCLOSE_FILE_INFO: {
      FILETIME file_time;
      FILETIME local;
      // Converts MS-DOS date and time values to a file time
      if (DosDateTimeToFileTime(pfdin->date, pfdin->time, &file_time) &&
          LocalFileTimeToFileTime(&file_time, &local)) {
        SetFileTime(reinterpret_cast<HANDLE>(pfdin->hf), &local, NULL, NULL);
      }

      result = !Close(pfdin->hf);
      pfdin->attribs &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN |
                        FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE;
      ::SetFileAttributes(destination, pfdin->attribs);
      break;
    }

    case fdintCABINET_INFO:
    case fdintENUMERATE:
      // OK. continue as normal.
      result = 0;
      break;

    case fdintPARTIAL_FILE:
    case fdintNEXT_CABINET:
    default:
      // Error case.
      result = -1;
      break;
  }

  return result;
}

// Module handle of cabinet.dll
HMODULE g_fdi = NULL;

// API prototypes.
typedef HFDI (DIAMONDAPI* FDICreateFn)(PFNALLOC alloc, PFNFREE free,
                                       PFNOPEN open, PFNREAD read,
                                       PFNWRITE write, PFNCLOSE close,
                                       PFNSEEK seek, int cpu_type, PERF perf);
typedef BOOL (DIAMONDAPI* FDIDestroyFn)(HFDI fdi);
typedef BOOL (DIAMONDAPI* FDICopyFn)(HFDI fdi, char* cab, char* cab_path,
                                     int flags, PFNFDINOTIFY notify,
                                     PFNFDIDECRYPT decrypt, void* context);
FDICreateFn g_FDICreate = NULL;
FDIDestroyFn g_FDIDestroy = NULL;
FDICopyFn g_FDICopy = NULL;

bool InitializeFdi() {
  if (!g_fdi) {
    // It has been observed that some users do not have the expected
    // environment variables set, so we try a couple that *should* always be
    // present and fallback to the default Windows install path if all else
    // fails.
    // The cabinet.dll should be available on all supported versions of Windows.
    static const wchar_t* const candidate_paths[] = {
      L"%WINDIR%\\system32\\cabinet.dll",
      L"%SYSTEMROOT%\\system32\\cabinet.dll",
      L"C:\\Windows\\system32\\cabinet.dll",
    };

    wchar_t path[MAX_PATH] = {0};
    for (size_t i = 0; i < _countof(candidate_paths); ++i) {
      path[0] = L'\0';
      DWORD result = ::ExpandEnvironmentStringsW(candidate_paths[i],
                                                 path, _countof(path));

      if (result > 0 && result <= _countof(path))
        g_fdi = ::LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

      if (g_fdi)
        break;
    }
  }

  if (g_fdi) {
    g_FDICreate =
        reinterpret_cast<FDICreateFn>(::GetProcAddress(g_fdi, "FDICreate"));
    g_FDIDestroy =
        reinterpret_cast<FDIDestroyFn>(::GetProcAddress(g_fdi, "FDIDestroy"));
    g_FDICopy =
        reinterpret_cast<FDICopyFn>(::GetProcAddress(g_fdi, "FDICopy"));
  }

  return g_FDICreate && g_FDIDestroy && g_FDICopy;
}

}  // namespace

namespace mini_installer {

bool Expand(const wchar_t* source, const wchar_t* destination) {
  if (!InitializeFdi())
    return false;

  // Start by splitting up the source path and convert to utf8 since the
  // cabinet API doesn't support wide strings.
  const wchar_t* source_name = source + lstrlenW(source);
  while (source_name > source && *source_name != L'\\')
    --source_name;
  if (source_name == source)
    return false;

  // Convert the name to utf8.
  source_name++;
  scoped_ptr<char> source_name_utf8(WideToUtf8(source_name, -1));
  // The directory part is assumed to have a trailing backslash.
  scoped_ptr<char> source_path_utf8(WideToUtf8(source, source_name - source));

  scoped_ptr<char> dest_utf8(WideToUtf8(destination, -1));
  if (!dest_utf8 || !source_name_utf8 || !source_path_utf8)
    return false;

  bool success = false;

  ERF erf = {0};
  HFDI fdi = g_FDICreate(&Alloc, &Free, &Open, &Read, &Write, &Close, &Seek,
                         cpuUNKNOWN, &erf);
  if (fdi) {
    if (g_FDICopy(fdi, source_name_utf8, source_path_utf8, 0,
                  &Notify, NULL, const_cast<wchar_t*>(destination))) {
      success = true;
    }
    g_FDIDestroy(fdi);
  }

  return success;
}

}  // namespace mini_installer
