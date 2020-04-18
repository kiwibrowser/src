// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/vfs_wrapper.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/debug/leak_annotations.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#endif

namespace sql {
namespace {

// https://www.sqlite.org/vfs.html - documents the overall VFS system.
//
// https://www.sqlite.org/c3ref/vfs.html - VFS methods.  This code tucks the
// wrapped VFS pointer into the wrapper's pAppData pointer.
//
// https://www.sqlite.org/c3ref/file.html - instance of an open file.  This code
// allocates a VfsFile for this, which contains a pointer to the wrapped file.
// Idiomatic SQLite would take the wrapped VFS szOsFile and increase it to store
// additional data as a prefix.

sqlite3_vfs* GetWrappedVfs(sqlite3_vfs* wrapped_vfs) {
  return static_cast<sqlite3_vfs*>(wrapped_vfs->pAppData);
}

// NOTE(shess): This structure is allocated by SQLite using malloc.  Do not add
// C++ objects, they will not be correctly constructed and destructed.  Instead,
// manually manage a pointer to a C++ object in Open() and Close().
struct VfsFile {
  const sqlite3_io_methods* methods;
  sqlite3_file* wrapped_file;
};

VfsFile* AsVfsFile(sqlite3_file* wrapper_file) {
  return reinterpret_cast<VfsFile*>(wrapper_file);
}

sqlite3_file* GetWrappedFile(sqlite3_file* wrapper_file) {
  return AsVfsFile(wrapper_file)->wrapped_file;
}

int Close(sqlite3_file* sqlite_file)
{
  VfsFile* file = AsVfsFile(sqlite_file);

  int r = file->wrapped_file->pMethods->xClose(file->wrapped_file);
  sqlite3_free(file->wrapped_file);
  memset(file, '\0', sizeof(*file));
  return r;
}

int Read(sqlite3_file* sqlite_file, void* buf, int amt, sqlite3_int64 ofs)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xRead(wrapped_file, buf, amt, ofs);
}

int Write(sqlite3_file* sqlite_file, const void* buf, int amt,
          sqlite3_int64 ofs)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xWrite(wrapped_file, buf, amt, ofs);
}

int Truncate(sqlite3_file* sqlite_file, sqlite3_int64 size)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xTruncate(wrapped_file, size);
}

int Sync(sqlite3_file* sqlite_file, int flags)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xSync(wrapped_file, flags);
}

int FileSize(sqlite3_file* sqlite_file, sqlite3_int64* size)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xFileSize(wrapped_file, size);
}

int Lock(sqlite3_file* sqlite_file, int file_lock)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xLock(wrapped_file, file_lock);
}

int Unlock(sqlite3_file* sqlite_file, int file_lock)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xUnlock(wrapped_file, file_lock);
}

int CheckReservedLock(sqlite3_file* sqlite_file, int* result)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xCheckReservedLock(wrapped_file, result);
}

int FileControl(sqlite3_file* sqlite_file, int op, void* arg)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xFileControl(wrapped_file, op, arg);
}

int SectorSize(sqlite3_file* sqlite_file)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xSectorSize(wrapped_file);
}

int DeviceCharacteristics(sqlite3_file* sqlite_file)
{
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xDeviceCharacteristics(wrapped_file);
}

int ShmMap(sqlite3_file *sqlite_file, int region, int size,
           int extend, void volatile **pp) {
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xShmMap(
      wrapped_file, region, size, extend, pp);
}

int ShmLock(sqlite3_file *sqlite_file, int ofst, int n, int flags) {
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xShmLock(wrapped_file, ofst, n, flags);
}

void ShmBarrier(sqlite3_file *sqlite_file) {
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  wrapped_file->pMethods->xShmBarrier(wrapped_file);
}

int ShmUnmap(sqlite3_file *sqlite_file, int del) {
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xShmUnmap(wrapped_file, del);
}

int Fetch(sqlite3_file *sqlite_file, sqlite3_int64 off, int amt, void **pp) {
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xFetch(wrapped_file, off, amt, pp);
}

int Unfetch(sqlite3_file *sqlite_file, sqlite3_int64 off, void *p) {
  sqlite3_file* wrapped_file = GetWrappedFile(sqlite_file);
  return wrapped_file->pMethods->xUnfetch(wrapped_file, off, p);
}

#if defined(OS_MACOSX) && !defined(OS_IOS)
// Helper to convert a POSIX path into a CoreFoundation path.
base::ScopedCFTypeRef<CFURLRef> CFURLRefForPath(const char* path){
  base::ScopedCFTypeRef<CFStringRef> urlString(
      CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, path));
  base::ScopedCFTypeRef<CFURLRef> url(
      CFURLCreateWithFileSystemPath(kCFAllocatorDefault, urlString,
                                    kCFURLPOSIXPathStyle, FALSE));
  return url;
}
#endif

