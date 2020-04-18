// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/media_stream_track_resource_base.h"

#include <stddef.h>

#include "base/logging.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace ppapi {
namespace proxy {

MediaStreamTrackResourceBase::MediaStreamTrackResourceBase(
    Connection connection,
    PP_Instance instance,
    int pending_renderer_id,
    const std::string& id)
    : PluginResource(connection, instance),
      buffer_manager_(this),
      id_(id),
      has_ended_(false) {
  AttachToPendingHost(RENDERER, pending_renderer_id);
}

MediaStreamTrackResourceBase::MediaStreamTrackResourceBase(
    Connection connection,
    PP_Instance instance)
    : PluginResource(connection, instance),
      buffer_manager_(this),
      has_ended_(false) {
}

MediaStreamTrackResourceBase::~MediaStreamTrackResourceBase() {
}

void MediaStreamTrackResourceBase::SendEnqueueBufferMessageToHost(
    int32_t index) {
  DCHECK_GE(index, 0);
  DCHECK_LT(index, buffer_manager()->number_of_buffers());
  Post(RENDERER, PpapiHostMsg_MediaStreamTrack_EnqueueBuffer(index));
}

void MediaStreamTrackResourceBase::OnReplyReceived(
    const ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  PPAPI_BEGIN_MESSAGE_MAP(MediaStreamTrackResourceBase, msg)
    PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
        PpapiPluginMsg_MediaStreamTrack_InitBuffers, OnPluginMsgInitBuffers)
    PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
        PpapiPluginMsg_MediaStreamTrack_EnqueueBuffer, OnPluginMsgEnqueueBuffer)
    PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(
        PpapiPluginMsg_MediaStreamTrack_EnqueueBuffers,
        OnPluginMsgEnqueueBuffers)
    PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL_UNHANDLED(
        PluginResource::OnReplyReceived(params, msg))
  PPAPI_END_MESSAGE_MAP()
}

void MediaStreamTrackResourceBase::CloseInternal() {
  if (!has_ended_) {
    Post(RENDERER, PpapiHostMsg_MediaStreamTrack_Close());
    has_ended_ = true;
  }
}

void MediaStreamTrackResourceBase::OnPluginMsgInitBuffers(
    const ResourceMessageReplyParams& params,
    int32_t number_of_buffers,
    int32_t buffer_size,
    bool readonly) {
  base::SharedMemoryHandle shm_handle;
  params.TakeSharedMemoryHandleAtIndex(0, &shm_handle);
  buffer_manager_.SetBuffers(number_of_buffers, buffer_size,
                             std::unique_ptr<base::SharedMemory>(
                                 new base::SharedMemory(shm_handle, readonly)),
                             false);
}

void MediaStreamTrackResourceBase::OnPluginMsgEnqueueBuffer(
    const ResourceMessageReplyParams& params,
    int32_t index) {
  buffer_manager_.EnqueueBuffer(index);
}

void MediaStreamTrackResourceBase::OnPluginMsgEnqueueBuffers(
    const ResourceMessageReplyParams& params,
    const std::vector<int32_t>& indices) {
  for (size_t i = 0; i < indices.size(); ++i)
    buffer_manager_.EnqueueBuffer(indices[i]);
}

}  // namespace proxy
}  // namespace ppapi
