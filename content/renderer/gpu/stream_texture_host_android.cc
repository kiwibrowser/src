// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/gpu/stream_texture_host_android.h"

#include "base/unguessable_token.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "ipc/ipc_message_macros.h"

namespace content {

StreamTextureHost::StreamTextureHost(scoped_refptr<gpu::GpuChannelHost> channel,
                                     int32_t route_id)
    : route_id_(route_id),
      listener_(nullptr),
      channel_(std::move(channel)),
      weak_ptr_factory_(this) {
  DCHECK(channel_);
  DCHECK(route_id_);
}

StreamTextureHost::~StreamTextureHost() {
  if (channel_)
    channel_->RemoveRoute(route_id_);
}

bool StreamTextureHost::BindToCurrentThread(Listener* listener) {
  listener_ = listener;

  if (channel_) {
    channel_->AddRoute(route_id_, weak_ptr_factory_.GetWeakPtr());
    channel_->Send(new GpuStreamTextureMsg_StartListening(route_id_));
    return true;
  }

  return false;
}

bool StreamTextureHost::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(StreamTextureHost, message)
    IPC_MESSAGE_HANDLER(GpuStreamTextureMsg_FrameAvailable, OnFrameAvailable);
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  DCHECK(handled);
  return handled;
}

void StreamTextureHost::OnChannelError() {
  channel_ = nullptr;
}

void StreamTextureHost::OnFrameAvailable() {
  if (listener_)
    listener_->OnFrameAvailable();
}

void StreamTextureHost::SetStreamTextureSize(const gfx::Size& size) {
  if (channel_)
    channel_->Send(new GpuStreamTextureMsg_SetSize(route_id_, size));
}

void StreamTextureHost::ForwardStreamTextureForSurfaceRequest(
    const base::UnguessableToken& request_token) {
  if (channel_) {
    channel_->Send(new GpuStreamTextureMsg_ForwardForSurfaceRequest(
        route_id_, request_token));
  }
}

}  // namespace content
