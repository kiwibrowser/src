// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/android/stream_texture_wrapper_impl.h"

#include "base/callback.h"
#include "cc/layers/video_frame_provider.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "media/base/bind_to_current_loop.h"

using gpu::gles2::GLES2Interface;

namespace {
// Non-member function to allow it to run even after this class is deleted.
void OnReleaseTexture(scoped_refptr<content::StreamTextureFactory> factories,
                      uint32_t texture_id,
                      const gpu::SyncToken& sync_token) {
  GLES2Interface* gl = factories->ContextGL();
  gl->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
  gl->DeleteTextures(1, &texture_id);
  // Flush to ensure that the stream texture gets deleted in a timely fashion.
  gl->ShallowFlushCHROMIUM();
}
}

namespace content {

StreamTextureWrapperImpl::StreamTextureWrapperImpl(
    bool enable_texture_copy,
    scoped_refptr<StreamTextureFactory> factory,
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner)
    : enable_texture_copy_(enable_texture_copy),
      texture_id_(0),
      factory_(factory),
      main_task_runner_(main_task_runner),
      weak_factory_(this) {}

StreamTextureWrapperImpl::~StreamTextureWrapperImpl() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (texture_id_) {
    GLES2Interface* gl = factory_->ContextGL();
    gl->DeleteTextures(1, &texture_id_);
    // Flush to ensure that the stream texture gets deleted in a timely fashion.
    gl->ShallowFlushCHROMIUM();
  }

  SetCurrentFrameInternal(nullptr);
}

media::ScopedStreamTextureWrapper StreamTextureWrapperImpl::Create(
    bool enable_texture_copy,
    scoped_refptr<StreamTextureFactory> factory,
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner) {
  return media::ScopedStreamTextureWrapper(new StreamTextureWrapperImpl(
      enable_texture_copy, factory, main_task_runner));
}

scoped_refptr<media::VideoFrame> StreamTextureWrapperImpl::GetCurrentFrame() {
  base::AutoLock auto_lock(current_frame_lock_);
  return current_frame_;
}

void StreamTextureWrapperImpl::ReallocateVideoFrame(
    const gfx::Size& natural_size) {
  DVLOG(2) << __func__;
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  GLES2Interface* gl = factory_->ContextGL();
  GLuint texture_target = GL_TEXTURE_EXTERNAL_OES;
  GLuint texture_id_ref =
      gl->CreateAndConsumeTextureCHROMIUM(texture_mailbox_.name);
  gl->Flush();

  gpu::SyncToken texture_mailbox_sync_token;
  if (texture_mailbox_sync_token.namespace_id() ==
      gpu::CommandBufferNamespace::IN_PROCESS) {
    // TODO(boliu): Remove this once Android WebView switches to IPC-based
    // command buffer for video.
    gl->GenSyncTokenCHROMIUM(texture_mailbox_sync_token.GetData());
  } else {
    gl->GenUnverifiedSyncTokenCHROMIUM(texture_mailbox_sync_token.GetData());
  }

  gpu::MailboxHolder holders[media::VideoFrame::kMaxPlanes] = {
      gpu::MailboxHolder(texture_mailbox_, texture_mailbox_sync_token,
                         texture_target)};

  scoped_refptr<media::VideoFrame> new_frame =
      media::VideoFrame::WrapNativeTextures(
          media::PIXEL_FORMAT_ARGB, holders,
          media::BindToCurrentLoop(
              base::Bind(&OnReleaseTexture, factory_, texture_id_ref)),
          natural_size, gfx::Rect(natural_size), natural_size,
          base::TimeDelta());

  if (enable_texture_copy_) {
    new_frame->metadata()->SetBoolean(media::VideoFrameMetadata::COPY_REQUIRED,
                                      true);
  }

  SetCurrentFrameInternal(new_frame);
}

void StreamTextureWrapperImpl::ForwardStreamTextureForSurfaceRequest(
    const base::UnguessableToken& request_token) {
  stream_texture_proxy_->ForwardStreamTextureForSurfaceRequest(request_token);
}

void StreamTextureWrapperImpl::ClearReceivedFrameCBOnAnyThread() {
  // Safely stop StreamTextureProxy from signaling the arrival of new frames.
  if (stream_texture_proxy_)
    stream_texture_proxy_->ClearReceivedFrameCB();
}

void StreamTextureWrapperImpl::SetCurrentFrameInternal(
    const scoped_refptr<media::VideoFrame>& video_frame) {
  base::AutoLock auto_lock(current_frame_lock_);
  current_frame_ = video_frame;
}

void StreamTextureWrapperImpl::UpdateTextureSize(const gfx::Size& new_size) {
  DVLOG(2) << __func__;

  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(
        FROM_HERE, base::Bind(&StreamTextureWrapperImpl::UpdateTextureSize,
                              weak_factory_.GetWeakPtr(), new_size));
    return;
  }

  // InitializeOnMainThread() hasn't run, or failed.
  if (!stream_texture_proxy_)
    return;

  if (natural_size_ == new_size)
    return;

  natural_size_ = new_size;

  ReallocateVideoFrame(new_size);
  stream_texture_proxy_->SetStreamTextureSize(new_size);
}

void StreamTextureWrapperImpl::Initialize(
    const base::Closure& received_frame_cb,
    const gfx::Size& natural_size,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
    const StreamTextureWrapperInitCB& init_cb) {
  DVLOG(2) << __func__;

  compositor_task_runner_ = compositor_task_runner;
  natural_size_ = natural_size;

  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&StreamTextureWrapperImpl::InitializeOnMainThread,
                            weak_factory_.GetWeakPtr(), received_frame_cb,
                            media::BindToCurrentLoop(init_cb)));
}

void StreamTextureWrapperImpl::InitializeOnMainThread(
    const base::Closure& received_frame_cb,
    const StreamTextureWrapperInitCB& init_cb) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(2) << __func__;

  stream_texture_proxy_ =
      factory_->CreateProxy(&texture_id_, &texture_mailbox_);
  if (!stream_texture_proxy_) {
    init_cb.Run(false);
    return;
  }

  ReallocateVideoFrame(natural_size_);

  stream_texture_proxy_->BindToTaskRunner(received_frame_cb,
                                          compositor_task_runner_);

  init_cb.Run(true);
}

void StreamTextureWrapperImpl::Destroy() {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    // base::Unretained is safe here because this function is the only one that
    // can call delete.
    main_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&StreamTextureWrapperImpl::Destroy, base::Unretained(this)));
    return;
  }

  delete this;
}

}  // namespace content
