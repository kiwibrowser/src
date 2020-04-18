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

#include "third_party/blink/public/platform/platform.h"
#include "third_party/sqlite/sqlite3.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

namespace blink {

// Chromium's Posix implementation of SQLite VFS
namespace {

struct chromiumVfsFile {
  sqlite3_io_methods* p_methods;
  sqlite3_file* wrapped_file;
  char* wrapped_file_name;
};

int ChromiumClose(sqlite3_file* sqlite_file) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  int r = chromium_file->wrapped_file->pMethods->xClose(
      chromium_file->wrapped_file);
  sqlite3_free(chromium_file->wrapped_file_name);
  sqlite3_free(chromium_file->wrapped_file);
  memset(chromium_file, 0, sizeof(*chromium_file));
  return r;
}

int ChromiumRead(sqlite3_file* sqlite_file,
                 void* p_buf,
                 int i_amt,
                 sqlite3_int64 i_ofst) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xRead(
      chromium_file->wrapped_file, p_buf, i_amt, i_ofst);
}

int ChromiumWrite(sqlite3_file* sqlite_file,
                  const void* p_buf,
                  int i_amt,
                  sqlite3_int64 i_ofst) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xWrite(
      chromium_file->wrapped_file, p_buf, i_amt, i_ofst);
}

int ChromiumTruncate(sqlite3_file* sqlite_file, sqlite3_int64 size) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);

  // The OSX and Linux sandboxes block ftruncate(), proxy to the browser
  // process.
  if (Platform::Current()->DatabaseSetFileSize(
          String::FromUTF8(chromium_file->wrapped_file_name), size))
    return SQLITE_OK;
  return SQLITE_IOERR_TRUNCATE;
}

int ChromiumSync(sqlite3_file* sqlite_file, int flags) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xSync(
      chromium_file->wrapped_file, flags);
}

int ChromiumFileSize(sqlite3_file* sqlite_file, sqlite3_int64* p_size) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xFileSize(
      chromium_file->wrapped_file, p_size);
}

int ChromiumLock(sqlite3_file* sqlite_file, int e_file_lock) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xLock(
      chromium_file->wrapped_file, e_file_lock);
}

int ChromiumUnlock(sqlite3_file* sqlite_file, int e_file_lock) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xUnlock(
      chromium_file->wrapped_file, e_file_lock);
}

int ChromiumCheckReservedLock(sqlite3_file* sqlite_file, int* p_res_out) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xCheckReservedLock(
      chromium_file->wrapped_file, p_res_out);
}

int ChromiumFileControl(sqlite3_file* sqlite_file, int op, void* p_arg) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xFileControl(
      chromium_file->wrapped_file, op, p_arg);
}

int ChromiumSectorSize(sqlite3_file* sqlite_file) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xSectorSize(
      chromium_file->wrapped_file);
}

int ChromiumDeviceCharacteristics(sqlite3_file* sqlite_file) {
  chromiumVfsFile* chromium_file =
      reinterpret_cast<chromiumVfsFile*>(sqlite_file);
  return chromium_file->wrapped_file->pMethods->xDeviceCharacteristics(
      chromium_file->wrapped_file);
}

// Opens a file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// id - the structure that will manipulate the newly opened file.
// desiredFlags - the desired open mode flags.
// usedFlags - the actual open mode flags that were used.
int ChromiumOpenInternal(sqlite3_vfs* vfs,
                         const char* file_name,
                         sqlite3_file* id,
                         int desired_flags,
                         int* used_flags) {
  int fd = Platform::Current()->DatabaseOpenFile(String::FromUTF8(file_name),
                                                 desired_flags);
  if ((fd < 0) && (desired_flags & SQLITE_OPEN_READWRITE)) {
    desired_flags =
        (desired_flags & ~(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)) |
        SQLITE_OPEN_READONLY;
    fd = Platform::Current()->DatabaseOpenFile(String::FromUTF8(file_name),
                                               desired_flags);
  }
  if (fd < 0)
    return SQLITE_CANTOPEN;

  if (used_flags)
    *used_flags = desired_flags;

  fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);

  // The mask 0x00007F00 gives us the 7 bits that determine the type of the file
  // SQLite is trying to open.
  int file_type = desired_flags & 0x00007F00;
  int no_lock = (file_type != SQLITE_OPEN_MAIN_DB);
  sqlite3_vfs* wrapped_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return chromium_sqlite3_fill_in_unix_sqlite3_file(
      wrapped_vfs, fd, id, file_name, no_lock, desired_flags);
}

