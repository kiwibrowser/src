// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_webgl_layer.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/imagebitmap/image_bitmap.h"
#include "third_party/blink/renderer/modules/webgl/webgl2_rendering_context.h"
#include "third_party/blink/renderer/modules/webgl/webgl_framebuffer.h"
#include "third_party/blink/renderer/modules/webgl/webgl_rendering_context.h"
#include "third_party/blink/renderer/modules/xr/xr_device.h"
#include "third_party/blink/renderer/modules/xr/xr_frame_provider.h"
#include "third_party/blink/renderer/modules/xr/xr_presentation_context.h"
#include "third_party/blink/renderer/modules/xr/xr_session.h"
#include "third_party/blink/renderer/modules/xr/xr_view.h"
#include "third_party/blink/renderer/modules/xr/xr_viewport.h"
#include "third_party/blink/renderer/platform/geometry/double_size.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"

namespace blink {

namespace {

const double kFramebufferMinScale = 0.2;
const double kFramebufferMaxScale = 1.0;

const double kViewportMinScale = 0.2;
const double kViewportMaxScale = 1.0;

// Because including base::ClampToRange would be a dependency violation
double ClampToRange(const double value, const double min, const double max) {
  return std::min(std::max(value, min), max);
}

}  // namespace

XRWebGLLayer* XRWebGLLayer::Create(
    XRSession* session,
    const WebGLRenderingContextOrWebGL2RenderingContext& context,
    const XRWebGLLayerInit& initializer,
    ExceptionState& exception_state) {
  if (session->ended()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "Cannot create an XRWebGLLayer for an "
                                      "XRSession which has already ended.");
    return nullptr;
  }

  WebGLRenderingContextBase* webgl_context;
  if (context.IsWebGL2RenderingContext()) {
    webgl_context = context.GetAsWebGL2RenderingContext();
  } else {
    webgl_context = context.GetAsWebGLRenderingContext();
  }

  if (webgl_context->isContextLost()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "Cannot create an XRWebGLLayer with a "
                                      "lost WebGL context.");
    return nullptr;
  }

  if (!webgl_context->IsXRDeviceCompatible(session->device())) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "The session's device is not the compatible device for this context.");
    return nullptr;
  }

  bool want_antialiasing = initializer.antialias();
  bool want_depth_buffer = initializer.depth();
  bool want_stencil_buffer = initializer.stencil();
  bool want_alpha_channel = initializer.alpha();
  bool want_multiview = initializer.multiview();

  double framebuffer_scale = session->DefaultFramebufferScale();

  if (initializer.hasFramebufferScaleFactor() &&
      initializer.framebufferScaleFactor() != 0.0) {
    // Clamp the developer-requested framebuffer size to ensure it's not too
    // small to see or unreasonably large.
    framebuffer_scale =
        ClampToRange(initializer.framebufferScaleFactor(), kFramebufferMinScale,
                     kFramebufferMaxScale);
  }

  DoubleSize framebuffers_size = session->IdealFramebufferSize();

  IntSize desired_size(framebuffers_size.Width() * framebuffer_scale,
                       framebuffers_size.Height() * framebuffer_scale);

  // Create an opaque WebGL Framebuffer
  WebGLFramebuffer* framebuffer = WebGLFramebuffer::CreateOpaque(webgl_context);

  scoped_refptr<XRWebGLDrawingBuffer> drawing_buffer =
      XRWebGLDrawingBuffer::Create(
          webgl_context->GetDrawingBuffer(), framebuffer->Object(),
          desired_size, want_alpha_channel, want_depth_buffer,
          want_stencil_buffer, want_antialiasing, want_multiview);

  if (!drawing_buffer) {
    exception_state.ThrowDOMException(kOperationError,
                                      "Unable to create a framebuffer.");
    return nullptr;
  }

  return new XRWebGLLayer(session, webgl_context, std::move(drawing_buffer),
                          framebuffer, framebuffer_scale);
}

