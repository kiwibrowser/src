// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/flash_fullscreen_resource.h"

#include <stdint.h>

#include "ppapi/c/pp_bool.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace ppapi {
namespace proxy {

FlashFullscreenResource::FlashFullscreenResource(Connection connection,
                                                 PP_Instance instance)
    : PluginResource(connection, instance),
      is_fullscreen_(PP_FALSE) {
}

FlashFullscreenResource::~FlashFullscreenResource() {
}

thunk::PPB_Flash_Fullscreen_API*
FlashFullscreenResource::AsPPB_Flash_Fullscreen_API() {
  return this;
}

PP_Bool FlashFullscreenResource::IsFullscreen(PP_Instance instance) {
  return is_fullscreen_;
}

PP_Bool FlashFullscreenResource::SetFullscreen(PP_Instance instance,
                                               PP_Bool fullscreen) {
  if (!sent_create_to_renderer())
    SendCreate(RENDERER, PpapiHostMsg_FlashFullscreen_Create());
  int32_t result = SyncCall<IPC::Message>(RENDERER,
      PpapiHostMsg_FlashFullscreen_SetFullscreen(PP_ToBool(fullscreen)));
  return PP_FromBool(result == PP_OK);
}

void FlashFullscreenResource::SetLocalIsFullscreen(PP_Instance instance,
                                                   PP_Bool is_fullscreen) {
  is_fullscreen_ = is_fullscreen;
}

}  // namespace proxy
}  // namespace ppapi
