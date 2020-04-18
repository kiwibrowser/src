// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From ppb_compositor_layer.idl modified Wed Jan 27 17:10:16 2016.

#include <stdint.h>

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_compositor_layer.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_compositor_layer_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Bool IsCompositorLayer(PP_Resource resource) {
  VLOG(4) << "PPB_CompositorLayer::IsCompositorLayer()";
  EnterResource<PPB_CompositorLayer_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t SetColor(PP_Resource layer,
                 float red,
                 float green,
                 float blue,
                 float alpha,
                 const struct PP_Size* size) {
  VLOG(4) << "PPB_CompositorLayer::SetColor()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->SetColor(red, green, blue, alpha, size);
}

int32_t SetTexture_0_1(PP_Resource layer,
                       PP_Resource context,
                       uint32_t texture,
                       const struct PP_Size* size,
                       struct PP_CompletionCallback cc) {
  VLOG(4) << "PPB_CompositorLayer::SetTexture_0_1()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, cc, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetTexture0_1(context, texture, size, enter.callback()));
}

int32_t SetTexture(PP_Resource layer,
                   PP_Resource context,
                   uint32_t target,
                   uint32_t texture,
                   const struct PP_Size* size,
                   struct PP_CompletionCallback cc) {
  VLOG(4) << "PPB_CompositorLayer::SetTexture()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, cc, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->SetTexture(context, target, texture,
                                                    size, enter.callback()));
}

int32_t SetImage(PP_Resource layer,
                 PP_Resource image_data,
                 const struct PP_Size* size,
                 struct PP_CompletionCallback cc) {
  VLOG(4) << "PPB_CompositorLayer::SetImage()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, cc, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetImage(image_data, size, enter.callback()));
}

int32_t SetClipRect(PP_Resource layer, const struct PP_Rect* rect) {
  VLOG(4) << "PPB_CompositorLayer::SetClipRect()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->SetClipRect(rect);
}

int32_t SetTransform(PP_Resource layer, const float matrix[16]) {
  VLOG(4) << "PPB_CompositorLayer::SetTransform()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->SetTransform(matrix);
}

int32_t SetOpacity(PP_Resource layer, float opacity) {
  VLOG(4) << "PPB_CompositorLayer::SetOpacity()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->SetOpacity(opacity);
}

int32_t SetBlendMode(PP_Resource layer, PP_BlendMode mode) {
  VLOG(4) << "PPB_CompositorLayer::SetBlendMode()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->SetBlendMode(mode);
}

int32_t SetSourceRect(PP_Resource layer, const struct PP_FloatRect* rect) {
  VLOG(4) << "PPB_CompositorLayer::SetSourceRect()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->SetSourceRect(rect);
}

int32_t SetPremultipliedAlpha(PP_Resource layer, PP_Bool premult) {
  VLOG(4) << "PPB_CompositorLayer::SetPremultipliedAlpha()";
  EnterResource<PPB_CompositorLayer_API> enter(layer, true);
  if (enter.failed())
    return enter.retval();
  return enter.object()->SetPremultipliedAlpha(premult);
}

const PPB_CompositorLayer_0_1 g_ppb_compositorlayer_thunk_0_1 = {
    &IsCompositorLayer, &SetColor,
    &SetTexture_0_1,    &SetImage,
    &SetClipRect,       &SetTransform,
    &SetOpacity,        &SetBlendMode,
    &SetSourceRect,     &SetPremultipliedAlpha};

const PPB_CompositorLayer_0_2 g_ppb_compositorlayer_thunk_0_2 = {
    &IsCompositorLayer, &SetColor,
    &SetTexture,        &SetImage,
    &SetClipRect,       &SetTransform,
    &SetOpacity,        &SetBlendMode,
    &SetSourceRect,     &SetPremultipliedAlpha};

}  // namespace

PPAPI_THUNK_EXPORT const PPB_CompositorLayer_0_1*
GetPPB_CompositorLayer_0_1_Thunk() {
  return &g_ppb_compositorlayer_thunk_0_1;
}

PPAPI_THUNK_EXPORT const PPB_CompositorLayer_0_2*
GetPPB_CompositorLayer_0_2_Thunk() {
  return &g_ppb_compositorlayer_thunk_0_2;
}

}  // namespace thunk
}  // namespace ppapi
