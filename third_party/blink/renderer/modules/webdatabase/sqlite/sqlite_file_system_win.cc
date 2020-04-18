/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/webdatabase/sqlite/sqlite_file_system.h"

#include <windows.h>
#include "third_party/blink/public/platform/platform.h"
#include "third_party/sqlite/sqlite3.h"

namespace blink {

// Chromium's Windows implementation of SQLite VFS
namespace {

// Opens a file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// id - the structure that will manipulate the newly opened file.
// desiredFlags - the desired open mode flags.
// usedFlags - the actual open mode flags that were used.
int ChromiumOpen(sqlite3_vfs*,
                 const char* file_name,
                 sqlite3_file* id,
                 int desired_flags,
                 int* used_flags) {
  HANDLE h = Platform::Current()->DatabaseOpenFile(String::FromUTF8(file_name),
                                                   desired_flags);
  if (h == INVALID_HANDLE_VALUE) {
    if (desired_flags & SQLITE_OPEN_READWRITE) {
      int new_flags =
          (desired_flags | SQLITE_OPEN_READONLY) & ~SQLITE_OPEN_READWRITE;
      return ChromiumOpen(0, file_name, id, new_flags, used_flags);
    } else
      return SQLITE_CANTOPEN;
  }
  if (used_flags) {
    if (desired_flags & SQLITE_OPEN_READWRITE)
      *used_flags = SQLITE_OPEN_READWRITE;
    else
      *used_flags = SQLITE_OPEN_READONLY;
  }

  chromium_sqlite3_initialize_win_sqlite3_file(id, h);
  return SQLITE_OK;
}

// Deletes the given file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// syncDir - determines if the directory to which this file belongs
//           should be synched after the file is deleted.
int ChromiumDelete(sqlite3_vfs*, const char* file_name, int) {
  return Platform::Current()->DatabaseDeleteFile(String::FromUTF8(file_name),
                                                 false);
}

// Check the existance and status of the given file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// flag - the type of test to make on this file.
// res - the result.
int ChromiumAccess(sqlite3_vfs*, const char* file_name, int flag, int* res) {
  DWORD attr = Platform::Current()->DatabaseGetFileAttributes(
      String::FromUTF8(file_name));
  switch (flag) {
    case SQLITE_ACCESS_READ:
    case SQLITE_ACCESS_EXISTS:
      *res = (attr != INVALID_FILE_ATTRIBUTES);
      break;
    case SQLITE_ACCESS_READWRITE:
      *res = ((attr & FILE_ATTRIBUTE_READONLY) == 0);
      break;
    default:
      return SQLITE_ERROR;
  }

  return SQLITE_OK;
}

// Turns a relative pathname into a full pathname.
//
// vfs - pointer to the sqlite3_vfs object.
// relativePath - the relative path.
// bufSize - the size of the output buffer in bytes.
// absolutePath - the output buffer where the absolute path will be stored.
int ChromiumFullPathname(sqlite3_vfs* vfs,
                         const char* relative_path,
                         int buf_size,
                         char* absolute_path) {
  // The renderer process doesn't need to know the absolute path of the file
  sqlite3_snprintf(buf_size, absolute_path, "%s", relative_path);
  return SQLITE_OK;
}

// Do not allow loading libraries in the renderer.
void* ChromiumDlOpen(sqlite3_vfs*, const char*) {
  return 0;
}

void ChromiumDlError(sqlite3_vfs*, int buf_size, char* error_buffer) {
  sqlite3_snprintf(buf_size, error_buffer, "Dynamic loading not supported");
}

void (*ChromiumDlSym(sqlite3_vfs*, void*, const char*))(void) {
  return 0;
}

void ChromiumDlClose(sqlite3_vfs*, void*) {}

int ChromiumRandomness(sqlite3_vfs* vfs, int buf_size, char* buffer) {
  sqlite3_vfs* wrapped_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return wrapped_vfs->xRandomness(wrapped_vfs, buf_size, buffer);
}

int ChromiumSleep(sqlite3_vfs* vfs, int microseconds) {
  sqlite3_vfs* wrapped_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return wrapped_vfs->xSleep(wrapped_vfs, microseconds);
}

int ChromiumGetLastError(sqlite3_vfs* vfs, int n_buf, char* z_buf) {
  if (n_buf && z_buf)
    *z_buf = '\0';
  return 0;
}

int ChromiumCurrentTimeInt64(sqlite3_vfs* vfs, sqlite3_int64* now) {
  sqlite3_vfs* wrapped_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return wrapped_vfs->xCurrentTimeInt64(wrapped_vfs, now);
}

}  // namespace

void SQLiteFileSystem::RegisterSQLiteVFS() {
  sqlite3_vfs* wrapped_vfs = sqlite3_vfs_find("win32");

  // These are implemented by delegating to |wrappedVfs|.
  // TODO(shess): Implement local versions.
  DCHECK(wrapped_vfs->xRandomness);
  DCHECK(wrapped_vfs->xSleep);
  DCHECK(wrapped_vfs->xCurrentTimeInt64);

  static sqlite3_vfs chromium_vfs = {2,  // SQLite VFS API version.
                                     wrapped_vfs->szOsFile,
                                     wrapped_vfs->mxPathname,
                                     0,
                                     "chromium_vfs",
                                     wrapped_vfs,
                                     ChromiumOpen,
                                     ChromiumDelete,
                                     ChromiumAccess,
                                     ChromiumFullPathname,
                                     ChromiumDlOpen,
                                     ChromiumDlError,
                                     ChromiumDlSym,
                                     ChromiumDlClose,
                                     ChromiumRandomness,
                                     ChromiumSleep,
                                     nullptr,  // CurrentTime is deprecated.
                                     ChromiumGetLastError,
                                     ChromiumCurrentTimeInt64};
  sqlite3_vfs_register(&chromium_vfs, 0);
}

}  // namespace blink