int ChromiumOpen(sqlite3_vfs* vfs,
                 const char* file_name,
                 sqlite3_file* id,
                 int desired_flags,
                 int* used_flags) {
  sqlite3_vfs* wrapped_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  sqlite3_file* wrapped_file =
      static_cast<sqlite3_file*>(sqlite3_malloc(wrapped_vfs->szOsFile));
  if (!wrapped_file)
    return SQLITE_NOMEM;

  // Make a local copy of the file name.  SQLite's os_unix.c appears to be
  // written to allow caching the pointer passed in to this function, but that
  // seems brittle.
  char* wrapped_file_name = sqlite3_mprintf("%s", file_name);
  if (!wrapped_file_name) {
    sqlite3_free(wrapped_file);
    return SQLITE_NOMEM;
  }

  // SQLite's unixOpen() makes assumptions about the structure of |fileName|.
  // Our local copy may not answer those assumptions correctly.
  int rc = ChromiumOpenInternal(vfs, file_name, wrapped_file, desired_flags,
                                used_flags);
  if (rc != SQLITE_OK) {
    sqlite3_free(wrapped_file_name);
    sqlite3_free(wrapped_file);
    return rc;
  }

  static sqlite3_io_methods chromium_io_methods = {
      1,
      ChromiumClose,
      ChromiumRead,
      ChromiumWrite,
      ChromiumTruncate,
      ChromiumSync,
      ChromiumFileSize,
      ChromiumLock,
      ChromiumUnlock,
      ChromiumCheckReservedLock,
      ChromiumFileControl,
      ChromiumSectorSize,
      ChromiumDeviceCharacteristics,
      // Methods above are valid for version 1.
  };
  chromiumVfsFile* chromium_file = reinterpret_cast<chromiumVfsFile*>(id);
  chromium_file->p_methods = &chromium_io_methods;
  chromium_file->wrapped_file = wrapped_file;
  chromium_file->wrapped_file_name = wrapped_file_name;
  return SQLITE_OK;
}

// Deletes the given file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// syncDir - determines if the directory to which this file belongs
//           should be synched after the file is deleted.
int ChromiumDelete(sqlite3_vfs*, const char* file_name, int sync_dir) {
  return Platform::Current()->DatabaseDeleteFile(String::FromUTF8(file_name),
                                                 sync_dir);
}

// Check the existance and status of the given file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// flag - the type of test to make on this file.
// res - the result.
int ChromiumAccess(sqlite3_vfs*, const char* file_name, int flag, int* res) {
  int attr = static_cast<int>(Platform::Current()->DatabaseGetFileAttributes(
      String::FromUTF8(file_name)));
  if (attr < 0) {
    *res = 0;
    return SQLITE_OK;
  }

  switch (flag) {
    case SQLITE_ACCESS_EXISTS:
      *res = 1;  // if the file doesn't exist, attr < 0
      break;
    case SQLITE_ACCESS_READWRITE:
      *res = (attr & W_OK) && (attr & R_OK);
      break;
    case SQLITE_ACCESS_READ:
      *res = (attr & R_OK);
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
  return nullptr;
}

void ChromiumDlError(sqlite3_vfs*, int buf_size, char* error_buffer) {
  sqlite3_snprintf(buf_size, error_buffer, "Dynamic loading not supported");
}

void (*ChromiumDlSym(sqlite3_vfs*, void*, const char*))(void) {
  return nullptr;
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
  sqlite3_vfs* wrapped_vfs = sqlite3_vfs_find("unix");

  // These are implemented by delegating to |wrappedVfs|.
  // TODO(shess): Implement local versions.
  DCHECK(wrapped_vfs->xRandomness);
  DCHECK(wrapped_vfs->xSleep);
  DCHECK(wrapped_vfs->xCurrentTimeInt64);

  static sqlite3_vfs chromium_vfs = {2,  // SQLite VFS API version.
                                     sizeof(chromiumVfsFile),
                                     wrapped_vfs->mxPathname,
                                     nullptr,
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
