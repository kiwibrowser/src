// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/demo/window_tree_data.h"

#include "base/time/time.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkRect.h"
#include "ui/aura/mus/window_tree_host_mus.h"
#include "ui/aura/window.h"
#include "ui/aura_extra/image_window_delegate.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"

namespace ui {
namespace demo {

namespace {

// Milliseconds between frames.
const int64_t kFrameDelay = 33;

const SkColor kBgColor = SK_ColorRED;
const SkColor kFgColor = SK_ColorYELLOW;

void DrawSquare(const gfx::Rect& bounds,
                double angle,
                SkCanvas* canvas,
                int size) {
  // Create SkRect to draw centered inside the bounds.
  gfx::Point top_left = bounds.CenterPoint();
  top_left.Offset(-size / 2, -size / 2);
  SkRect rect = SkRect::MakeXYWH(top_left.x(), top_left.y(), size, size);

  // Set SkPaint to fill solid color.
  SkPaint paint;
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(kFgColor);

  // Rotate the canvas.
  const gfx::Size canvas_size = bounds.size();
  if (angle != 0.0) {
    canvas->translate(SkFloatToScalar(canvas_size.width() * 0.5f),
                      SkFloatToScalar(canvas_size.height() * 0.5f));
    canvas->rotate(angle);
    canvas->translate(-SkFloatToScalar(canvas_size.width() * 0.5f),
                      -SkFloatToScalar(canvas_size.height() * 0.5f));
  }

  canvas->drawRect(rect, paint);
}

}  // namespace

WindowTreeData::WindowTreeData(int square_size) : square_size_(square_size) {}

WindowTreeData::~WindowTreeData() {}

aura::Window* WindowTreeData::bitmap_window() {
  DCHECK(!window_tree_host_->window()->children().empty());
  return window_tree_host_->window()->children()[0];
}

void WindowTreeData::Init(
    std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) {
  window_tree_host->Show();
  // Take ownership of the WTH.
  SetWindowTreeHost(std::move(window_tree_host));

  // Initialize the window for the bitmap.
  window_delegate_ = new aura_extra::ImageWindowDelegate();
  aura::Window* root_window = window_tree_host_->window();
  aura::Window* bitmap_window = new aura::Window(window_delegate_);
  bitmap_window->Init(LAYER_TEXTURED);
  bitmap_window->SetBounds(gfx::Rect(root_window->bounds().size()));
  bitmap_window->Show();
  bitmap_window->SetName("Bitmap");
  root_window->AddChild(bitmap_window);

  // Draw initial frame and start the timer to regularly draw frames.
  DrawFrame();
  timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(kFrameDelay),
               base::Bind(&WindowTreeData::DrawFrame, base::Unretained(this)));
}

void WindowTreeData::DrawFrame() {
  angle_ += 2.0;
  if (angle_ >= 360.0)
    angle_ = 0.0;

  const gfx::Rect& bounds = bitmap_window()->bounds();

  // Allocate a bitmap of correct size.
  SkBitmap bitmap;
  SkImageInfo image_info = SkImageInfo::MakeN32(bounds.width(), bounds.height(),
                                                kPremul_SkAlphaType);
  bitmap.allocPixels(image_info);

  // Draw the rotated square on background in bitmap.
  SkCanvas canvas(bitmap);
  canvas.clear(kBgColor);
  // TODO(kylechar): Add GL drawing instead of software rasterization in future.
  DrawSquare(bounds, angle_, &canvas, square_size_);
  canvas.flush();

  gfx::ImageSkiaRep image_skia_rep(bitmap, 1);
  gfx::ImageSkia image_skia(image_skia_rep);
  gfx::Image image(image_skia);

  window_delegate_->SetImage(image);
  bitmap_window()->SchedulePaintInRect(gfx::Rect(bounds.size()));
}

}  // namespace demo
}  // namespace ui
