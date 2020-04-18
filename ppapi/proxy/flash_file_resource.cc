// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/flash_file_resource.h"

#include <stddef.h>

#include "base/files/file_path.h"
#include "ipc/ipc_message.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/file_type_conversion.h"
#include "ppapi/shared_impl/scoped_pp_var.h"
#include "ppapi/shared_impl/time_conversion.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_file_ref_api.h"

namespace ppapi {
namespace proxy {

namespace {

// Returns the path that a PPB_FileRef resource references as long as it is an
// PP_FILESYSTEMTYPE_EXTERNAL path. Returns an empty string on error.
std::string GetPathFromFileRef(PP_Resource file_ref) {
  thunk::EnterResourceNoLock<thunk::PPB_FileRef_API> enter(file_ref, true);
  if (enter.failed())
    return std::string();
  if (enter.object()->GetFileSystemType() != PP_FILESYSTEMTYPE_EXTERNAL)
    return std::string();
  ScopedPPVar var(ScopedPPVar::PassRef(), enter.object()->GetAbsolutePath());
  StringVar* string_var = StringVar::FromPPVar(var.get());
  if (!string_var)
    return std::string();
  return string_var->value();
}

}  // namespace

FlashFileResource::FlashFileResource(Connection connection,
                                     PP_Instance instance)
    : PluginResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_FlashFile_Create());
}

FlashFileResource::~FlashFileResource() {
}

thunk::PPB_Flash_File_API* FlashFileResource::AsPPB_Flash_File_API() {
  return this;
}

int32_t FlashFileResource::OpenFile(PP_Instance /*instance*/,
                                    const char* path,
                                    int32_t mode,
                                    PP_FileHandle* file) {
  return OpenFileHelper(path, PepperFilePath::DOMAIN_MODULE_LOCAL, mode, file);
}

int32_t FlashFileResource::RenameFile(PP_Instance /*instance*/,
                                      const char* path_from,
                                      const char* path_to) {
  PepperFilePath pepper_from(PepperFilePath::DOMAIN_MODULE_LOCAL,
                             base::FilePath::FromUTF8Unsafe(path_from));
  PepperFilePath pepper_to(PepperFilePath::DOMAIN_MODULE_LOCAL,
                           base::FilePath::FromUTF8Unsafe(path_to));

  int32_t error = SyncCall<IPC::Message>(
      BROWSER, PpapiHostMsg_FlashFile_RenameFile(pepper_from, pepper_to));

  return error;
}

int32_t FlashFileResource::DeleteFileOrDir(PP_Instance /*instance*/,
                                           const char* path,
                                           PP_Bool recursive) {
  PepperFilePath pepper_path(PepperFilePath::DOMAIN_MODULE_LOCAL,
                             base::FilePath::FromUTF8Unsafe(path));

  int32_t error = SyncCall<IPC::Message>(
      BROWSER, PpapiHostMsg_FlashFile_DeleteFileOrDir(pepper_path,
                                                      PP_ToBool(recursive)));

  return error;
}

int32_t FlashFileResource::CreateDir(PP_Instance /*instance*/,
                                     const char* path) {
  PepperFilePath pepper_path(PepperFilePath::DOMAIN_MODULE_LOCAL,
                             base::FilePath::FromUTF8Unsafe(path));

  int32_t error = SyncCall<IPC::Message>(BROWSER,
      PpapiHostMsg_FlashFile_CreateDir(pepper_path));

  return error;
}

int32_t FlashFileResource::QueryFile(PP_Instance /*instance*/,
                                     const char* path,
                                     PP_FileInfo* info) {
  return QueryFileHelper(path, PepperFilePath::DOMAIN_MODULE_LOCAL, info);
}

int32_t FlashFileResource::GetDirContents(PP_Instance /*instance*/,
                                          const char* path,
                                          PP_DirContents_Dev** contents) {
  ppapi::DirContents entries;
  PepperFilePath pepper_path(PepperFilePath::DOMAIN_MODULE_LOCAL,
                             base::FilePath::FromUTF8Unsafe(path));

  int32_t error = SyncCall<PpapiPluginMsg_FlashFile_GetDirContentsReply>(
      BROWSER, PpapiHostMsg_FlashFile_GetDirContents(pepper_path), &entries);

  if (error == PP_OK) {
    // Copy the serialized dir entries to the output struct.
    *contents = new PP_DirContents_Dev;
    (*contents)->count = static_cast<int32_t>(entries.size());
    (*contents)->entries = new PP_DirEntry_Dev[entries.size()];
    for (size_t i = 0; i < entries.size(); i++) {
      const ppapi::DirEntry& source = entries[i];
      PP_DirEntry_Dev* dest = &(*contents)->entries[i];
      std::string name = source.name.AsUTF8Unsafe();
      char* name_copy = new char[name.size() + 1];
      memcpy(name_copy, name.c_str(), name.size() + 1);
      dest->name = name_copy;
      dest->is_dir = PP_FromBool(source.is_dir);
    }
  }

  return error;
}

void FlashFileResource::FreeDirContents(PP_Instance /*instance*/,
                                        PP_DirContents_Dev* contents) {
  for (int32_t i = 0; i < contents->count; ++i)
    delete[] contents->entries[i].name;
  delete[] contents->entries;
  delete contents;
}

int32_t FlashFileResource::CreateTemporaryFile(PP_Instance /*instance*/,
                                               PP_FileHandle* file) {
  if (!file)
    return PP_ERROR_BADARGUMENT;

  IPC::Message unused;
  ResourceMessageReplyParams reply_params;
  int32_t error = GenericSyncCall(BROWSER,
      PpapiHostMsg_FlashFile_CreateTemporaryFile(), &unused, &reply_params);
  if (error != PP_OK)
    return error;

  IPC::PlatformFileForTransit transit_file;
  if (!reply_params.TakeFileHandleAtIndex(0, &transit_file))
    return PP_ERROR_FAILED;

  *file = IPC::PlatformFileForTransitToPlatformFile(transit_file);
  return PP_OK;
}

int32_t FlashFileResource::OpenFileRef(PP_Instance /*instance*/,
                                       PP_Resource file_ref,
                                       int32_t mode,
                                       PP_FileHandle* file) {
  return OpenFileHelper(GetPathFromFileRef(file_ref),
                        PepperFilePath::DOMAIN_ABSOLUTE, mode, file);
}

int32_t FlashFileResource::QueryFileRef(PP_Instance /*instance*/,
                                        PP_Resource file_ref,
                                        PP_FileInfo* info) {
  return QueryFileHelper(GetPathFromFileRef(file_ref),
                         PepperFilePath::DOMAIN_ABSOLUTE, info);
}

int32_t FlashFileResource::OpenFileHelper(const std::string& path,
                                          PepperFilePath::Domain domain_type,
                                          int32_t mode,
                                          PP_FileHandle* file) {
  if (path.empty() ||
      !ppapi::PepperFileOpenFlagsToPlatformFileFlags(mode, NULL) ||
      !file)
    return PP_ERROR_BADARGUMENT;

  PepperFilePath pepper_path(domain_type, base::FilePath::FromUTF8Unsafe(path));

  IPC::Message unused;
  ResourceMessageReplyParams reply_params;
  int32_t error = GenericSyncCall(BROWSER,
      PpapiHostMsg_FlashFile_OpenFile(pepper_path, mode), &unused,
      &reply_params);
  if (error != PP_OK)
    return error;

  IPC::PlatformFileForTransit transit_file;
  if (!reply_params.TakeFileHandleAtIndex(0, &transit_file))
    return PP_ERROR_FAILED;

  *file = IPC::PlatformFileForTransitToPlatformFile(transit_file);
  return PP_OK;
}

int32_t FlashFileResource::QueryFileHelper(const std::string& path,
                                           PepperFilePath::Domain domain_type,
                                           PP_FileInfo* info) {
  if (path.empty() || !info)
    return PP_ERROR_BADARGUMENT;

  base::File::Info file_info;
  PepperFilePath pepper_path(domain_type, base::FilePath::FromUTF8Unsafe(path));

  int32_t error = SyncCall<PpapiPluginMsg_FlashFile_QueryFileReply>(BROWSER,
      PpapiHostMsg_FlashFile_QueryFile(pepper_path), &file_info);

  if (error == PP_OK) {
    info->size = file_info.size;
    info->creation_time = TimeToPPTime(file_info.creation_time);
    info->last_access_time = TimeToPPTime(file_info.last_accessed);
    info->last_modified_time = TimeToPPTime(file_info.last_modified);
    info->system_type = PP_FILESYSTEMTYPE_EXTERNAL;
    if (file_info.is_directory)
      info->type = PP_FILETYPE_DIRECTORY;
    else
      info->type = PP_FILETYPE_REGULAR;
  }

  return error;
}

}  // namespace proxy
}  // namespace ppapi
