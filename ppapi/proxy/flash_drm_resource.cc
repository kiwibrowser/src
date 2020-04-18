// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/flash_drm_resource.h"

#include "base/bind.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/file_ref_resource.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/var.h"

namespace ppapi {
namespace proxy {

FlashDRMResource::FlashDRMResource(Connection connection,
                                   PP_Instance instance)
    : PluginResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_FlashDRM_Create());
  SendCreate(RENDERER, PpapiHostMsg_FlashDRM_Create());
}

FlashDRMResource::~FlashDRMResource() {
}

thunk::PPB_Flash_DRM_API* FlashDRMResource::AsPPB_Flash_DRM_API() {
  return this;
}

int32_t FlashDRMResource::GetDeviceID(PP_Var* id,
                                      scoped_refptr<TrackedCallback> callback) {
  if (!id)
    return PP_ERROR_BADARGUMENT;

  *id = PP_MakeUndefined();

  Call<PpapiPluginMsg_FlashDRM_GetDeviceIDReply>(
      BROWSER,
      PpapiHostMsg_FlashDRM_GetDeviceID(),
      base::Bind(&FlashDRMResource::OnPluginMsgGetDeviceIDReply, this,
      id, callback));
  return PP_OK_COMPLETIONPENDING;
}

PP_Bool FlashDRMResource::GetHmonitor(int64_t* hmonitor) {
  int64_t hmonitor_out;
  int32_t result = SyncCall<PpapiPluginMsg_FlashDRM_GetHmonitorReply>(
      BROWSER,
      PpapiHostMsg_FlashDRM_GetHmonitor(),
      &hmonitor_out);
  if (result != PP_OK)
    return PP_FALSE;
  *hmonitor = hmonitor_out;
  return PP_TRUE;
}

int32_t FlashDRMResource::GetVoucherFile(
    PP_Resource* file_ref,
    scoped_refptr<TrackedCallback> callback) {
  if (!file_ref)
    return PP_ERROR_BADARGUMENT;

  *file_ref = 0;

  Call<PpapiPluginMsg_FlashDRM_GetVoucherFileReply>(
      RENDERER,
      PpapiHostMsg_FlashDRM_GetVoucherFile(),
      base::Bind(&FlashDRMResource::OnPluginMsgGetVoucherFileReply, this,
      file_ref, callback));
  return PP_OK_COMPLETIONPENDING;
}

int32_t FlashDRMResource::MonitorIsExternal(
    PP_Bool* is_external,
    scoped_refptr<TrackedCallback> callback) {
  if (!is_external)
    return PP_ERROR_BADARGUMENT;

  *is_external = PP_FALSE;

  Call<PpapiPluginMsg_FlashDRM_MonitorIsExternalReply>(
      BROWSER,
      PpapiHostMsg_FlashDRM_MonitorIsExternal(),
      base::Bind(&FlashDRMResource::OnPluginMsgMonitorIsExternalReply, this,
                 is_external, callback));
  return PP_OK_COMPLETIONPENDING;
}

void FlashDRMResource::OnPluginMsgGetDeviceIDReply(
    PP_Var* dest,
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params,
    const std::string& id) {
  if (TrackedCallback::IsPending(callback)) {
    if (params.result() == PP_OK)
      *dest = StringVar::StringToPPVar(id);
    callback->Run(params.result());
  }
}

void FlashDRMResource::OnPluginMsgGetVoucherFileReply(
    PP_Resource* dest,
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params,
    const FileRefCreateInfo& file_info) {
  if (TrackedCallback::IsPending(callback)) {
    if (params.result() == PP_OK) {
      *dest = FileRefResource::CreateFileRef(
          connection(),
          pp_instance(),
          file_info);
    }
    callback->Run(params.result());
  }
}

void FlashDRMResource::OnPluginMsgMonitorIsExternalReply(
    PP_Bool* dest,
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params,
    PP_Bool is_external) {
  if (TrackedCallback::IsPending(callback)) {
    if (params.result() == PP_OK)
      *dest = is_external;
    callback->Run(params.result());
  }
}

}  // namespace proxy
}  // namespace ppapi
