// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From private/ppb_flash_device_id.idl modified Thu Dec 20 13:10:26 2012.

#include <stdint.h>

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash_device_id.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_drm_api.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  VLOG(4) << "PPB_Flash_DeviceID::Create()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateFlashDRM(instance);
}

int32_t GetDeviceID(PP_Resource device_id,
                    struct PP_Var* id,
                    struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_Flash_DeviceID::GetDeviceID()";
  EnterResource<PPB_Flash_DRM_API> enter(device_id, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->GetDeviceID(id, enter.callback()));
}

const PPB_Flash_DeviceID_1_0 g_ppb_flash_deviceid_thunk_1_0 = {
  &Create,
  &GetDeviceID
};

}  // namespace

const PPB_Flash_DeviceID_1_0* GetPPB_Flash_DeviceID_1_0_Thunk() {
  return &g_ppb_flash_deviceid_thunk_1_0;
}

}  // namespace thunk
}  // namespace ppapi
