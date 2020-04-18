// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/gpu/xr_webgl_drawing_buffer.h"

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/graphics/accelerated_static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/gpu/drawing_buffer.h"
#include "third_party/blink/renderer/platform/graphics/gpu/extensions_3d_util.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace blink {

// Large parts of this file have been shamelessly borrowed from
// platform/graphics/gpu/DrawingBuffer.cpp and simplified where applicable due
// to the more narrow use case. It may make sense in the future to abstract out
// some of the common bits into a base class?

XRWebGLDrawingBuffer::ColorBuffer::ColorBuffer(
    XRWebGLDrawingBuffer* drawing_buffer,
    const IntSize& size,
    GLuint texture_id)
    : drawing_buffer(drawing_buffer), size(size), texture_id(texture_id) {
  drawing_buffer->ContextGL()->GenMailboxCHROMIUM(mailbox.name);
}

XRWebGLDrawingBuffer::ColorBuffer::~ColorBuffer() {
  gpu::gles2::GLES2Interface* gl = drawing_buffer->ContextGL();
  if (receive_sync_token.HasData())
    gl->WaitSyncTokenCHROMIUM(receive_sync_token.GetConstData());
  gl->DeleteTextures(1, &texture_id);
}

scoped_refptr<XRWebGLDrawingBuffer> XRWebGLDrawingBuffer::Create(
    DrawingBuffer* drawing_buffer,
    GLuint framebuffer,
    const IntSize& size,
    bool want_alpha_channel,
    bool want_depth_buffer,
    bool want_stencil_buffer,
    bool want_antialiasing,
    bool want_multiview) {
  DCHECK(drawing_buffer);

  // Don't proceeed if the context is already lost.
  if (drawing_buffer->destroyed())
    return nullptr;

  gpu::gles2::GLES2Interface* gl = drawing_buffer->ContextGL();

  std::unique_ptr<Extensions3DUtil> extensions_util =
      Extensions3DUtil::Create(gl);
  if (!extensions_util->IsValid()) {
    return nullptr;
  }

  DCHECK(extensions_util->SupportsExtension("GL_OES_packed_depth_stencil"));
  extensions_util->EnsureExtensionEnabled("GL_OES_packed_depth_stencil");
  bool multisample_supported =
      want_antialiasing &&
      (extensions_util->SupportsExtension(
           "GL_CHROMIUM_framebuffer_multisample") ||
       extensions_util->SupportsExtension(
           "GL_EXT_multisampled_render_to_texture")) &&
      extensions_util->SupportsExtension("GL_OES_rgb8_rgba8");
  if (multisample_supported) {
    extensions_util->EnsureExtensionEnabled("GL_OES_rgb8_rgba8");
    if (extensions_util->SupportsExtension(
            "GL_CHROMIUM_framebuffer_multisample")) {
      extensions_util->EnsureExtensionEnabled(
          "GL_CHROMIUM_framebuffer_multisample");
    } else {
      extensions_util->EnsureExtensionEnabled(
          "GL_EXT_multisampled_render_to_texture");
    }
  }
  bool discard_framebuffer_supported =
      extensions_util->SupportsExtension("GL_EXT_discard_framebuffer");
  if (discard_framebuffer_supported)
    extensions_util->EnsureExtensionEnabled("GL_EXT_discard_framebuffer");

  // TODO(bajones): Support multiview.
  bool multiview_supported = false;

  scoped_refptr<XRWebGLDrawingBuffer> xr_drawing_buffer =
      base::AdoptRef(new XRWebGLDrawingBuffer(
          drawing_buffer, framebuffer, discard_framebuffer_supported,
          want_alpha_channel, want_depth_buffer, want_stencil_buffer,
          multiview_supported));
  if (!xr_drawing_buffer->Initialize(size, multisample_supported,
                                     multiview_supported)) {
    DLOG(ERROR) << "XRWebGLDrawingBuffer Initialization Failed";
    return nullptr;
  }

  return xr_drawing_buffer;
}

XRWebGLDrawingBuffer::XRWebGLDrawingBuffer(DrawingBuffer* drawing_buffer,
                                           GLuint framebuffer,
                                           bool discard_framebuffer_supported,
                                           bool want_alpha_channel,
                                           bool want_depth_buffer,
                                           bool want_stencil_buffer,
                                           bool multiview_supported)
    : drawing_buffer_(drawing_buffer),
      framebuffer_(framebuffer),
      discard_framebuffer_supported_(discard_framebuffer_supported),
      depth_(want_depth_buffer),
      stencil_(want_stencil_buffer),
      alpha_(want_alpha_channel),
      multiview_(false) {}

