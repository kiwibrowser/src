// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_video_destination_host.h"

#include "base/time/time.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "content/renderer/pepper/ppb_image_data_impl.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_image_data_api.h"

using ppapi::host::HostMessageContext;
using ppapi::host::ReplyMessageContext;

namespace content {

PepperVideoDestinationHost::PepperVideoDestinationHost(RendererPpapiHost* host,
                                                       PP_Instance instance,
                                                       PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
#if DCHECK_IS_ON()
      has_received_frame_(false),
#endif
      weak_factory_(this) {}

PepperVideoDestinationHost::~PepperVideoDestinationHost() {}

int32_t PepperVideoDestinationHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperVideoDestinationHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_VideoDestination_Open,
                                      OnHostMsgOpen)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_VideoDestination_PutFrame,
                                      OnHostMsgPutFrame)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_VideoDestination_Close,
                                        OnHostMsgClose)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperVideoDestinationHost::OnHostMsgOpen(
    HostMessageContext* context,
    const std::string& stream_url) {
  GURL gurl(stream_url);
  if (!gurl.is_valid())
    return PP_ERROR_BADARGUMENT;

  FrameWriterInterface* frame_writer = nullptr;
  if (!PepperToVideoTrackAdapter::Open(nullptr /* registry */, gurl.spec(),
                                       &frame_writer))
    return PP_ERROR_FAILED;
  frame_writer_.reset(frame_writer);

  ReplyMessageContext reply_context = context->MakeReplyMessageContext();
  reply_context.params.set_result(PP_OK);
  host()->SendReply(reply_context, PpapiPluginMsg_VideoDestination_OpenReply());
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperVideoDestinationHost::OnHostMsgPutFrame(
    HostMessageContext* context,
    const ppapi::HostResource& image_data_resource,
    PP_TimeTicks timestamp) {
  ppapi::thunk::EnterResourceNoLock<ppapi::thunk::PPB_ImageData_API> enter(
      image_data_resource.host_resource(), true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  PPB_ImageData_Impl* image_data_impl =
      static_cast<PPB_ImageData_Impl*>(enter.object());

  if (!PPB_ImageData_Impl::IsImageDataFormatSupported(
          image_data_impl->format()))
    return PP_ERROR_BADARGUMENT;

  if (!frame_writer_.get())
    return PP_ERROR_FAILED;

  // Convert PP_TimeTicks (a double, in seconds) to a video timestamp (int64_t,
  // nanoseconds).
  const int64_t timestamp_ns =
      static_cast<int64_t>(timestamp * base::Time::kNanosecondsPerSecond);
  // Check that timestamps are strictly increasing.
#if DCHECK_IS_ON()
  if (has_received_frame_)
    DCHECK_GT(timestamp_ns, previous_timestamp_ns_);
  has_received_frame_ = true;
  previous_timestamp_ns_ = timestamp_ns;
#endif

  frame_writer_->PutFrame(image_data_impl, timestamp_ns);

  return PP_OK;
}

int32_t PepperVideoDestinationHost::OnHostMsgClose(
    HostMessageContext* context) {
  frame_writer_.reset(nullptr);
  return PP_OK;
}

}  // namespace content
