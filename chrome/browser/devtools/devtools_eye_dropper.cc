// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/devtools_eye_dropper.h"

#include "base/bind.h"
#include "build/build_config.h"
#include "cc/paint/skia_paint_canvas.h"
#include "components/viz/common/features.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/common/cursor_info.h"
#include "content/public/common/screen_info.h"
#include "media/base/limits.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_mouse_event.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpaceXform.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/geometry/size_conversions.h"

DevToolsEyeDropper::DevToolsEyeDropper(content::WebContents* web_contents,
                                       EyeDropperCallback callback)
    : content::WebContentsObserver(web_contents),
      callback_(callback),
      last_cursor_x_(-1),
      last_cursor_y_(-1),
      host_(nullptr),
      use_video_capture_api_(
          base::FeatureList::IsEnabled(features::kVizDisplayCompositor) ||
          base::FeatureList::IsEnabled(
              features::kUseVideoCaptureApiForDevToolsSnapshots)),
      weak_factory_(this) {
  mouse_event_callback_ =
      base::Bind(&DevToolsEyeDropper::HandleMouseEvent, base::Unretained(this));
  content::RenderViewHost* rvh = web_contents->GetRenderViewHost();
  if (rvh) {
    AttachToHost(rvh->GetWidget());
    UpdateFrame();
  }
}

DevToolsEyeDropper::~DevToolsEyeDropper() {
  DetachFromHost();
}

void DevToolsEyeDropper::AttachToHost(content::RenderWidgetHost* host) {
  host_ = host;
  host_->AddMouseEventCallback(mouse_event_callback_);

  if (!use_video_capture_api_)
    return;

  // The view can be null if the renderer process has crashed.
  // (https://crbug.com/847363)
  if (!host_->GetView())
    return;

  // Capturing a full-page screenshot can be costly so we shouldn't do it too
  // often. We can capture at a lower frame rate without hurting the user
  // experience.
  constexpr static int kMaxFrameRate = 15;

  // Create and configure the video capturer.
  video_capturer_ = host_->GetView()->CreateVideoCapturer();
  video_capturer_->SetResolutionConstraints(
      host_->GetView()->GetViewBounds().size(),
      host_->GetView()->GetViewBounds().size(), true);
  video_capturer_->SetAutoThrottlingEnabled(false);
  video_capturer_->SetMinSizeChangePeriod(base::TimeDelta());
  video_capturer_->SetFormat(media::PIXEL_FORMAT_ARGB,
                             media::COLOR_SPACE_UNSPECIFIED);
  video_capturer_->SetMinCapturePeriod(base::TimeDelta::FromSeconds(1) /
                                       kMaxFrameRate);
  video_capturer_->Start(this);
}

void DevToolsEyeDropper::DetachFromHost() {
  if (!host_)
    return;
  host_->RemoveMouseEventCallback(mouse_event_callback_);
  content::CursorInfo cursor_info;
  cursor_info.type = blink::WebCursorInfo::kTypePointer;
  host_->SetCursor(cursor_info);
  video_capturer_.reset();
  host_ = nullptr;
}

void DevToolsEyeDropper::RenderViewCreated(content::RenderViewHost* host) {
  if (!host_) {
    AttachToHost(host->GetWidget());
    UpdateFrame();
  }
}

void DevToolsEyeDropper::RenderViewDeleted(content::RenderViewHost* host) {
  if (host->GetWidget() == host_) {
    DetachFromHost();
    ResetFrame();
  }
}

void DevToolsEyeDropper::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {
  if ((old_host && old_host->GetWidget() == host_) || (!old_host && !host_)) {
    DetachFromHost();
    AttachToHost(new_host->GetWidget());
    UpdateFrame();
  }
}

void DevToolsEyeDropper::DidReceiveCompositorFrame() {
  UpdateFrame();
}

void DevToolsEyeDropper::UpdateFrame() {
  if (use_video_capture_api_ || !host_ || !host_->GetView())
    return;

  // TODO(miu): This is the wrong size. It's the size of the view on-screen, and
  // not the rendering size of the view. The latter is what is wanted here, so
  // that the resulting bitmap's pixel coordinates line-up with the
  // blink::WebMouseEvent coordinates. http://crbug.com/73362
  gfx::Size should_be_rendering_size = host_->GetView()->GetViewBounds().size();
  host_->GetView()->CopyFromSurface(
      gfx::Rect(), should_be_rendering_size,
      base::BindOnce(&DevToolsEyeDropper::FrameUpdated,
                     weak_factory_.GetWeakPtr()));
}

