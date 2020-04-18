// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_GPU_STREAM_TEXTURE_HOST_ANDROID_H_
#define CONTENT_RENDERER_GPU_STREAM_TEXTURE_HOST_ANDROID_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_message.h"

namespace base {
class UnguessableToken;
}

namespace gfx {
class Size;
}

namespace gpu {
class GpuChannelHost;
}

namespace content {

// Class for handling all the IPC messages between the GPU process and
// StreamTextureProxy.
class StreamTextureHost : public IPC::Listener {
 public:
  explicit StreamTextureHost(scoped_refptr<gpu::GpuChannelHost> channel,
                             int32_t route_id);
  ~StreamTextureHost() override;

  // Listener class that is listening to the stream texture updates. It is
  // implemented by StreamTextureProxyImpl.
  class Listener {
   public:
    virtual void OnFrameAvailable() = 0;
    virtual ~Listener() {}
  };

  bool BindToCurrentThread(Listener* listener);

  // IPC::Channel::Listener implementation:
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelError() override;

  void SetStreamTextureSize(const gfx::Size& size);
  void ForwardStreamTextureForSurfaceRequest(
      const base::UnguessableToken& request_token);

 private:
  // Message handlers:
  void OnFrameAvailable();

  int32_t route_id_;
  Listener* listener_;
  scoped_refptr<gpu::GpuChannelHost> channel_;
  base::WeakPtrFactory<StreamTextureHost> weak_ptr_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StreamTextureHost);
};

}  // namespace content

#endif  // CONTENT_RENDERER_GPU_STREAM_TEXTURE_HOST_ANDROID_H_
