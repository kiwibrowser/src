// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/painter.h"

#include "base/logging.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/insets_f.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/nine_image_painter.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/view.h"

namespace views {

namespace {

// SolidRoundRectPainter -------------------------------------------------------

// Creates a round rect painter with a 1 pixel border. The border paints on top
// of the background.
class SolidRoundRectPainter : public Painter {
 public:
  SolidRoundRectPainter(SkColor bg_color, SkColor stroke_color, float radius);
  ~SolidRoundRectPainter() override;

  // Painter:
  gfx::Size GetMinimumSize() const override;
  void Paint(gfx::Canvas* canvas, const gfx::Size& size) override;

 private:
  const SkColor bg_color_;
  const SkColor stroke_color_;
  const float radius_;

  DISALLOW_COPY_AND_ASSIGN(SolidRoundRectPainter);
};

SolidRoundRectPainter::SolidRoundRectPainter(SkColor bg_color,
                                             SkColor stroke_color,
                                             float radius)
    : bg_color_(bg_color), stroke_color_(stroke_color), radius_(radius) {}

SolidRoundRectPainter::~SolidRoundRectPainter() {}

gfx::Size SolidRoundRectPainter::GetMinimumSize() const {
  return gfx::Size();
}

void SolidRoundRectPainter::Paint(gfx::Canvas* canvas, const gfx::Size& size) {
  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();

  gfx::RectF border_rect_f(gfx::ScaleToEnclosingRect(gfx::Rect(size), scale));
  const SkScalar scaled_corner_radius = SkFloatToScalar(radius_ * scale);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(bg_color_);
  canvas->DrawRoundRect(border_rect_f, scaled_corner_radius, flags);

  border_rect_f.Inset(gfx::InsetsF(0.5f));
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1);
  flags.setColor(stroke_color_);
  canvas->DrawRoundRect(border_rect_f, scaled_corner_radius, flags);
}

// DashedFocusPainter ----------------------------------------------------------

class DashedFocusPainter : public Painter {
 public:
  explicit DashedFocusPainter(const gfx::Insets& insets);
  ~DashedFocusPainter() override;

  // Painter:
  gfx::Size GetMinimumSize() const override;
  void Paint(gfx::Canvas* canvas, const gfx::Size& size) override;

 private:
  const gfx::Insets insets_;

  DISALLOW_COPY_AND_ASSIGN(DashedFocusPainter);
};

DashedFocusPainter::DashedFocusPainter(const gfx::Insets& insets)
    : insets_(insets) {
}

DashedFocusPainter::~DashedFocusPainter() {
}

gfx::Size DashedFocusPainter::GetMinimumSize() const {
  return gfx::Size();
}

void DashedFocusPainter::Paint(gfx::Canvas* canvas, const gfx::Size& size) {
  gfx::Rect rect(size);
  rect.Inset(insets_);
  canvas->DrawFocusRect(rect);
}

// SolidFocusPainter -----------------------------------------------------------

class SolidFocusPainter : public Painter {
 public:
  SolidFocusPainter(SkColor color, int thickness, const gfx::InsetsF& insets);
  ~SolidFocusPainter() override;

  // Painter:
  gfx::Size GetMinimumSize() const override;
  void Paint(gfx::Canvas* canvas, const gfx::Size& size) override;

 private:
  const SkColor color_;
  const int thickness_;
  const gfx::InsetsF insets_;

  DISALLOW_COPY_AND_ASSIGN(SolidFocusPainter);
};

SolidFocusPainter::SolidFocusPainter(SkColor color,
                                     int thickness,
                                     const gfx::InsetsF& insets)
    : color_(color), thickness_(thickness), insets_(insets) {}

SolidFocusPainter::~SolidFocusPainter() {
}

gfx::Size SolidFocusPainter::GetMinimumSize() const {
  return gfx::Size();
}

void SolidFocusPainter::Paint(gfx::Canvas* canvas, const gfx::Size& size) {
  gfx::RectF rect((gfx::Rect(size)));
  rect.Inset(insets_);
  canvas->DrawSolidFocusRect(rect, color_, thickness_);
}

// ImagePainter ---------------------------------------------------------------

// ImagePainter stores and paints nine images as a scalable grid.
class ImagePainter : public Painter {
 public:
  // Constructs an ImagePainter with the specified image resource ids.
  // See CreateImageGridPainter()'s comment regarding image ID count and order.
  explicit ImagePainter(const int image_ids[]);

  // Constructs an ImagePainter with the specified image and insets.
  ImagePainter(const gfx::ImageSkia& image, const gfx::Insets& insets);

  ~ImagePainter() override;

  // Painter:
  gfx::Size GetMinimumSize() const override;
  void Paint(gfx::Canvas* canvas, const gfx::Size& size) override;

 private:
  std::unique_ptr<gfx::NineImagePainter> nine_painter_;

