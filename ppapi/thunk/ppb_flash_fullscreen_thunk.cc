// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/c/ppb_fullscreen.h"
#include "ppapi/c/private/ppb_flash_fullscreen.h"
#include "ppapi/thunk/thunk.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_fullscreen_api.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/resource_creation_api.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Bool IsFullscreen(PP_Instance instance) {
  EnterInstanceAPI<PPB_Flash_Fullscreen_API> enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->IsFullscreen(instance);
}

PP_Bool SetFullscreen(PP_Instance instance, PP_Bool fullscreen) {
  EnterInstanceAPI<PPB_Flash_Fullscreen_API> enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->SetFullscreen(instance, fullscreen);
}

// TODO(raymes): The codepaths for GetScreenSize in PPB_Fullscreen and
// PPB_Flash_Fullscreen are the same. Consider deprecating the flash version.
PP_Bool GetScreenSize(PP_Instance instance, PP_Size* size) {
  EnterInstance enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->GetScreenSize(instance, size);
}

const PPB_FlashFullscreen_0_1 g_ppb_flash_fullscreen_thunk = {
  &IsFullscreen,
  &SetFullscreen,
  &GetScreenSize
};

}  // namespace

const PPB_FlashFullscreen_0_1* GetPPB_FlashFullscreen_0_1_Thunk() {
  return &g_ppb_flash_fullscreen_thunk;
}

}  // namespace thunk
}  // namespace ppapi
