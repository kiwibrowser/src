// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_C_PRIVATE_PPB_FLASH_FILE_H_
#define PPAPI_C_PRIVATE_PPB_FLASH_FILE_H_

#include <stdint.h>

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/private/pp_file_handle.h"

struct PP_FileInfo;

struct PP_DirEntry_Dev {
  const char* name;
  PP_Bool is_dir;
};

struct PP_DirContents_Dev {
  int32_t count;
  struct PP_DirEntry_Dev* entries;
};

// PPB_Flash_File_ModuleLocal --------------------------------------------------
#define PPB_FLASH_FILE_MODULELOCAL_INTERFACE_3_0 "PPB_Flash_File_ModuleLocal;3"
#define PPB_FLASH_FILE_MODULELOCAL_INTERFACE \
    PPB_FLASH_FILE_MODULELOCAL_INTERFACE_3_0

// This interface provides (for Flash) synchronous access to module-local files.
// Module-local file paths are '/'-separated UTF-8 strings, relative to a
// module-specific root.
struct PPB_Flash_File_ModuleLocal_3_0 {
  // Deprecated. Returns true.
  bool (*CreateThreadAdapterForInstance)(PP_Instance instance);
  // Deprecated. Does nothing.
  void (*ClearThreadAdapterForInstance)(PP_Instance instance);

  // Opens a file, returning a file descriptor (posix) or a HANDLE (win32) into
  // file. The return value is the ppapi error, PP_OK if success, one of the
  // PP_ERROR_* in case of failure.
  int32_t (*OpenFile)(PP_Instance instance,
                      const char* path,
                      int32_t mode,
                      PP_FileHandle* file);

  // Renames a file. The return value is the ppapi error, PP_OK if success, one
  // of the PP_ERROR_* in case of failure.
  int32_t (*RenameFile)(PP_Instance instance,
                        const char* path_from,
                        const char* path_to);

  // Deletes a file or directory. If recursive is set and the path points to a
  // directory, deletes all the contents of the directory. The return value is
  // the ppapi error, PP_OK if success, one of the PP_ERROR_* in case of
  // failure.
  int32_t (*DeleteFileOrDir)(PP_Instance instance,
                             const char* path,
                             PP_Bool recursive);

  // Creates a directory. The return value is the ppapi error, PP_OK if success,
  // one of the PP_ERROR_* in case of failure.
  int32_t (*CreateDir)(PP_Instance instance, const char* path);

  // Queries information about a file. The return value is the ppapi error,
  // PP_OK if success, one of the PP_ERROR_* in case of failure.
  int32_t (*QueryFile)(PP_Instance instance,
                       const char* path,
                       struct PP_FileInfo* info);

  // Gets the list of files contained in a directory. The return value is the
  // ppapi error, PP_OK if success, one of the PP_ERROR_* in case of failure. If
  // non-NULL, the returned contents should be freed with FreeDirContents.
  int32_t (*GetDirContents)(PP_Instance instance,
                            const char* path,
                            struct PP_DirContents_Dev** contents);

  // Frees the data allocated by GetDirContents.
  void (*FreeDirContents)(PP_Instance instance,
                          struct PP_DirContents_Dev* contents);

  // Creates a temporary file. The file will be automatically deleted when all
  // handles to it are closed.
  // Returns PP_OK if successful, one of the PP_ERROR_* values in case of
  // failure.
  //
  // If successful, |file| is set to a file descriptor (posix) or a HANDLE
  // (win32) to the file. If failed, |file| is not touched.
  int32_t (*CreateTemporaryFile)(PP_Instance instance, PP_FileHandle* file);
};

typedef struct PPB_Flash_File_ModuleLocal_3_0 PPB_Flash_File_ModuleLocal;

// PPB_Flash_File_FileRef ------------------------------------------------------

#define PPB_FLASH_FILE_FILEREF_INTERFACE "PPB_Flash_File_FileRef;2"

// This interface provides (for Flash) synchronous access to files whose paths
// are given by a Pepper FileRef. Such FileRefs are typically obtained via the
// Pepper file chooser.
struct PPB_Flash_File_FileRef {
  // The functions below correspond exactly to their module-local counterparts
  // (except in taking FileRefs instead of paths, of course). We omit the
  // functionality which we do not provide for FileRefs.
  int32_t (*OpenFile)(PP_Resource file_ref_id,
                      int32_t mode,
                      PP_FileHandle* file);
  int32_t (*QueryFile)(PP_Resource file_ref_id,
                       struct PP_FileInfo* info);
};

#endif  // PPAPI_C_PRIVATE_PPB_FLASH_FILE_H_