  DISALLOW_COPY_AND_ASSIGN(ImagePainter);
};

ImagePainter::ImagePainter(const int image_ids[])
    : nine_painter_(ui::CreateNineImagePainter(image_ids)) {
}

ImagePainter::ImagePainter(const gfx::ImageSkia& image,
                           const gfx::Insets& insets)
    : nine_painter_(new gfx::NineImagePainter(image, insets)) {
}

ImagePainter::~ImagePainter() {
}

gfx::Size ImagePainter::GetMinimumSize() const {
  return nine_painter_->GetMinimumSize();
}

void ImagePainter::Paint(gfx::Canvas* canvas, const gfx::Size& size) {
  nine_painter_->Paint(canvas, gfx::Rect(size));
}

class PaintedLayer : public ui::LayerOwner, public ui::LayerDelegate {
 public:
  explicit PaintedLayer(std::unique_ptr<Painter> painter);
  ~PaintedLayer() override;

  // LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;

 private:
  std::unique_ptr<Painter> painter_;

  DISALLOW_COPY_AND_ASSIGN(PaintedLayer);
};

PaintedLayer::PaintedLayer(std::unique_ptr<Painter> painter)
    : painter_(std::move(painter)) {
  SetLayer(std::make_unique<ui::Layer>(ui::LAYER_TEXTURED));
  layer()->set_delegate(this);
}

PaintedLayer::~PaintedLayer() {}

void PaintedLayer::OnPaintLayer(const ui::PaintContext& context) {
  ui::PaintRecorder recorder(context, layer()->size());
  painter_->Paint(recorder.canvas(), layer()->size());
}

void PaintedLayer::OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                              float new_device_scale_factor) {}

}  // namespace


// Painter --------------------------------------------------------------------

Painter::Painter() {
}

Painter::~Painter() {
}

// static
void Painter::PaintPainterAt(gfx::Canvas* canvas,
                             Painter* painter,
                             const gfx::Rect& rect) {
  DCHECK(canvas);
  DCHECK(painter);
  canvas->Save();
  canvas->Translate(rect.OffsetFromOrigin());
  painter->Paint(canvas, rect.size());
  canvas->Restore();
}

// static
void Painter::PaintFocusPainter(View* view,
                                gfx::Canvas* canvas,
                                Painter* focus_painter) {
  if (focus_painter && view->HasFocus())
    PaintPainterAt(canvas, focus_painter, view->GetLocalBounds());
}

// static
std::unique_ptr<Painter> Painter::CreateSolidRoundRectPainter(SkColor color,
                                                              float radius) {
  return std::make_unique<SolidRoundRectPainter>(color, SK_ColorTRANSPARENT,
                                                 radius);
}

// static
std::unique_ptr<Painter> Painter::CreateRoundRectWith1PxBorderPainter(
    SkColor bg_color,
    SkColor stroke_color,
    float radius) {
  return std::make_unique<SolidRoundRectPainter>(bg_color, stroke_color,
                                                 radius);
}

// static
std::unique_ptr<Painter> Painter::CreateImagePainter(
    const gfx::ImageSkia& image,
    const gfx::Insets& insets) {
  return std::make_unique<ImagePainter>(image, insets);
}

// static
std::unique_ptr<Painter> Painter::CreateImageGridPainter(
    const int image_ids[]) {
  return std::make_unique<ImagePainter>(image_ids);
}

// static
std::unique_ptr<Painter> Painter::CreateDashedFocusPainter() {
  return std::make_unique<DashedFocusPainter>(gfx::Insets());
}

// static
std::unique_ptr<Painter> Painter::CreateDashedFocusPainterWithInsets(
    const gfx::Insets& insets) {
  return std::make_unique<DashedFocusPainter>(insets);
}

// static
std::unique_ptr<Painter> Painter::CreateSolidFocusPainter(
    SkColor color,
    const gfx::Insets& insets) {
  // Before Canvas::DrawSolidFocusRect correctly inset the rect's bounds based
  // on the thickness, callers had to add 1 to the bottom and right insets.
  // Subtract that here so it works the same way with the new
  // Canvas::DrawSolidFocusRect.
  const gfx::Insets corrected_insets = insets - gfx::Insets(0, 0, 1, 1);
  return std::make_unique<SolidFocusPainter>(color, 1, corrected_insets);
}

// static
std::unique_ptr<Painter> Painter::CreateSolidFocusPainter(
    SkColor color,
    int thickness,
    const gfx::InsetsF& insets) {
  return std::make_unique<SolidFocusPainter>(color, thickness, insets);
}

// static
std::unique_ptr<ui::LayerOwner> Painter::CreatePaintedLayer(
    std::unique_ptr<Painter> painter) {
  return std::make_unique<PaintedLayer>(std::move(painter));
}

}  // namespace views