void DevToolsEyeDropper::ResetFrame() {
  frame_.reset();
  last_cursor_x_ = -1;
  last_cursor_y_ = -1;
}

void DevToolsEyeDropper::FrameUpdated(const SkBitmap& bitmap) {
  DCHECK(!use_video_capture_api_);
  if (bitmap.drawsNothing())
    return;
  frame_ = bitmap;
  UpdateCursor();
}

bool DevToolsEyeDropper::HandleMouseEvent(const blink::WebMouseEvent& event) {
  last_cursor_x_ = event.PositionInWidget().x;
  last_cursor_y_ = event.PositionInWidget().y;
  if (frame_.drawsNothing())
    return true;

  if (event.button == blink::WebMouseEvent::Button::kLeft &&
      (event.GetType() == blink::WebInputEvent::kMouseDown ||
       event.GetType() == blink::WebInputEvent::kMouseMove)) {
    if (last_cursor_x_ < 0 || last_cursor_x_ >= frame_.width() ||
        last_cursor_y_ < 0 || last_cursor_y_ >= frame_.height()) {
      return true;
    }

    SkColor sk_color = frame_.getColor(last_cursor_x_, last_cursor_y_);
    uint8_t rgba_color[4] = {
        SkColorGetR(sk_color), SkColorGetG(sk_color), SkColorGetB(sk_color),
        SkColorGetA(sk_color),
    };

    // The picked colors are expected to be sRGB. Create a color transform from
    // |frame_|'s color space to sRGB.
    // TODO(ccameron): We don't actually know |frame_|'s color space, so just
    // use |host_|'s current display's color space. This will almost always be
    // the right color space, but is sloppy.
    // http://crbug.com/758057
    content::ScreenInfo screen_info;
    host_->GetScreenInfo(&screen_info);
    gfx::ColorSpace frame_color_space = screen_info.color_space;
    std::unique_ptr<SkColorSpaceXform> frame_color_space_to_srgb_xform =
        SkColorSpaceXform::New(frame_color_space.ToSkColorSpace().get(),
                               SkColorSpace::MakeSRGB().get());
    if (frame_color_space_to_srgb_xform) {
      bool xform_apply_result = frame_color_space_to_srgb_xform->apply(
          SkColorSpaceXform::kRGBA_8888_ColorFormat, rgba_color,
          SkColorSpaceXform::kRGBA_8888_ColorFormat, rgba_color, 1,
          kUnpremul_SkAlphaType);
      DCHECK(xform_apply_result);
    }

    callback_.Run(rgba_color[0], rgba_color[1], rgba_color[2], rgba_color[3]);
  }
  UpdateCursor();
  return true;
}