// TODO(bajones): The GL resources allocated in this function are leaking. Add
// a way to clean up the buffers when the layer is GCed or the session ends.
bool XRWebGLDrawingBuffer::Initialize(const IntSize& size,
                                      bool use_multisampling,
                                      bool use_multiview) {
  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();

  std::unique_ptr<Extensions3DUtil> extensions_util =
      Extensions3DUtil::Create(gl);

  gl->GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size_);
  DVLOG(2) << __FUNCTION__ << ": max_texture_size_=" << max_texture_size_;

  // Check context capabilities
  int max_sample_count = 0;
  anti_aliasing_mode_ = kNone;
  if (use_multisampling) {
    gl->GetIntegerv(GL_MAX_SAMPLES_ANGLE, &max_sample_count);
    anti_aliasing_mode_ = kMSAAExplicitResolve;
    if (extensions_util->SupportsExtension(
            "GL_EXT_multisampled_render_to_texture")) {
      anti_aliasing_mode_ = kMSAAImplicitResolve;
    } else if (extensions_util->SupportsExtension(
                   "GL_CHROMIUM_screen_space_antialiasing")) {
      anti_aliasing_mode_ = kScreenSpaceAntialiasing;
    }
  }
  DVLOG(2) << __FUNCTION__
           << ": anti_aliasing_mode_=" << static_cast<int>(anti_aliasing_mode_);

  storage_texture_supported_ =
      (drawing_buffer_->webgl_version() > DrawingBuffer::kWebGL1 ||
       extensions_util->SupportsExtension("GL_EXT_texture_storage")) &&
      anti_aliasing_mode_ == kScreenSpaceAntialiasing;

#if defined(OS_ANDROID)
  // On Android devices use a smaller numer of samples to provide more breathing
  // room for fill-rate-bound applications.
  sample_count_ = std::min(2, max_sample_count);
#else
  sample_count_ = std::min(4, max_sample_count);
#endif

  Resize(size);

  // It's possible that the drawing buffer allocation provokes a context loss,
  // so check again just in case.
  if (gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR) {
    return false;
  }

  return true;
}

gpu::gles2::GLES2Interface* XRWebGLDrawingBuffer::ContextGL() {
  return drawing_buffer_->ContextGL();
}

void XRWebGLDrawingBuffer::SetMirrorClient(MirrorClient* client) {
  mirror_client_ = client;
  if (mirror_client_) {
    // Immediately send a black 1x1 image to the mirror client to ensure that
    // it has content to show.
    sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(1, 1);
    mirror_client_->OnMirrorImageAvailable(
        StaticBitmapImage::Create(surface->makeImageSnapshot()), nullptr);
  }
}

bool XRWebGLDrawingBuffer::ContextLost() {
  return drawing_buffer_->destroyed();
}

IntSize XRWebGLDrawingBuffer::AdjustSize(const IntSize& new_size) {
  // Ensure we always have at least a 1x1 buffer
  float width = std::max(1, new_size.Width());
  float height = std::max(1, new_size.Height());

  float adjusted_scale =
      std::min(static_cast<float>(max_texture_size_) / width,
               static_cast<float>(max_texture_size_) / height);

  // Clamp if the desired size is greater than the maximum texture size for the
  // device. Scale both dimensions proportionally so that we avoid stretching.
  if (adjusted_scale < 1.0f) {
    width *= adjusted_scale;
    height *= adjusted_scale;
  }

  return IntSize(width, height);
}

