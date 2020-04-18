// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/flash_file.h"

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/file_ref.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

// FileModuleLocal -------------------------------------------------------------

namespace {

template <> const char* interface_name<PPB_Flash_File_ModuleLocal_3_0>() {
  return PPB_FLASH_FILE_MODULELOCAL_INTERFACE_3_0;
}

}  // namespace

namespace flash {

static FileModuleLocal::DirEntry ConvertDirEntry(const PP_DirEntry_Dev& entry) {
  FileModuleLocal::DirEntry rv = { entry.name, PP_ToBool(entry.is_dir) };
  return rv;
}

// static
bool FileModuleLocal::IsAvailable() {
  return has_interface<PPB_Flash_File_ModuleLocal_3_0>();
}

// static
PP_FileHandle FileModuleLocal::OpenFile(const InstanceHandle& instance,
                                        const std::string& path,
                                        int32_t mode) {
  PP_FileHandle file_handle = PP_kInvalidFileHandle;
  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_ModuleLocal_3_0>()) {
    result = get_interface<PPB_Flash_File_ModuleLocal_3_0>()->
        OpenFile(instance.pp_instance(), path.c_str(), mode, &file_handle);
  }
  return (result == PP_OK) ? file_handle : PP_kInvalidFileHandle;
}

// static
bool FileModuleLocal::RenameFile(const InstanceHandle& instance,
                                 const std::string& path_from,
                                 const std::string& path_to) {
  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_ModuleLocal_3_0>()) {
    result = get_interface<PPB_Flash_File_ModuleLocal_3_0>()->
        RenameFile(instance.pp_instance(), path_from.c_str(), path_to.c_str());
  }
  return result == PP_OK;
}

// static
bool FileModuleLocal::DeleteFileOrDir(const InstanceHandle& instance,
                                      const std::string& path,
                                      bool recursive) {
  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_ModuleLocal_3_0>()) {
    result = get_interface<PPB_Flash_File_ModuleLocal_3_0>()->
        DeleteFileOrDir(instance.pp_instance(), path.c_str(),
                        PP_FromBool(recursive));
  }
  return result == PP_OK;
}

// static
bool FileModuleLocal::CreateDir(const InstanceHandle& instance,
                                const std::string& path) {
  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_ModuleLocal_3_0>()) {
    result = get_interface<PPB_Flash_File_ModuleLocal_3_0>()->
        CreateDir(instance.pp_instance(), path.c_str());
  }
  return result == PP_OK;
}

// static
bool FileModuleLocal::QueryFile(const InstanceHandle& instance,
                                const std::string& path,
                                PP_FileInfo* info) {
  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_ModuleLocal_3_0>()) {
    result = get_interface<PPB_Flash_File_ModuleLocal_3_0>()->
        QueryFile(instance.pp_instance(), path.c_str(), info);
  }
  return result == PP_OK;
}

// static
bool FileModuleLocal::GetDirContents(
    const InstanceHandle& instance,
    const std::string& path,
    std::vector<DirEntry>* dir_contents) {
  dir_contents->clear();

  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_ModuleLocal_3_0>()) {
    PP_DirContents_Dev* contents = NULL;
    result = get_interface<PPB_Flash_File_ModuleLocal_3_0>()->
        GetDirContents(instance.pp_instance(), path.c_str(), &contents);
    if (result == PP_OK && contents) {
      for (int32_t i = 0; i < contents->count; i++)
        dir_contents->push_back(ConvertDirEntry(contents->entries[i]));
    }
    if (contents) {
        get_interface<PPB_Flash_File_ModuleLocal_3_0>()->
            FreeDirContents(instance.pp_instance(), contents);
    }
  }
  return result == PP_OK;
}

// static
PP_FileHandle FileModuleLocal::CreateTemporaryFile(
    const InstanceHandle& instance) {
  PP_FileHandle file_handle = PP_kInvalidFileHandle;
  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_ModuleLocal_3_0>()) {
    result = get_interface<PPB_Flash_File_ModuleLocal_3_0>()->
        CreateTemporaryFile(instance.pp_instance(), &file_handle);
  }
  return (result == PP_OK) ? file_handle : PP_kInvalidFileHandle;
}

}  // namespace flash

// FileFileRef -----------------------------------------------------------------

namespace {

template <> const char* interface_name<PPB_Flash_File_FileRef>() {
  return PPB_FLASH_FILE_FILEREF_INTERFACE;
}

}  // namespace

namespace flash {

// static
bool FileFileRef::IsAvailable() {
  return has_interface<PPB_Flash_File_FileRef>();
}

// static
PP_FileHandle FileFileRef::OpenFile(const pp::FileRef& resource,
                                    int32_t mode) {
  PP_FileHandle file_handle = PP_kInvalidFileHandle;
  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_FileRef>()) {
    result = get_interface<PPB_Flash_File_FileRef>()->
        OpenFile(resource.pp_resource(), mode, &file_handle);
  }
  return (result == PP_OK) ? file_handle : PP_kInvalidFileHandle;
}

// static
bool FileFileRef::QueryFile(const pp::FileRef& resource,
                            PP_FileInfo* info) {
  int32_t result = PP_ERROR_FAILED;
  if (has_interface<PPB_Flash_File_FileRef>()) {
    result = get_interface<PPB_Flash_File_FileRef>()->
        QueryFile(resource.pp_resource(), info);
  }
  return result == PP_OK;
}

}  // namespace flash

}  // namespace pp
