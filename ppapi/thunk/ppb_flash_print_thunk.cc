// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/c/pp_instance.h"
#include "ppapi/c/private/ppb_flash_print.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_functions_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

void InvokePrinting(PP_Instance instance) {
  EnterInstanceAPI<PPB_Flash_Functions_API> enter(instance);
  if (enter.failed())
    return;
  enter.functions()->InvokePrinting(instance);
}

const PPB_Flash_Print_1_0 g_ppb_flash_print_1_0_thunk = {
  &InvokePrinting,
};

}  // namespace

const PPB_Flash_Print_1_0* GetPPB_Flash_Print_1_0_Thunk() {
  return &g_ppb_flash_print_1_0_thunk;
}

}  // namespace thunk
}  // namespace ppapi