void XRWebGLDrawingBuffer::OverwriteColorBufferFromMailboxTexture(
    const gpu::MailboxHolder& mailbox_holder,
    const IntSize& size_in) {
  TRACE_EVENT0("gpu", __FUNCTION__);
  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();

  gl->WaitSyncTokenCHROMIUM(mailbox_holder.sync_token.GetConstData());

  GLuint source_texture =
      gl->CreateAndConsumeTextureCHROMIUM(mailbox_holder.mailbox.name);

  GLuint dest_texture = back_color_buffer_->texture_id;

  // TODO(836496): clean this up and move some of the math to call site.
  int dest_width = size_.Width();
  int dest_height = size_.Height();
  int source_width = size_in.Width();
  int source_height = size_in.Height();

  int copy_width = std::min(source_width, dest_width);
  int copy_height = std::min(source_height, dest_height);

  // If the source is too small, center the image.
  int dest_x0 = source_width < dest_width ? (dest_width - source_width) / 2 : 0;
  int dest_y0 =
      source_height < dest_height ? (dest_height - source_height) / 2 : 0;
  int src_x0 = source_width > dest_width ? (source_width - dest_width) / 2 : 0;
  int src_y0 =
      source_height > dest_height ? (source_height - dest_height) / 2 : 0;

  gl->CopySubTextureCHROMIUM(
      source_texture, 0, GL_TEXTURE_2D, dest_texture, 0, dest_x0, dest_y0,
      src_x0, src_y0, copy_width, copy_height, false /* flipY */,
      false /* premultiplyAlpha */, false /* unmultiplyAlpha */);
}

void XRWebGLDrawingBuffer::UseSharedBuffer(
    const gpu::MailboxHolder& buffer_mailbox_holder) {
  DVLOG(3) << __FUNCTION__;

  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();

  // Create a texture backed by the shared buffer image.
  DCHECK(!shared_buffer_texture_id_);
  shared_buffer_texture_id_ =
      gl->CreateAndConsumeTextureCHROMIUM(buffer_mailbox_holder.mailbox.name);

  if (WantExplicitResolve()) {
    // Bind the shared texture to the destination framebuffer of
    // the explicit resolve step.
    if (!resolved_framebuffer_) {
      gl->GenFramebuffers(1, &resolved_framebuffer_);
    }
    gl->BindFramebuffer(GL_FRAMEBUFFER, resolved_framebuffer_);
  } else {
    // Bind the shared texture directly to the drawing framebuffer.
    gl->BindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  }

  if (anti_aliasing_mode_ == kMSAAImplicitResolve) {
    gl->FramebufferTexture2DMultisampleEXT(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        shared_buffer_texture_id_, 0, sample_count_);
  } else {
    // Explicit resolve, screen space antialiasing, or no antialiasing.
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, shared_buffer_texture_id_, 0);
  }

  if (!framebuffer_complete_checked_for_sharedbuffer_) {
    DCHECK(gl->CheckFramebufferStatus(GL_FRAMEBUFFER) ==
           GL_FRAMEBUFFER_COMPLETE);
    framebuffer_complete_checked_for_sharedbuffer_ = true;
  }

  if (discard_framebuffer_supported_) {
    const GLenum kAttachments[3] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT,
                                    GL_STENCIL_ATTACHMENT};
    gl->DiscardFramebufferEXT(GL_FRAMEBUFFER, 3, kAttachments);
  }

  DrawingBuffer::Client* client = drawing_buffer_->client();
  if (!client)
    return;
  client->DrawingBufferClientRestoreFramebufferBinding();
}

