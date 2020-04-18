// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_message_loop_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateFlashMessageLoop(instance);
}

PP_Bool IsFlashMessageLoop(PP_Resource resource) {
  EnterResource<PPB_Flash_MessageLoop_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t Run(PP_Resource resource) {
  EnterResource<PPB_Flash_MessageLoop_API> enter(resource, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  return enter.object()->Run();
}

void Quit(PP_Resource resource) {
  EnterResource<PPB_Flash_MessageLoop_API> enter(resource, true);
  if (enter.succeeded())
    enter.object()->Quit();
}

const PPB_Flash_MessageLoop g_ppb_flash_message_loop_thunk = {
  &Create,
  &IsFlashMessageLoop,
  &Run,
  &Quit
};

}  // namespace

const PPB_Flash_MessageLoop_0_1* GetPPB_Flash_MessageLoop_0_1_Thunk() {
  return &g_ppb_flash_message_loop_thunk;
}

}  // namespace thunk
}  // namespace ppapi
