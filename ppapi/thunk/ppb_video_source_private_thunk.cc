// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/pp_video_frame_private.h"
#include "ppapi/c/private/ppb_video_source_private.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_video_source_private_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateVideoSource(instance);
}

PP_Bool IsVideoSource(PP_Resource resource) {
  EnterResource<PPB_VideoSource_Private_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t Open(PP_Resource source,
             PP_Var stream_url,
             PP_CompletionCallback callback) {
  EnterResource<PPB_VideoSource_Private_API> enter(source, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Open(stream_url, enter.callback()));
}

int32_t GetFrame(PP_Resource source,
                 PP_VideoFrame_Private* frame,
                 PP_CompletionCallback callback) {
  EnterResource<PPB_VideoSource_Private_API> enter(source, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->GetFrame(frame, enter.callback()));
}

void Close(PP_Resource source) {
  EnterResource<PPB_VideoSource_Private_API> enter(source, true);
  if (enter.succeeded())
    enter.object()->Close();
}

const PPB_VideoSource_Private_0_1 g_ppb_video_source_private_thunk_0_1 = {
  &Create,
  &IsVideoSource,
  &Open,
  &GetFrame,
  &Close
};

}  // namespace

const PPB_VideoSource_Private_0_1* GetPPB_VideoSource_Private_0_1_Thunk() {
  return &g_ppb_video_source_private_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi
