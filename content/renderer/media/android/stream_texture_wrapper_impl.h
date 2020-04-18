// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_ANDROID_STREAM_TEXTURE_WRAPPER_IMPL_H_
#define CONTENT_RENDERER_MEDIA_ANDROID_STREAM_TEXTURE_WRAPPER_IMPL_H_

#include <memory>

#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "content/renderer/media/android/stream_texture_factory.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "media/base/android/stream_texture_wrapper.h"
#include "media/base/video_frame.h"

namespace content {

// Concrete implementation of StreamTextureWrapper. Any method can be called on
// any thread, but additional threading considerations are listed in the
// comments of individual methods.
//
// The StreamTexture is an abstraction allowing Chrome to wrap a SurfaceTexture
// living in the GPU process. It allows VideoFrames to be created from the
// SurfaceTexture's texture, in the Renderer process.
//
// The general idea behind our use of StreamTexture is as follows:
// - We create a client GL texture in the Renderer process.
// - We request the creation of a StreamTexture via the StreamTextureFactory,
// passing the client texture ID. The call is sent to the GPU process via the
// CommandBuffer. The "platform" GL texture reference associated with the client
// texture ID is looked up in the TextureManager. A StreamTexture is then
// created, wrapping a SurfaceTexture created from the texture reference. The
// SurfaceTexture's OnFrameAvailable() callback is tied to StreamTexture's
// OnFrameAvailable(), which fires an IPC accross the GPU channel.
// - We create a StreamTextureProxy in the Renderer process which listens for
// the IPC fired by the StreamTexture's OnFrameAvailable() callback.
// - We bind the StreamTextureProxy's lifetime to the |compositor_task_runner_|.
// - We wrap the client texture into a VideoFrame.
// - When the SurfaceTexture's OnFrameAvailable() callback is fired (and routed
// to the StreamTextureProxy living on the compositor thread), we notify
// |client_| that a new frame is available, via the DidReceiveFrame() callback.
class CONTENT_EXPORT StreamTextureWrapperImpl
    : public media::StreamTextureWrapper {
 public:
  // |enable_texture_copy| controls the VideoFrameMetadata::COPY_REQUIRED flag,
  // making sure it is correctly set on |current_frame_|, for webview scenarios.
  static media::ScopedStreamTextureWrapper Create(
      bool enable_texture_copy,
      scoped_refptr<StreamTextureFactory> factory,
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner);

  // Creates the underlying StreamTexture, and binds |stream_texture_proxy_| to
  // |compositor_task_runner|.
  //
  // Additional threading considerations:
  //   - Can be called from any thread.
  //   - Initialization will be posted to |main_task_runner_|.
  //   - |init_cb| will be run on the calling thread, and will be passed a bool
  //     indicating whether the initialization was successful.
  //   - New frames will be signaled on |compositor_task_runner| via |client|'s
  //     DidReceiveFrame() method.
  void Initialize(
      const base::Closure& received_frame_cb,
      const gfx::Size& natural_size,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
      const StreamTextureWrapperInitCB& init_cb) override;

  // Should be called when the Video size changes.
  // Can be called from any thread, but runs on |main_task_runner_|.
  void UpdateTextureSize(const gfx::Size& natural_size) override;

  // Returns the latest frame.
  // N.B: We create a single VideoFrame at initialization time (and update it
  // in UpdateTextureSize()), and repeatedly return it here. The underlying
  // texture's changes are signalled via |client|'s DidReceiveFrame() callback.
  scoped_refptr<media::VideoFrame> GetCurrentFrame() override;

  // Sends the StreamTexture to the browser process, to fulfill the request
  // identified by |request_token|.
  // Uses the gpu::ScopedSurfaceRequestConduit to forward the underlying
  // SurfaceTexture to the ScopedSurfaceRequestManager.
  void ForwardStreamTextureForSurfaceRequest(
      const base::UnguessableToken& request_token) override;

  // Clears the |received_frame_cb| passed in Initialize().
  // Should be safe to call from any thread.
  void ClearReceivedFrameCBOnAnyThread() override;

 private:
  StreamTextureWrapperImpl(
      bool enable_texture_copy,
      scoped_refptr<StreamTextureFactory> factory,
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner);
  ~StreamTextureWrapperImpl() override;

  // Destroys |this| safely on |main_task_runner_|.
  void Destroy() override;

  void InitializeOnMainThread(const base::Closure& received_frame_cb,
                              const StreamTextureWrapperInitCB& init_cb);

  void ReallocateVideoFrame(const gfx::Size& natural_size);

  void SetCurrentFrameInternal(
      const scoped_refptr<media::VideoFrame>& video_frame);

  bool enable_texture_copy_;

  // Client GL texture ID allocated to the StreamTexture.
  unsigned texture_id_;

  // GL texture mailbox for |texture_id_|.
  gpu::Mailbox texture_mailbox_;

  // Object for calling back the compositor thread to repaint the video when a
  // frame is available. It should be bound to |compositor_task_runner_|.
  ScopedStreamTextureProxy stream_texture_proxy_;

  // Size of the video frames.
  gfx::Size natural_size_;

  scoped_refptr<StreamTextureFactory> factory_;

  base::Lock current_frame_lock_;
  scoped_refptr<media::VideoFrame> current_frame_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  base::WeakPtrFactory<StreamTextureWrapperImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(StreamTextureWrapperImpl);
};

}  // namespace media

#endif  // CONTENT_RENDERER_MEDIA_ANDROID_STREAM_TEXTURE_WRAPPER_IMPL_H_