void XRWebGLDrawingBuffer::DoneWithSharedBuffer() {
  DVLOG(3) << __FUNCTION__;

  BindAndResolveDestinationFramebuffer();

  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();
  if (discard_framebuffer_supported_) {
    // Discard the depth and stencil attachments since we're done with them.
    // Don't discard the color buffer, we do need this rendered into the
    // shared buffer.
    if (WantExplicitResolve()) {
      gl->BindFramebuffer(GL_FRAMEBUFFER, resolved_framebuffer_);
    } else {
      gl->BindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    }
    const GLenum kAttachments[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    gl->DiscardFramebufferEXT(GL_FRAMEBUFFER,
                              sizeof(kAttachments) / sizeof(kAttachments[0]),
                              kAttachments);
  }

  // Always bind to the default framebuffer as a hint to the GPU to start
  // rendering now.
  gl->BindFramebuffer(GL_FRAMEBUFFER, 0);

  // Done with the texture created by CreateAndConsumeTexture, delete it.
  DCHECK(shared_buffer_texture_id_);
  gl->DeleteTextures(1, &shared_buffer_texture_id_);
  shared_buffer_texture_id_ = 0;

  DrawingBuffer::Client* client = drawing_buffer_->client();
  if (!client)
    return;
  client->DrawingBufferClientRestoreFramebufferBinding();
}

void XRWebGLDrawingBuffer::Resize(const IntSize& new_size) {
  IntSize adjusted_size = AdjustSize(new_size);

  if (adjusted_size == size_)
    return;

  // Don't attempt to resize if the context is lost.
  if (ContextLost())
    return;

  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();

  size_ = adjusted_size;

  // Free all mailboxes, because they are now of the wrong size. Only the
  // first call in this loop has any effect.
  recycled_color_buffer_queue_.clear();

  gl->BindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

  // Provide a depth and/or stencil buffer if requested.
  if (depth_ || stencil_) {
    if (depth_stencil_buffer_) {
      gl->DeleteRenderbuffers(1, &depth_stencil_buffer_);
      depth_stencil_buffer_ = 0;
    }
    gl->GenRenderbuffers(1, &depth_stencil_buffer_);
    gl->BindRenderbuffer(GL_RENDERBUFFER, depth_stencil_buffer_);

    if (anti_aliasing_mode_ == kMSAAImplicitResolve) {
      gl->RenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, sample_count_,
                                            GL_DEPTH24_STENCIL8_OES,
                                            size_.Width(), size_.Height());
    } else if (anti_aliasing_mode_ == kMSAAExplicitResolve) {
      gl->RenderbufferStorageMultisampleCHROMIUM(GL_RENDERBUFFER, sample_count_,
                                                 GL_DEPTH24_STENCIL8_OES,
                                                 size_.Width(), size_.Height());
    } else {
      gl->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES,
                              size_.Width(), size_.Height());
    }

    gl->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_RENDERBUFFER, depth_stencil_buffer_);
  }

  if (WantExplicitResolve()) {
    // If we're doing an explicit multisample resolve use the main framebuffer
    // as the multisample target and resolve into resolved_fbo_ when needed.
    GLenum multisample_format = alpha_ ? GL_RGBA8_OES : GL_RGB8_OES;

    if (multisample_renderbuffer_) {
      gl->DeleteRenderbuffers(1, &multisample_renderbuffer_);
      multisample_renderbuffer_ = 0;
    }

    gl->GenRenderbuffers(1, &multisample_renderbuffer_);
    gl->BindRenderbuffer(GL_RENDERBUFFER, multisample_renderbuffer_);
    gl->RenderbufferStorageMultisampleCHROMIUM(GL_RENDERBUFFER, sample_count_,
                                               multisample_format,
                                               size_.Width(), size_.Height());

    gl->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_RENDERBUFFER, multisample_renderbuffer_);

    // Now bind the resolve target framebuffer to attach the color textures to.
    if (!resolved_framebuffer_) {
      gl->GenFramebuffers(1, &resolved_framebuffer_);
    }
    gl->BindFramebuffer(GL_FRAMEBUFFER, resolved_framebuffer_);
  }

  back_color_buffer_ = CreateColorBuffer();
  front_color_buffer_ = nullptr;

  if (anti_aliasing_mode_ == kMSAAImplicitResolve) {
    gl->FramebufferTexture2DMultisampleEXT(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        back_color_buffer_->texture_id, 0, sample_count_);
  } else {
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, back_color_buffer_->texture_id, 0);
  }

  if (!framebuffer_complete_checked_for_resize_) {
    DCHECK(gl->CheckFramebufferStatus(GL_FRAMEBUFFER) ==
           GL_FRAMEBUFFER_COMPLETE);
    framebuffer_complete_checked_for_resize_ = true;
  }

  DrawingBuffer::Client* client = drawing_buffer_->client();
  client->DrawingBufferClientRestoreRenderbufferBinding();
  client->DrawingBufferClientRestoreFramebufferBinding();
}

scoped_refptr<XRWebGLDrawingBuffer::ColorBuffer>
XRWebGLDrawingBuffer::CreateColorBuffer() {
  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();

  GLuint texture_id = 0;
  gl->GenTextures(1, &texture_id);
  gl->BindTexture(GL_TEXTURE_2D, texture_id);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (storage_texture_supported_) {
    GLenum internal_storage_format = alpha_ ? GL_RGBA8 : GL_RGB8;
    gl->TexStorage2DEXT(GL_TEXTURE_2D, 1, internal_storage_format,
                        size_.Width(), size_.Height());
  } else {
    GLenum gl_format = alpha_ ? GL_RGBA : GL_RGB;
    gl->TexImage2D(GL_TEXTURE_2D, 0, gl_format, size_.Width(), size_.Height(),
                   0, gl_format, GL_UNSIGNED_BYTE, nullptr);
  }

  DrawingBuffer::Client* client = drawing_buffer_->client();
  client->DrawingBufferClientRestoreTexture2DBinding();

  return base::AdoptRef(new ColorBuffer(this, size_, texture_id));
}

