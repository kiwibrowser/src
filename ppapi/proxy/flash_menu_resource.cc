// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/flash_menu_resource.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_flash_menu.h"

namespace ppapi {
namespace proxy {

FlashMenuResource::FlashMenuResource(Connection connection,
                                     PP_Instance instance)
    : PluginResource(connection, instance),
      selected_id_dest_(NULL) {
}

FlashMenuResource::~FlashMenuResource() {
}

bool FlashMenuResource::Initialize(const PP_Flash_Menu* menu_data) {
  SerializedFlashMenu serialized_menu;
  if (!menu_data || !serialized_menu.SetPPMenu(menu_data))
    return false;
  SendCreate(RENDERER, PpapiHostMsg_FlashMenu_Create(serialized_menu));
  return true;
}

thunk::PPB_Flash_Menu_API* FlashMenuResource::AsPPB_Flash_Menu_API() {
  return this;
}

int32_t FlashMenuResource::Show(
    const PP_Point* location,
    int32_t* selected_id,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(callback_))
    return PP_ERROR_INPROGRESS;

  selected_id_dest_ = selected_id;
  callback_ = callback;

  // This must be a sync message even though we don't care about the result.
  // The actual reply will be sent asynchronously in the future. This sync
  // request is due to the following deadlock:
  //
  //  1. Flash sends a show request to the renderer.
  //  2. The show handler in the renderer (in the case of full screen) requests
  //     the window rect which is a sync message to the browser. This causes
  //     a nested run loop to be spun up in the renderer.
  //  3. Flash expects context menus to be synchronous so it starts a nested
  //     message loop. This creates a second nested run loop in both the
  //     plugin and renderer process.
  //  4. The browser sends the window rect reply to unblock the renderer, but
  //     it's in the second nested run loop and the reply will *not*
  //     unblock this loop.
  //  5. The second loop won't exit until the message loop is complete, but
  //     that can't start until the first one exits.
  //
  // Having this message sync forces the sync request from the renderer to the
  // browser for the window rect will complete before Flash can run a nested
  // message loop to wait for the result of the menu.
  SyncCall<IPC::Message>(RENDERER, PpapiHostMsg_FlashMenu_Show(*location));
  return PP_OK_COMPLETIONPENDING;
}

void FlashMenuResource::OnReplyReceived(
    const proxy::ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  // Because the Show call is synchronous but we ignore the sync result, we
  // can't use the normal reply dispatch and have to do it manually here.
  switch (msg.type()) {
    case PpapiPluginMsg_FlashMenu_ShowReply::ID: {
      int32_t selected_id;
      if (ppapi::UnpackMessage<PpapiPluginMsg_FlashMenu_ShowReply>(
              msg, &selected_id))
        OnShowReply(params, selected_id);
      break;
    }
  }
}

void FlashMenuResource::OnShowReply(
    const proxy::ResourceMessageReplyParams& params,
    int32_t selected_id) {
  if (!TrackedCallback::IsPending(callback_))
    return;

  *selected_id_dest_ = selected_id;
  selected_id_dest_ = NULL;
  callback_->Run(params.result());
}

}  // namespace proxy
}  // namespace ppapi
