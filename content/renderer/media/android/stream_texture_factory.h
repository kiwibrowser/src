// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_ANDROID_STREAM_TEXTURE_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_ANDROID_STREAM_TEXTURE_FACTORY_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/unguessable_token.h"
#include "cc/layers/video_frame_provider.h"
#include "content/common/content_export.h"
#include "content/renderer/gpu/stream_texture_host_android.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "ui/gfx/geometry/size.h"

namespace gpu {
namespace gles2 {
class GLES2Interface;
}  // namespace gles2
class GpuChannelHost;
}  // namespace gpu

namespace ui {
class ContextProviderCommandBuffer;
}

namespace content {

class StreamTextureFactory;

// The proxy class for the gpu thread to notify the compositor thread
// when a new video frame is available.
class StreamTextureProxy : public StreamTextureHost::Listener {
 public:
  ~StreamTextureProxy() override;

  // Initialize and bind to |task_runner|, which becomes the thread that the
  // provided callback will be run on. This can be called on any thread, but
  // must be called with the same |task_runner| every time.
  void BindToTaskRunner(
      const base::Closure& received_frame_cb,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  // StreamTextureHost::Listener implementation:
  void OnFrameAvailable() override;

  // Set the streamTexture size.
  void SetStreamTextureSize(const gfx::Size& size);

  // Sends an IPC to the GPU process.
  // Asks the StreamTexture to forward its SurfaceTexture to the
  // ScopedSurfaceRequestManager, using the gpu::ScopedSurfaceRequestConduit.
  void ForwardStreamTextureForSurfaceRequest(
      const base::UnguessableToken& request_token);

  // Clears |received_frame_cb_| in a thread safe way.
  void ClearReceivedFrameCB();

  struct Deleter {
    inline void operator()(StreamTextureProxy* ptr) const { ptr->Release(); }
  };
 private:
  friend class StreamTextureFactory;
  explicit StreamTextureProxy(std::unique_ptr<StreamTextureHost> host);

  void BindOnThread();
  void Release();

  const std::unique_ptr<StreamTextureHost> host_;

  // Protects access to |received_frame_cb_| and |task_runner_|.
  base::Lock lock_;
  base::Closure received_frame_cb_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StreamTextureProxy);
};

typedef std::unique_ptr<StreamTextureProxy, StreamTextureProxy::Deleter>
    ScopedStreamTextureProxy;

// Factory class for managing stream textures.
class CONTENT_EXPORT StreamTextureFactory
    : public base::RefCounted<StreamTextureFactory> {
 public:
  static scoped_refptr<StreamTextureFactory> Create(
      scoped_refptr<ui::ContextProviderCommandBuffer> context_provider);

  // Create the StreamTextureProxy object. This internally calls
  // CreateSteamTexture with the recieved arguments. CreateSteamTexture
  // generates a texture and stores it in  *texture_id, the texture is produced
  // into a mailbox so it can be shipped in a VideoFrame, it creates a
  // gpu::StreamTexture and returns its route_id. If this route_id is  invalid
  // nullptr is returned and *texture_id will be set to 0. If the route_id is
  // valid it returns StreamTextureProxy object. The caller needs to take care
  // of cleaning up the texture_id.
  ScopedStreamTextureProxy CreateProxy(unsigned* texture_id,
                                       gpu::Mailbox* texture_mailbox);

  gpu::gles2::GLES2Interface* ContextGL();

 private:
  friend class base::RefCounted<StreamTextureFactory>;
  StreamTextureFactory(
      scoped_refptr<ui::ContextProviderCommandBuffer> context_provider);
  ~StreamTextureFactory();
  // Creates a gpu::StreamTexture and returns its id.  Sets |*texture_id| to the
  // client-side id of the gpu::StreamTexture. The texture is produced into
  // a mailbox so it can be shipped in a VideoFrame.
  unsigned CreateStreamTexture(unsigned* texture_id,
                               gpu::Mailbox* texture_mailbox);

  scoped_refptr<ui::ContextProviderCommandBuffer> context_provider_;
  scoped_refptr<gpu::GpuChannelHost> channel_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StreamTextureFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_ANDROID_STREAM_TEXTURE_FACTORY_H_