scoped_refptr<XRWebGLDrawingBuffer::ColorBuffer>
XRWebGLDrawingBuffer::CreateOrRecycleColorBuffer() {
  if (!recycled_color_buffer_queue_.IsEmpty()) {
    scoped_refptr<ColorBuffer> recycled =
        recycled_color_buffer_queue_.TakeLast();
    if (recycled->receive_sync_token.HasData()) {
      gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();
      gl->WaitSyncTokenCHROMIUM(recycled->receive_sync_token.GetData());
    }
    DCHECK(recycled->size == size_);
    return recycled;
  }
  return CreateColorBuffer();
}

bool XRWebGLDrawingBuffer::WantExplicitResolve() const {
  return anti_aliasing_mode_ == kMSAAExplicitResolve;
}

void XRWebGLDrawingBuffer::BindAndResolveDestinationFramebuffer() {
  // Ensure that the mode-appropriate destination framebuffer's color
  // attachment contains the drawn content after any antialiasing steps needed.

  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();

  DrawingBuffer::Client* client = drawing_buffer_->client();

  // Resolve multisample buffers if needed
  if (WantExplicitResolve()) {
    DVLOG(3) << __FUNCTION__ << ": explicit resolve";
    gl->BindFramebuffer(GL_READ_FRAMEBUFFER_ANGLE, framebuffer_);
    gl->BindFramebuffer(GL_DRAW_FRAMEBUFFER_ANGLE, resolved_framebuffer_);
    gl->Disable(GL_SCISSOR_TEST);

    int width = size_.Width();
    int height = size_.Height();
    // Use NEAREST, because there is no scale performed during the blit.
    gl->BlitFramebufferCHROMIUM(0, 0, width, height, 0, 0, width, height,
                                GL_COLOR_BUFFER_BIT, GL_NEAREST);

    gl->BindFramebuffer(GL_FRAMEBUFFER, resolved_framebuffer_);

    client->DrawingBufferClientRestoreScissorTest();
  } else {
    gl->BindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    if (anti_aliasing_mode_ == kScreenSpaceAntialiasing) {
      DVLOG(3) << __FUNCTION__ << ": screen space antialiasing";
      gl->ApplyScreenSpaceAntialiasingCHROMIUM();
    } else {
      DVLOG(3) << __FUNCTION__ << ": nothing to do";
    }
  }

  // On exit, leaves the destination framebuffer active. Caller is responsible
  // for restoring client bindings.
}

// Swap the front and back buffers. After this call the front buffer should
// contain the previously rendered content, resolved from the multisample
// renderbuffer if needed.
void XRWebGLDrawingBuffer::SwapColorBuffers() {
  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();

  DrawingBuffer::Client* client = drawing_buffer_->client();

  BindAndResolveDestinationFramebuffer();

  // Swap buffers
  front_color_buffer_ = back_color_buffer_;
  back_color_buffer_ = CreateOrRecycleColorBuffer();

  if (anti_aliasing_mode_ == kMSAAImplicitResolve) {
    gl->FramebufferTexture2DMultisampleEXT(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        back_color_buffer_->texture_id, 0, sample_count_);
  } else {
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, back_color_buffer_->texture_id, 0);
  }

  if (!framebuffer_complete_checked_for_swap_) {
    DCHECK(gl->CheckFramebufferStatus(GL_FRAMEBUFFER) ==
           GL_FRAMEBUFFER_COMPLETE);
    framebuffer_complete_checked_for_swap_ = true;
  }

  if (discard_framebuffer_supported_) {
    const GLenum kAttachments[3] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT,
                                    GL_STENCIL_ATTACHMENT};
    gl->DiscardFramebufferEXT(GL_FRAMEBUFFER, 3, kAttachments);
  }

  client->DrawingBufferClientRestoreFramebufferBinding();
}