XRWebGLLayer::XRWebGLLayer(XRSession* session,
                           WebGLRenderingContextBase* webgl_context,
                           scoped_refptr<XRWebGLDrawingBuffer> drawing_buffer,
                           WebGLFramebuffer* framebuffer,
                           double framebuffer_scale)
    : XRLayer(session, kXRWebGLLayerType),
      webgl_context_(webgl_context),
      drawing_buffer_(drawing_buffer),
      framebuffer_(framebuffer),
      framebuffer_scale_(framebuffer_scale) {
  DCHECK(drawing_buffer);
  // If the contents need mirroring, indicate that to the drawing buffer.
  if (session->exclusive() && session->outputContext() &&
      session->device()->external()) {
    mirroring_ = true;
    drawing_buffer_->SetMirrorClient(this);
  }
  UpdateViewports();
}

XRWebGLLayer::~XRWebGLLayer() {
  if (mirroring_)
    drawing_buffer_->SetMirrorClient(nullptr);
}

void XRWebGLLayer::getXRWebGLRenderingContext(
    WebGLRenderingContextOrWebGL2RenderingContext& result) const {
  if (webgl_context_->Version() == 2) {
    result.SetWebGL2RenderingContext(
        static_cast<WebGL2RenderingContext*>(webgl_context_.Get()));
  } else {
    result.SetWebGLRenderingContext(
        static_cast<WebGLRenderingContext*>(webgl_context_.Get()));
  }
}

XRViewport* XRWebGLLayer::getViewport(XRView* view) {
  if (!view || view->session() != session())
    return nullptr;

  return GetViewportForEye(view->EyeValue());
}

XRViewport* XRWebGLLayer::GetViewportForEye(XRView::Eye eye) {
  if (viewports_dirty_)
    UpdateViewports();

  if (eye == XRView::kEyeLeft)
    return left_viewport_;

  return right_viewport_;
}

void XRWebGLLayer::requestViewportScaling(double scale_factor) {
  if (!session()->exclusive()) {
    // TODO(bajones): For the moment we're just going to ignore viewport changes
    // in non-exclusive mode. This is legal, but probably not what developers
    // would like to see. Look into making viewport scale apply properly.
    scale_factor = 1.0;
  } else {
    // Clamp the developer-requested viewport size to ensure it's not too
    // small to see or larger than the framebuffer.
    scale_factor =
        ClampToRange(scale_factor, kViewportMinScale, kViewportMaxScale);
  }

  // Don't set this as the viewport_scale_ directly, since that would allow the
  // viewports to change mid-frame.
  requested_viewport_scale_ = scale_factor;
}

void XRWebGLLayer::UpdateViewports() {
  long framebuffer_width = framebufferWidth();
  long framebuffer_height = framebufferHeight();

  viewports_dirty_ = false;

  if (session()->exclusive()) {
    left_viewport_ =
        new XRViewport(0, 0, framebuffer_width * 0.5 * viewport_scale_,
                       framebuffer_height * viewport_scale_);
    right_viewport_ =
        new XRViewport(framebuffer_width * 0.5 * viewport_scale_, 0,
                       framebuffer_width * 0.5 * viewport_scale_,
                       framebuffer_height * viewport_scale_);

    session()->device()->frameProvider()->UpdateWebGLLayerViewports(this);

    // When mirroring make sure to also update the mirrored canvas UVs so it
    // only shows a single eye's data, cropped to display proportionally.
    if (session()->outputContext()) {
      float left = 0;
      float top = 0;
      float right = static_cast<float>(left_viewport_->width()) /
                    static_cast<float>(framebuffer_width);
      float bottom = static_cast<float>(left_viewport_->height()) /
                     static_cast<float>(framebuffer_height);

      // Adjust the UVs so that the mirrored content always fills the canvas
      // and is centered while staying proportional.
      DoubleSize output_size = session()->OutputCanvasSize();
      double output_aspect = output_size.Width() / output_size.Height();
      double viewport_aspect = static_cast<float>(left_viewport_->width()) /
                               static_cast<float>(left_viewport_->height());

      if (output_aspect > viewport_aspect) {
        float viewport_scale = bottom;
        output_aspect = viewport_aspect / output_aspect;
        top = 0.5 - (output_aspect * 0.5);
        bottom = top + output_aspect;
        top *= viewport_scale;
        bottom *= viewport_scale;
      } else {
        float viewport_scale = right;
        output_aspect = output_aspect / viewport_aspect;
        left = 0.5 - (output_aspect * 0.5);
        right = left + output_aspect;
        left *= viewport_scale;
        right *= viewport_scale;
      }

      session()->outputContext()->SetUV(FloatPoint(left, top),
                                        FloatPoint(right, bottom));
    }
  } else {
    left_viewport_ = new XRViewport(0, 0, framebuffer_width * viewport_scale_,
                                    framebuffer_height * viewport_scale_);
  }
}

