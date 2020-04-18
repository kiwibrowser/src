// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/video_source_resource.h"

#include "base/bind.h"
#include "ipc/ipc_message.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/pp_video_frame_private.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/ppb_image_data_proxy.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"

using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_VideoSource_Private_API;

namespace ppapi {
namespace proxy {

VideoSourceResource::VideoSourceResource(
    Connection connection,
    PP_Instance instance)
    : PluginResource(connection, instance),
      is_open_(false) {
  SendCreate(RENDERER, PpapiHostMsg_VideoSource_Create());
}

VideoSourceResource::~VideoSourceResource() {
}

PPB_VideoSource_Private_API*
    VideoSourceResource::AsPPB_VideoSource_Private_API() {
  return this;
}

int32_t VideoSourceResource::Open(
    const PP_Var& stream_url,
    scoped_refptr<TrackedCallback> callback) {
  if (TrackedCallback::IsPending(open_callback_))
    return PP_ERROR_INPROGRESS;

  open_callback_ = callback;

  scoped_refptr<StringVar> stream_url_var = StringVar::FromPPVar(stream_url);
  const uint32_t kMaxStreamIdSizeInBytes = 16384;
  if (!stream_url_var.get() ||
      stream_url_var->value().size() > kMaxStreamIdSizeInBytes)
    return PP_ERROR_BADARGUMENT;
  Call<PpapiPluginMsg_VideoSource_OpenReply>(RENDERER,
      PpapiHostMsg_VideoSource_Open(stream_url_var->value()),
      base::Bind(&VideoSourceResource::OnPluginMsgOpenComplete, this));
  return PP_OK_COMPLETIONPENDING;
}

int32_t VideoSourceResource::GetFrame(
    PP_VideoFrame_Private* frame,
    scoped_refptr<TrackedCallback> callback) {
  if (!is_open_)
    return PP_ERROR_FAILED;

  if (TrackedCallback::IsPending(get_frame_callback_))
    return PP_ERROR_INPROGRESS;

  get_frame_callback_ = callback;
  Call<PpapiPluginMsg_VideoSource_GetFrameReply>(RENDERER,
      PpapiHostMsg_VideoSource_GetFrame(),
      base::Bind(&VideoSourceResource::OnPluginMsgGetFrameComplete, this,
                 frame));
  return PP_OK_COMPLETIONPENDING;
}

void VideoSourceResource::Close() {
  Post(RENDERER, PpapiHostMsg_VideoSource_Close());

  if (TrackedCallback::IsPending(open_callback_))
    open_callback_->PostAbort();
  if (TrackedCallback::IsPending(get_frame_callback_))
    get_frame_callback_->PostAbort();
}

void VideoSourceResource::OnPluginMsgOpenComplete(
    const ResourceMessageReplyParams& reply_params) {
  if (TrackedCallback::IsPending(open_callback_)) {
    int32_t result = reply_params.result();
    if (result == PP_OK)
      is_open_ = true;
    open_callback_->Run(result);
  }
}

void VideoSourceResource::OnPluginMsgGetFrameComplete(
    PP_VideoFrame_Private* frame,
    const ResourceMessageReplyParams& reply_params,
    const HostResource& image_data,
    const PP_ImageDataDesc& image_desc,
    PP_TimeTicks timestamp) {
  // The callback may have been aborted by Close().
  if (TrackedCallback::IsPending(get_frame_callback_)) {
    int32_t result = reply_params.result();
    if (result == PP_OK &&
        PPB_ImageData_Shared::IsImageDataDescValid(image_desc)) {
      frame->timestamp = timestamp;

      base::SharedMemoryHandle handle;
      if (!reply_params.TakeSharedMemoryHandleAtIndex(0, &handle))
        frame->image_data = 0;
      frame->image_data =
          (new SimpleImageData(
              image_data, image_desc, handle))->GetReference();
    }
    get_frame_callback_->Run(result);
  }
}

}  // namespace proxy
}  // namespace ppapi