scoped_refptr<StaticBitmapImage>
XRWebGLDrawingBuffer::TransferToStaticBitmapImage(
    std::unique_ptr<viz::SingleReleaseCallback>* out_release_callback) {
  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();
  scoped_refptr<ColorBuffer> buffer;
  bool success = false;

  // Ensure the context isn't lost and the framebuffer is complete before
  // continuing.
  if (!ContextLost()) {
    SwapColorBuffers();

    buffer = front_color_buffer_;

    gl->ProduceTextureDirectCHROMIUM(buffer->texture_id, buffer->mailbox.name);
    gl->GenUnverifiedSyncTokenCHROMIUM(buffer->produce_sync_token.GetData());

    // This should only fail if the context is lost during the buffer swap.
    if (buffer->produce_sync_token.HasData()) {
      success = true;
    }
  }

  if (!success) {
    // If we can't get a mailbox, return an transparent black ImageBitmap.
    // The only situation in which this could happen is when two or more calls
    // to transferToImageBitmap are made back-to-back, if the framebuffer is
    // incomplete (likely due to a failed buffer allocation), or when the
    // context gets lost.
    sk_sp<SkSurface> surface =
        SkSurface::MakeRasterN32Premul(size_.Width(), size_.Height());
    return StaticBitmapImage::Create(surface->makeImageSnapshot());
  }

  // This holds a ref on the XRWebGLDrawingBuffer that will keep it alive
  // until the mailbox is released (and while the callback is running).
  auto func =
      WTF::Bind(mirror_client_ ? &XRWebGLDrawingBuffer::MailboxReleasedToMirror
                               : &XRWebGLDrawingBuffer::MailboxReleased,
                scoped_refptr<XRWebGLDrawingBuffer>(this), buffer);

  std::unique_ptr<viz::SingleReleaseCallback> release_callback =
      viz::SingleReleaseCallback::Create(std::move(func));

  // Make our own textureId that is a reference on the same texture backing
  // being used as the front buffer. We do not need to wait on the sync
  // token since the mailbox was produced on the same GL context that we are
  // using here. Similarly, the release callback will run on the same context so
  // we don't need to send a sync token for this consume action back to it.
  GLuint texture_id = gl->CreateAndConsumeTextureCHROMIUM(buffer->mailbox.name);

  if (out_release_callback) {
    *out_release_callback = std::move(release_callback);
  } else {
    release_callback->Run(gpu::SyncToken(), true /* lost_resource */);
  }

  return AcceleratedStaticBitmapImage::CreateFromWebGLContextImage(
      buffer->mailbox, buffer->produce_sync_token, texture_id,
      drawing_buffer_->ContextProviderWeakPtr(), size_);
}

void XRWebGLDrawingBuffer::MailboxReleased(
    scoped_refptr<ColorBuffer> color_buffer,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {
  // If the mailbox has been returned by the compositor then it is no
  // longer being presented, and so is no longer the front buffer.
  if (color_buffer == front_color_buffer_)
    front_color_buffer_ = nullptr;

  // Update the SyncToken to ensure that we will wait for it even if we
  // immediately destroy this buffer.
  color_buffer->receive_sync_token = sync_token;

  if (drawing_buffer_->destroyed() || color_buffer->size != size_ ||
      lost_resource) {
    return;
  }

  const size_t cache_limit = 2;
  while (recycled_color_buffer_queue_.size() >= cache_limit)
    recycled_color_buffer_queue_.TakeLast();

  recycled_color_buffer_queue_.push_front(color_buffer);
}

void XRWebGLDrawingBuffer::MailboxReleasedToMirror(
    scoped_refptr<ColorBuffer> color_buffer,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {
  if (!mirror_client_ || lost_resource) {
    MailboxReleased(std::move(color_buffer), sync_token, lost_resource);
    return;
  }

  gpu::gles2::GLES2Interface* gl = drawing_buffer_->ContextGL();
  color_buffer->receive_sync_token = sync_token;

  auto func =
      WTF::Bind(&XRWebGLDrawingBuffer::MailboxReleased,
                scoped_refptr<XRWebGLDrawingBuffer>(this), color_buffer);

  std::unique_ptr<viz::SingleReleaseCallback> release_callback =
      viz::SingleReleaseCallback::Create(std::move(func));

  GLuint texture_id =
      gl->CreateAndConsumeTextureCHROMIUM(color_buffer->mailbox.name);

  scoped_refptr<StaticBitmapImage> image =
      AcceleratedStaticBitmapImage::CreateFromWebGLContextImage(
          color_buffer->mailbox, color_buffer->produce_sync_token, texture_id,
          drawing_buffer_->ContextProviderWeakPtr(), color_buffer->size);

  mirror_client_->OnMirrorImageAvailable(std::move(image),
                                         std::move(release_callback));
}

}  // namespace blink