int Open(sqlite3_vfs* vfs, const char* file_name, sqlite3_file* wrapper_file,
         int desired_flags, int* used_flags) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);

  sqlite3_file* wrapped_file = static_cast<sqlite3_file*>(
      sqlite3_malloc(wrapped_vfs->szOsFile));
  if (!wrapped_file)
    return SQLITE_NOMEM;

  // NOTE(shess): SQLite's unixOpen() makes assumptions about the structure of
  // |file_name|.  Do not pass a local copy, here, only the passed-in value.
  int rc = wrapped_vfs->xOpen(wrapped_vfs,
                              file_name, wrapped_file,
                              desired_flags, used_flags);
  if (rc != SQLITE_OK) {
    sqlite3_free(wrapped_file);
    return rc;
  }
  // NOTE(shess): Any early exit from here needs to call xClose() on
  // |wrapped_file|.

#if defined(OS_MACOSX) && !defined(OS_IOS)
  // When opening journal files, propagate time-machine exclusion from db.
  static int kJournalFlags =
      SQLITE_OPEN_MAIN_JOURNAL | SQLITE_OPEN_TEMP_JOURNAL |
      SQLITE_OPEN_SUBJOURNAL | SQLITE_OPEN_MASTER_JOURNAL;
  if (file_name && (desired_flags & kJournalFlags)) {
    // https://www.sqlite.org/c3ref/vfs.html indicates that the journal path
    // will have a "-suffix".
    size_t dash_index = base::StringPiece(file_name).rfind('-');
    if (dash_index != base::StringPiece::npos) {
      std::string db_name(file_name, dash_index);
      base::ScopedCFTypeRef<CFURLRef> db_url(CFURLRefForPath(db_name.c_str()));
      if (CSBackupIsItemExcluded(db_url, nullptr)) {
        base::ScopedCFTypeRef<CFURLRef> journal_url(CFURLRefForPath(file_name));
        CSBackupSetItemExcluded(journal_url, TRUE, FALSE);
      }
    }
  }
#endif

  // |iVersion| determines what methods SQLite may call on the instance.
  // Having the methods which can't be proxied return an error may cause SQLite
  // to operate differently than if it didn't call those methods at all. To be
  // on the safe side, the wrapper sqlite3_io_methods version perfectly matches
  // the version of the wrapped files.
  //
  // At a first glance, it might be tempting to simplify the code by
  // restricting wrapping support to VFS version 3. However, this would fail
  // on Fuchsia and might fail on Mac.
  //
  // On Mac, SQLite built with SQLITE_ENABLE_LOCKING_STYLE ends up using a VFS
  // that dynamically dispatches between a few variants of sqlite3_io_methods,
  // based on whether the opened database is on a local or on a remote (AFS,
  // NFS) filesystem. Some variants return a VFS version 1 structure.
  //
  // Fuchsia doesn't implement POSIX locking, so it always uses dot-style
  // locking, which returns VFS version 1 files.
  VfsFile* file = AsVfsFile(wrapper_file);
  file->wrapped_file = wrapped_file;
  if (wrapped_file->pMethods->iVersion == 1) {
    static const sqlite3_io_methods io_methods = {
      1,
      Close,
      Read,
      Write,
      Truncate,
      Sync,
      FileSize,
      Lock,
      Unlock,
      CheckReservedLock,
      FileControl,
      SectorSize,
      DeviceCharacteristics,
    };
    file->methods = &io_methods;
  } else if (wrapped_file->pMethods->iVersion == 2) {
    static const sqlite3_io_methods io_methods = {
      2,
      Close,
      Read,
      Write,
      Truncate,
      Sync,
      FileSize,
      Lock,
      Unlock,
      CheckReservedLock,
      FileControl,
      SectorSize,
      DeviceCharacteristics,
      // Methods above are valid for version 1.
      ShmMap,
      ShmLock,
      ShmBarrier,
      ShmUnmap,
    };
    file->methods = &io_methods;
  } else {
    static const sqlite3_io_methods io_methods = {
      3,
      Close,
      Read,
      Write,
      Truncate,
      Sync,
      FileSize,
      Lock,
      Unlock,
      CheckReservedLock,
      FileControl,
      SectorSize,
      DeviceCharacteristics,
      // Methods above are valid for version 1.
      ShmMap,
      ShmLock,
      ShmBarrier,
      ShmUnmap,
      // Methods above are valid for version 2.
      Fetch,
      Unfetch,
    };
    file->methods = &io_methods;
  }
  return SQLITE_OK;
}

int Delete(sqlite3_vfs* vfs, const char* file_name, int sync_dir) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xDelete(wrapped_vfs, file_name, sync_dir);
}

int Access(sqlite3_vfs* vfs, const char* file_name, int flag, int* res) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xAccess(wrapped_vfs, file_name, flag, res);
}

int FullPathname(sqlite3_vfs* vfs, const char* relative_path,
                 int buf_size, char* absolute_path) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xFullPathname(
      wrapped_vfs, relative_path, buf_size, absolute_path);
}

void* DlOpen(sqlite3_vfs* vfs, const char* filename) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xDlOpen(wrapped_vfs, filename);
}

void DlError(sqlite3_vfs* vfs, int buf_size, char* error_buffer) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  wrapped_vfs->xDlError(wrapped_vfs, buf_size, error_buffer);
}

