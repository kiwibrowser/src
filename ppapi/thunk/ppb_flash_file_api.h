// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_FLASH_FILE_API_H_
#define PPAPI_THUNK_PPB_FLASH_FILE_API_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/private/pp_file_handle.h"
#include "ppapi/c/private/ppb_flash_file.h"
#include "ppapi/shared_impl/singleton_resource_id.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

struct PP_FileInfo;

namespace ppapi {
namespace thunk {

class PPAPI_THUNK_EXPORT PPB_Flash_File_API {
 public:
  virtual ~PPB_Flash_File_API() {}

  // FlashFile_ModuleLocal.
  virtual int32_t OpenFile(PP_Instance instance,
                           const char* path,
                           int32_t mode,
                           PP_FileHandle* file) = 0;
  virtual int32_t RenameFile(PP_Instance instance,
                             const char* path_from,
                             const char* path_to) = 0;
  virtual int32_t DeleteFileOrDir(PP_Instance instance,
                                  const char* path,
                                  PP_Bool recursive) = 0;
  virtual int32_t CreateDir(PP_Instance instance, const char* path) = 0;
  virtual int32_t QueryFile(PP_Instance instance,
                            const char* path,
                            PP_FileInfo* info) = 0;
  virtual int32_t GetDirContents(PP_Instance instance,
                                 const char* path,
                                 PP_DirContents_Dev** contents) = 0;
  virtual void FreeDirContents(PP_Instance instance,
                               PP_DirContents_Dev* contents) = 0;
  virtual int32_t CreateTemporaryFile(PP_Instance instance,
                                      PP_FileHandle* file) = 0;

  // FlashFile_FileRef.
  virtual int32_t OpenFileRef(PP_Instance instance,
                              PP_Resource file_ref,
                              int32_t mode,
                              PP_FileHandle* file) = 0;
  virtual int32_t QueryFileRef(PP_Instance instance,
                               PP_Resource file_ref,
                               PP_FileInfo* info) = 0;

  static const SingletonResourceID kSingletonResourceID =
      FLASH_FILE_SINGLETON_ID;
};

}  // namespace thunk
}  // namespace ppapi

#endif // PPAPI_THUNK_PPB_FLASH_FILE_API_H_
