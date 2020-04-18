// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/pp_video_frame_private.h"
#include "ppapi/c/private/ppb_video_destination_private.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/ppb_video_destination_private_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateVideoDestination(instance);
}

PP_Bool IsVideoDestination(PP_Resource resource) {
  EnterResource<PPB_VideoDestination_Private_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t Open(PP_Resource destination,
             PP_Var stream_url,
             PP_CompletionCallback callback) {
  EnterResource<PPB_VideoDestination_Private_API> enter(destination,
                                                        callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Open(stream_url, enter.callback()));
}

int32_t PutFrame(PP_Resource destination,
                 const PP_VideoFrame_Private* frame) {
  EnterResource<PPB_VideoDestination_Private_API> enter(destination, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->PutFrame(*frame);
}

void Close(PP_Resource destination) {
  EnterResource<PPB_VideoDestination_Private_API> enter(destination, true);
  if (enter.succeeded())
    enter.object()->Close();
}

const PPB_VideoDestination_Private_0_1
    g_ppb_video_destination_private_thunk_0_1 = {
  &Create,
  &IsVideoDestination,
  &Open,
  &PutFrame,
  &Close
};

}  // namespace

const PPB_VideoDestination_Private_0_1*
    GetPPB_VideoDestination_Private_0_1_Thunk() {
  return &g_ppb_video_destination_private_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi
