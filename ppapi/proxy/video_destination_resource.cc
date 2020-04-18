// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/video_destination_resource.h"

#include "base/bind.h"
#include "ipc/ipc_message.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/pp_video_frame_private.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_image_data_api.h"

using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_VideoDestination_Private_API;

namespace ppapi {
namespace proxy {

VideoDestinationResource::VideoDestinationResource(
    Connection connection,
    PP_Instance instance)
    : PluginResource(connection, instance),
      is_open_(false) {
  SendCreate(RENDERER, PpapiHostMsg_VideoDestination_Create());
}

VideoDestinationResource::~VideoDestinationResource() {
}

PPB_VideoDestination_Private_API*
    VideoDestinationResource::AsPPB_VideoDestination_Private_API() {
  return this;
}

int32_t VideoDestinationResource::Open(
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
  Call<PpapiPluginMsg_VideoDestination_OpenReply>(RENDERER,
      PpapiHostMsg_VideoDestination_Open(stream_url_var->value()),
      base::Bind(&VideoDestinationResource::OnPluginMsgOpenComplete, this));
  return PP_OK_COMPLETIONPENDING;
}

int32_t VideoDestinationResource::PutFrame(
    const PP_VideoFrame_Private& frame) {
  if (!is_open_)
    return PP_ERROR_FAILED;

  thunk::EnterResourceNoLock<thunk::PPB_ImageData_API> enter_image(
      frame.image_data, true);
  if (enter_image.failed())
    return PP_ERROR_BADRESOURCE;

  // Check that the PP_Instance matches.
  Resource* image_object =
      PpapiGlobals::Get()->GetResourceTracker()->GetResource(frame.image_data);
  if (!image_object || pp_instance() != image_object->pp_instance()) {
    Log(PP_LOGLEVEL_ERROR,
        "VideoDestinationPrivateResource.PutFrame: Bad image resource.");
    return PP_ERROR_BADRESOURCE;
  }

  Post(RENDERER,
       PpapiHostMsg_VideoDestination_PutFrame(image_object->host_resource(),
                                              frame.timestamp));
  return PP_OK;
}

void VideoDestinationResource::Close() {
  Post(RENDERER, PpapiHostMsg_VideoDestination_Close());

  if (TrackedCallback::IsPending(open_callback_))
    open_callback_->PostAbort();
}

void VideoDestinationResource::OnPluginMsgOpenComplete(
    const ResourceMessageReplyParams& params) {
  if (TrackedCallback::IsPending(open_callback_)) {
    int32_t result = params.result();
    if (result == PP_OK)
      is_open_ = true;
    open_callback_->Run(result);
  }
}

}  // namespace proxy
}  // namespace ppapi
