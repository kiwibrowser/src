// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_FLASH_FILE_RESOURCE_H_
#define PPAPI_PROXY_FLASH_FILE_RESOURCE_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/shared_impl/file_path.h"
#include "ppapi/thunk/ppb_flash_file_api.h"

namespace ppapi {
namespace proxy {

class FlashFileResource
    : public PluginResource,
      public thunk::PPB_Flash_File_API {
 public:
  FlashFileResource(Connection connection, PP_Instance instance);
  ~FlashFileResource() override;

  // Resource overrides.
  thunk::PPB_Flash_File_API* AsPPB_Flash_File_API() override;

  // PPB_Flash_Functions_API.
  int32_t OpenFile(PP_Instance instance,
                   const char* path,
                   int32_t mode,
                   PP_FileHandle* file) override;
  int32_t RenameFile(PP_Instance instance,
                     const char* path_from,
                     const char* path_to) override;
  int32_t DeleteFileOrDir(PP_Instance instance,
                          const char* path,
                          PP_Bool recursive) override;
  int32_t CreateDir(PP_Instance instance, const char* path) override;
  int32_t QueryFile(PP_Instance instance,
                    const char* path,
                    PP_FileInfo* info) override;
  int32_t GetDirContents(PP_Instance instance,
                         const char* path,
                         PP_DirContents_Dev** contents) override;
  void FreeDirContents(PP_Instance instance,
                       PP_DirContents_Dev* contents) override;
  int32_t CreateTemporaryFile(PP_Instance instance,
                              PP_FileHandle* file) override;
  int32_t OpenFileRef(PP_Instance instance,
                      PP_Resource file_ref,
                      int32_t mode,
                      PP_FileHandle* file) override;
  int32_t QueryFileRef(PP_Instance instance,
                       PP_Resource file_ref,
                       PP_FileInfo* info) override;

 private:
  int32_t OpenFileHelper(const std::string& path,
                         PepperFilePath::Domain domain_type,
                         int32_t mode,
                         PP_FileHandle* file);
  int32_t QueryFileHelper(const std::string& path,
                          PepperFilePath::Domain domain_type,
                          PP_FileInfo* info);

  DISALLOW_COPY_AND_ASSIGN(FlashFileResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_FLASH_FILE_RESOURCE_H_
