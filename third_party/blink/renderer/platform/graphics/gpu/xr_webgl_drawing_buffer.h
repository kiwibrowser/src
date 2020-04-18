// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_GPU_XR_WEBGL_DRAWING_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_GPU_XR_WEBGL_DRAWING_BUFFER_H_

#include "cc/layers/texture_layer_client.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"

namespace blink {

class DrawingBuffer;
class StaticBitmapImage;

class PLATFORM_EXPORT XRWebGLDrawingBuffer
    : public RefCounted<XRWebGLDrawingBuffer> {
 public:
  static scoped_refptr<XRWebGLDrawingBuffer> Create(DrawingBuffer*,
                                                    GLuint framebuffer,
                                                    const IntSize&,
                                                    bool want_alpha_channel,
                                                    bool want_depth_buffer,
                                                    bool want_stencil_buffer,
                                                    bool want_antialiasing,
                                                    bool want_multiview);

  gpu::gles2::GLES2Interface* ContextGL();
  bool ContextLost();

  const IntSize& size() const { return size_; }

  bool antialias() const { return anti_aliasing_mode_ != kNone; }
  bool depth() const { return depth_; }
  bool stencil() const { return stencil_; }
  bool alpha() const { return alpha_; }
  bool multiview() const { return multiview_; }

  void Resize(const IntSize&);

  void OverwriteColorBufferFromMailboxTexture(const gpu::MailboxHolder&,
                                              const IntSize& size);

  scoped_refptr<StaticBitmapImage> TransferToStaticBitmapImage(
      std::unique_ptr<viz::SingleReleaseCallback>* out_release_callback);

  class MirrorClient {
   public:
    virtual void OnMirrorImageAvailable(
        scoped_refptr<StaticBitmapImage>,
        std::unique_ptr<viz::SingleReleaseCallback>) = 0;
  };

  void SetMirrorClient(MirrorClient*);

  void UseSharedBuffer(const gpu::MailboxHolder&);
  void DoneWithSharedBuffer();

 private:
  struct ColorBuffer : public RefCounted<ColorBuffer> {
    ColorBuffer(XRWebGLDrawingBuffer*, const IntSize&, GLuint texture_id);
    ~ColorBuffer();

    // The owning XRWebGLDrawingBuffer. Note that DrawingBuffer is explicitly
    // destroyed by the beginDestruction method, which will eventually drain all
    // of its ColorBuffers.
    scoped_refptr<XRWebGLDrawingBuffer> drawing_buffer;
    const IntSize size;
    const GLuint texture_id = 0;

    // The mailbox used to send this buffer to the compositor.
    gpu::Mailbox mailbox;

    // The sync token for when this buffer was sent to the compositor.
    gpu::SyncToken produce_sync_token;

    // The sync token for when this buffer was received back from the
    // compositor.
    gpu::SyncToken receive_sync_token;

   private:
    WTF_MAKE_NONCOPYABLE(ColorBuffer);
  };

  XRWebGLDrawingBuffer(DrawingBuffer*,
                       GLuint framebuffer,
                       bool discard_framebuffer_supported,
                       bool want_alpha_channel,
                       bool want_depth_buffer,
                       bool want_stencil_buffer,
                       bool multiview_supported);

  bool Initialize(const IntSize&, bool use_multisampling, bool use_multiview);

  IntSize AdjustSize(const IntSize&);

  scoped_refptr<ColorBuffer> CreateColorBuffer();
  scoped_refptr<ColorBuffer> CreateOrRecycleColorBuffer();

  bool WantExplicitResolve() const;
  void BindAndResolveDestinationFramebuffer();
  void SwapColorBuffers();

  void MailboxReleased(scoped_refptr<ColorBuffer>,
                       const gpu::SyncToken&,
                       bool lost_resource);
  void MailboxReleasedToMirror(scoped_refptr<ColorBuffer>,
                               const gpu::SyncToken&,
                               bool lost_resource);

  // Reference to the DrawingBuffer that owns the GL context for this object.
  scoped_refptr<DrawingBuffer> drawing_buffer_;

  const GLuint framebuffer_ = 0;
  GLuint resolved_framebuffer_ = 0;
  GLuint multisample_renderbuffer_ = 0;
  scoped_refptr<ColorBuffer> back_color_buffer_ = 0;
  scoped_refptr<ColorBuffer> front_color_buffer_ = 0;
  GLuint depth_stencil_buffer_ = 0;
  IntSize size_;

  // Nonzero for shared buffer mode from UseSharedBuffer until
  // DoneWithSharedBuffer.
  GLuint shared_buffer_texture_id_ = 0;

  // Checking framebuffer completeness is extremely expensive, it's basically a
  // glFinish followed by a synchronous wait for a reply. Do so only once per
  // code path, and only in DCHECK mode.
  bool framebuffer_complete_checked_for_resize_ = false;
  bool framebuffer_complete_checked_for_swap_ = false;
  bool framebuffer_complete_checked_for_sharedbuffer_ = false;

  // Color buffers that were released by the XR compositor can be used again.
  Deque<scoped_refptr<ColorBuffer>> recycled_color_buffer_queue_;

  bool discard_framebuffer_supported_;
  bool depth_;
  bool stencil_;
  bool alpha_;
  bool multiview_;

  enum AntialiasingMode {
    kNone,
    kMSAAImplicitResolve,
    kMSAAExplicitResolve,
    kScreenSpaceAntialiasing,
  };

  AntialiasingMode anti_aliasing_mode_ = kNone;

  bool storage_texture_supported_ = false;
  int max_texture_size_ = 0;
  int sample_count_ = 0;

  MirrorClient* mirror_client_ = nullptr;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_GPU_XR_WEBGL_DRAWING_BUFFER_H_