void DevToolsEyeDropper::UpdateCursor() {
  if (!host_ || frame_.drawsNothing())
    return;

  if (last_cursor_x_ < 0 || last_cursor_x_ >= frame_.width() ||
      last_cursor_y_ < 0 || last_cursor_y_ >= frame_.height()) {
    return;
  }

// Due to platform limitations, we are using two different cursors
// depending on the platform. Mac and Win have large cursors with two circles
// for original spot and its magnified projection; Linux gets smaller (64 px)
// magnified projection only with centered hotspot.
// Mac Retina requires cursor to be > 120px in order to render smoothly.

#if defined(OS_LINUX)
  const float kCursorSize = 63;
  const float kDiameter = 63;
  const float kHotspotOffset = 32;
  const float kHotspotRadius = 0;
  const float kPixelSize = 9;
#else
  const float kCursorSize = 150;
  const float kDiameter = 110;
  const float kHotspotOffset = 25;
  const float kHotspotRadius = 5;
  const float kPixelSize = 10;
#endif

  content::ScreenInfo screen_info;
  host_->GetScreenInfo(&screen_info);
  double device_scale_factor = screen_info.device_scale_factor;

  SkBitmap result;
  result.allocN32Pixels(kCursorSize * device_scale_factor,
                        kCursorSize * device_scale_factor);
  result.eraseARGB(0, 0, 0, 0);

  SkCanvas canvas(result);
  canvas.scale(device_scale_factor, device_scale_factor);
  canvas.translate(0.5f, 0.5f);

  SkPaint paint;

  // Paint original spot with cross.
  if (kHotspotRadius > 0) {
    paint.setStrokeWidth(1);
    paint.setAntiAlias(false);
    paint.setColor(SK_ColorDKGRAY);
    paint.setStyle(SkPaint::kStroke_Style);

    canvas.drawLine(kHotspotOffset, kHotspotOffset - 2 * kHotspotRadius,
                    kHotspotOffset, kHotspotOffset - kHotspotRadius, paint);
    canvas.drawLine(kHotspotOffset, kHotspotOffset + kHotspotRadius,
                    kHotspotOffset, kHotspotOffset + 2 * kHotspotRadius, paint);
    canvas.drawLine(kHotspotOffset - 2 * kHotspotRadius, kHotspotOffset,
                    kHotspotOffset - kHotspotRadius, kHotspotOffset, paint);
    canvas.drawLine(kHotspotOffset + kHotspotRadius, kHotspotOffset,
                    kHotspotOffset + 2 * kHotspotRadius, kHotspotOffset, paint);

    paint.setStrokeWidth(2);
    paint.setAntiAlias(true);
    canvas.drawCircle(kHotspotOffset, kHotspotOffset, kHotspotRadius, paint);
  }

  // Clip circle for magnified projection.
  float padding = (kCursorSize - kDiameter) / 2;
  SkPath clip_path;
  clip_path.addOval(SkRect::MakeXYWH(padding, padding, kDiameter, kDiameter));
  clip_path.close();
  canvas.clipPath(clip_path, SkClipOp::kIntersect, true);

  // Project pixels.
  int pixel_count = kDiameter / kPixelSize;
  SkRect src_rect = SkRect::MakeXYWH(last_cursor_x_ - pixel_count / 2,
                                     last_cursor_y_ - pixel_count / 2,
                                     pixel_count, pixel_count);
  SkRect dst_rect = SkRect::MakeXYWH(padding, padding, kDiameter, kDiameter);
  canvas.drawBitmapRect(frame_, src_rect, dst_rect, NULL);

  // Paint grid.
  paint.setStrokeWidth(1);
  paint.setAntiAlias(false);
  paint.setColor(SK_ColorGRAY);
  for (int i = 0; i < pixel_count; ++i) {
    canvas.drawLine(padding + i * kPixelSize, padding, padding + i * kPixelSize,
                    kCursorSize - padding, paint);
    canvas.drawLine(padding, padding + i * kPixelSize, kCursorSize - padding,
                    padding + i * kPixelSize, paint);
  }

  // Paint central pixel in red.
  SkRect pixel =
      SkRect::MakeXYWH((kCursorSize - kPixelSize) / 2,
                       (kCursorSize - kPixelSize) / 2, kPixelSize, kPixelSize);
  paint.setColor(SK_ColorRED);
  paint.setStyle(SkPaint::kStroke_Style);
  canvas.drawRect(pixel, paint);

  // Paint outline.
  paint.setStrokeWidth(2);
  paint.setColor(SK_ColorDKGRAY);
  paint.setAntiAlias(true);
  canvas.drawCircle(kCursorSize / 2, kCursorSize / 2, kDiameter / 2, paint);

  content::CursorInfo cursor_info;
  cursor_info.type = blink::WebCursorInfo::kTypeCustom;
  cursor_info.image_scale_factor = device_scale_factor;
  cursor_info.custom_image = result;
  cursor_info.hotspot = gfx::Point(kHotspotOffset * device_scale_factor,
                                   kHotspotOffset * device_scale_factor);
  host_->SetCursor(cursor_info);
}

void DevToolsEyeDropper::OnFrameCaptured(
    mojo::ScopedSharedBufferHandle buffer,
    uint32_t buffer_size,
    ::media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& update_rect,
    const gfx::Rect& content_rect,
    viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) {
  gfx::Size view_size = host_->GetView()->GetViewBounds().size();
  if (view_size != content_rect.size()) {
    video_capturer_->SetResolutionConstraints(view_size, view_size, true);
    video_capturer_->RequestRefreshFrame();
    return;
  }

  if (!buffer.is_valid()) {
    callbacks->Done();
    return;
  }
  mojo::ScopedSharedBufferMapping mapping = buffer->Map(buffer_size);
  if (!mapping) {
    DLOG(ERROR) << "Shared memory mapping failed.";
    return;
  }

  SkImageInfo image_info = SkImageInfo::MakeN32(
      content_rect.width(), content_rect.height(), kPremul_SkAlphaType);
  SkPixmap pixmap(image_info, mapping.get(),
                  media::VideoFrame::RowBytes(media::VideoFrame::kARGBPlane,
                                              info->pixel_format,
                                              info->coded_size.width()));
  frame_.installPixels(pixmap);
  shared_memory_mapping_ = std::move(mapping);
  shared_memory_releaser_ = std::move(callbacks);

  UpdateCursor();
}

void DevToolsEyeDropper::OnTargetLost(const viz::FrameSinkId& frame_sink_id) {}

void DevToolsEyeDropper::OnStopped() {}