void(*DlSym(sqlite3_vfs* vfs, void* handle, const char* sym))(void) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xDlSym(wrapped_vfs, handle, sym);
}

void DlClose(sqlite3_vfs* vfs, void* handle) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  wrapped_vfs->xDlClose(wrapped_vfs, handle);
}

int Randomness(sqlite3_vfs* vfs, int buf_size, char* buffer) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xRandomness(wrapped_vfs, buf_size, buffer);
}

int Sleep(sqlite3_vfs* vfs, int microseconds) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xSleep(wrapped_vfs, microseconds);
}

int CurrentTime(sqlite3_vfs* vfs, double* now) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xCurrentTime(wrapped_vfs, now);
}

int GetLastError(sqlite3_vfs* vfs, int e, char* s) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xGetLastError(wrapped_vfs, e, s);
}

int CurrentTimeInt64(sqlite3_vfs* vfs, sqlite3_int64* now) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xCurrentTimeInt64(wrapped_vfs, now);
}

int SetSystemCall(sqlite3_vfs* vfs, const char* name,
                  sqlite3_syscall_ptr func) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xSetSystemCall(wrapped_vfs, name, func);
}

sqlite3_syscall_ptr GetSystemCall(sqlite3_vfs* vfs, const char* name) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xGetSystemCall(wrapped_vfs, name);
}

const char* NextSystemCall(sqlite3_vfs* vfs, const char* name) {
  sqlite3_vfs* wrapped_vfs = GetWrappedVfs(vfs);
  return wrapped_vfs->xNextSystemCall(wrapped_vfs, name);
}

}  // namespace

sqlite3_vfs* VFSWrapper() {
  const char* kVFSName = "VFSWrapper";

  // Return existing version if already registered.
  {
    sqlite3_vfs* vfs = sqlite3_vfs_find(kVFSName);
    if (vfs != nullptr)
      return vfs;
  }

  // Get the default VFS for this platform.  If no default VFS, give up.
  sqlite3_vfs* wrapped_vfs = sqlite3_vfs_find(nullptr);
  if (!wrapped_vfs)
    return nullptr;

  std::unique_ptr<sqlite3_vfs, std::function<void(sqlite3_vfs*)>> wrapper_vfs(
      static_cast<sqlite3_vfs*>(sqlite3_malloc(sizeof(sqlite3_vfs))),
      [](sqlite3_vfs* v) {
        sqlite3_free(v);
      });
  memset(wrapper_vfs.get(), '\0', sizeof(sqlite3_vfs));

  // VFS implementations should always work with a SQLite that only knows about
  // earlier versions.
  wrapper_vfs->iVersion = std::min(wrapped_vfs->iVersion, 3);

  // Caller of xOpen() allocates this much space.
  wrapper_vfs->szOsFile = sizeof(VfsFile);

  wrapper_vfs->mxPathname = wrapped_vfs->mxPathname;
  wrapper_vfs->pNext = nullptr;
  wrapper_vfs->zName = kVFSName;

  // Keep a reference to the wrapped vfs for use in methods.
  wrapper_vfs->pAppData = wrapped_vfs;

  // VFS methods.
  wrapper_vfs->xOpen = &Open;
  wrapper_vfs->xDelete = &Delete;
  wrapper_vfs->xAccess = &Access;
  wrapper_vfs->xFullPathname = &FullPathname;
  wrapper_vfs->xDlOpen = &DlOpen;
  wrapper_vfs->xDlError = &DlError;
  wrapper_vfs->xDlSym = &DlSym;
  wrapper_vfs->xDlClose = &DlClose;
  wrapper_vfs->xRandomness = &Randomness;
  wrapper_vfs->xSleep = &Sleep;
  // |xCurrentTime| is null when SQLite is built with SQLITE_OMIT_DEPRECATED.
  wrapper_vfs->xCurrentTime =
      (wrapped_vfs->xCurrentTime ? &CurrentTime : nullptr);
  wrapper_vfs->xGetLastError = &GetLastError;
  // The methods above are in version 1 of sqlite_vfs.
  DCHECK(wrapped_vfs->xCurrentTimeInt64);
  wrapper_vfs->xCurrentTimeInt64 = &CurrentTimeInt64;
  // The methods above are in version 2 of sqlite_vfs.
  DCHECK(wrapped_vfs->xSetSystemCall);
  wrapper_vfs->xSetSystemCall = &SetSystemCall;
  DCHECK(wrapped_vfs->xGetSystemCall);
  wrapper_vfs->xGetSystemCall = &GetSystemCall;
  DCHECK(wrapped_vfs->xNextSystemCall);
  wrapper_vfs->xNextSystemCall = &NextSystemCall;
  // The methods above are in version 3 of sqlite_vfs.

  if (SQLITE_OK == sqlite3_vfs_register(wrapper_vfs.get(), 0)) {
    ANNOTATE_LEAKING_OBJECT_PTR(wrapper_vfs.get());
    wrapper_vfs.release();
  }

  return sqlite3_vfs_find(kVFSName);
}

}  // namespace sql
