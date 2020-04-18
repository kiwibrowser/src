// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From ppb_compositor.idl modified Wed Jan 27 17:39:22 2016.

#include <stdint.h>

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_compositor.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_compositor_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Bool IsCompositor(PP_Resource resource) {
  VLOG(4) << "PPB_Compositor::IsCompositor()";
  EnterResource<PPB_Compositor_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

PP_Resource Create(PP_Instance instance) {
  VLOG(4) << "PPB_Compositor::Create()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateCompositor(instance);
}

PP_Resource AddLayer(PP_Resource compositor) {
  VLOG(4) << "PPB_Compositor::AddLayer()";
  EnterResource<PPB_Compositor_API> enter(compositor, true);
  if (enter.failed())
    return 0;
  return enter.object()->AddLayer();
}

int32_t CommitLayers(PP_Resource compositor, struct PP_CompletionCallback cc) {
  VLOG(4) << "PPB_Compositor::CommitLayers()";
  EnterResource<PPB_Compositor_API> enter(compositor, cc, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->CommitLayers(enter.callback()));
}

int32_t ResetLayers(PP_Resource compositor) {
  VLOG(4) << "PPB_Compositor::ResetLayers()";
  EnterResource<PPB_Compositor_API> enter(compositor, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->ResetLayers();
}

const PPB_Compositor_0_1 g_ppb_compositor_thunk_0_1 = {
    &IsCompositor, &Create, &AddLayer, &CommitLayers, &ResetLayers};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_Compositor_0_1* GetPPB_Compositor_0_1_Thunk() {
  return &g_ppb_compositor_thunk_0_1;
}

}  // namespace thunk
}  // namespace ppapi
