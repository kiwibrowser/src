// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash_file.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_file_ref_api.h"
#include "ppapi/thunk/ppb_flash_file_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

// Returns 0 on failure.
PP_Instance GetInstanceFromFileRef(PP_Resource file_ref) {
  thunk::EnterResource<thunk::PPB_FileRef_API> enter(file_ref, true);
  if (enter.failed())
    return 0;
  return enter.resource()->pp_instance();
}

int32_t OpenFile(PP_Resource file_ref_id, int32_t mode, PP_FileHandle* file) {
  // TODO(brettw): this function should take an instance.
  // To work around this, use the PP_Instance from the resource.
  PP_Instance instance = GetInstanceFromFileRef(file_ref_id);
  EnterInstanceAPI<PPB_Flash_File_API> enter(instance);
  if (enter.failed())
    return PP_ERROR_BADARGUMENT;
  return enter.functions()->OpenFileRef(instance, file_ref_id, mode, file);
}

int32_t QueryFile(PP_Resource file_ref_id, struct PP_FileInfo* info) {
  // TODO(brettw): this function should take an instance.
  // To work around this, use the PP_Instance from the resource.
  PP_Instance instance = GetInstanceFromFileRef(file_ref_id);
  EnterInstanceAPI<PPB_Flash_File_API> enter(instance);
  if (enter.failed())
    return PP_ERROR_BADARGUMENT;
  return enter.functions()->QueryFileRef(instance, file_ref_id, info);
}

const PPB_Flash_File_FileRef g_ppb_flash_file_fileref_thunk = {
  &OpenFile,
  &QueryFile
};

}  // namespace

const PPB_Flash_File_FileRef* GetPPB_Flash_File_FileRef_Thunk() {
  return &g_ppb_flash_file_fileref_thunk;
}

}  // namespace thunk
}  // namespace ppapi