void XRWebGLLayer::OverwriteColorBufferFromMailboxTexture(
    const gpu::MailboxHolder& mailbox_holder,
    const IntSize& size) {
  drawing_buffer_->OverwriteColorBufferFromMailboxTexture(mailbox_holder, size);
}

void XRWebGLLayer::OnFrameStart(
    const base::Optional<gpu::MailboxHolder>& buffer_mailbox_holder) {
  // If the requested scale has changed since the last from, update it now.
  if (viewport_scale_ != requested_viewport_scale_) {
    viewport_scale_ = requested_viewport_scale_;
    viewports_dirty_ = true;
  }

  framebuffer_->MarkOpaqueBufferComplete(true);
  framebuffer_->SetContentsChanged(false);
  if (buffer_mailbox_holder) {
    drawing_buffer_->UseSharedBuffer(buffer_mailbox_holder.value());
    is_direct_draw_frame = true;
  } else {
    is_direct_draw_frame = false;
  }
}

void XRWebGLLayer::OnFrameEnd() {
  framebuffer_->MarkOpaqueBufferComplete(false);
  if (is_direct_draw_frame) {
    drawing_buffer_->DoneWithSharedBuffer();
    is_direct_draw_frame = false;
  }

  // Submit the frame to the XR compositor.
  if (session()->exclusive()) {
    // Always call submit, but notify if the contents were changed or not.
    session()->device()->frameProvider()->SubmitWebGLLayer(
        this, framebuffer_->HaveContentsChanged());
  } else if (session()->outputContext()) {
    // Nothing to do if the framebuffer contents have not changed.
    if (framebuffer_->HaveContentsChanged()) {
      ImageBitmap* image_bitmap =
          ImageBitmap::Create(TransferToStaticBitmapImage(nullptr));
      session()->outputContext()->SetImage(image_bitmap);
    }
  }
}

void XRWebGLLayer::OnResize() {
  if (!session()->exclusive()) {
    // For non-exclusive sessions a resize indicates we should adjust the
    // drawing buffer size to match the canvas.
    DoubleSize framebuffers_size = session()->IdealFramebufferSize();

    IntSize desired_size(framebuffers_size.Width() * framebuffer_scale_,
                         framebuffers_size.Height() * framebuffer_scale_);
    drawing_buffer_->Resize(desired_size);
  }

  // With both exclusive and non-exclusive session the viewports should be
  // recomputed when the output canvas resizes.
  viewports_dirty_ = true;
}

scoped_refptr<StaticBitmapImage> XRWebGLLayer::TransferToStaticBitmapImage(
    std::unique_ptr<viz::SingleReleaseCallback>* out_release_callback) {
  return drawing_buffer_->TransferToStaticBitmapImage(out_release_callback);
}

void XRWebGLLayer::OnMirrorImageAvailable(
    scoped_refptr<StaticBitmapImage> image,
    std::unique_ptr<viz::SingleReleaseCallback> release_callback) {
  ImageBitmap* image_bitmap = ImageBitmap::Create(std::move(image));

  session()->outputContext()->SetImage(image_bitmap);

  if (mirror_release_callback_) {
    // TODO(bajones): We should probably have the compositor report to us when
    // it's done with the image, rather than reporting back that it's usable as
    // soon as we receive a new one.
    mirror_release_callback_->Run(gpu::SyncToken(), false /* lost_resource */);
  }

  mirror_release_callback_ = std::move(release_callback);
}

void XRWebGLLayer::Trace(blink::Visitor* visitor) {
  visitor->Trace(left_viewport_);
  visitor->Trace(right_viewport_);
  visitor->Trace(webgl_context_);
  visitor->Trace(framebuffer_);
  XRLayer::Trace(visitor);
}

void XRWebGLLayer::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(webgl_context_);
  XRLayer::TraceWrappers(visitor);
}

}  // namespace blink
