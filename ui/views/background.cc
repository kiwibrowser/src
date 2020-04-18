// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/background.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "build/build_config.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/painter.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

#if defined(OS_WIN)
#include "skia/ext/skia_utils_win.h"
#endif

namespace views {

// SolidBackground is a trivial Background implementation that fills the
// background in a solid color.
class SolidBackground : public Background {
 public:
  explicit SolidBackground(SkColor color) {
    SetNativeControlColor(color);
  }

  void Paint(gfx::Canvas* canvas, View* view) const override {
    // Fill the background. Note that we don't constrain to the bounds as
    // canvas is already clipped for us.
    canvas->DrawColor(get_color());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SolidBackground);
};

// ThemedSolidBackground is a solid background that stays in sync with a view's
// native theme.
class ThemedSolidBackground : public SolidBackground, public ViewObserver {
 public:
  explicit ThemedSolidBackground(View* view, ui::NativeTheme::ColorId color_id)
      : SolidBackground(gfx::kPlaceholderColor),
        observer_(this),
        color_id_(color_id) {
    observer_.Add(view);
    OnViewNativeThemeChanged(view);
  }
  ~ThemedSolidBackground() override {}

  // ViewObserver:
  void OnViewNativeThemeChanged(View* view) override {
    SetNativeControlColor(view->GetNativeTheme()->GetSystemColor(color_id_));
    view->SchedulePaint();
  }
  void OnViewIsDeleting(View* view) override { observer_.Remove(view); }

 private:
  ScopedObserver<View, ViewObserver> observer_;
  ui::NativeTheme::ColorId color_id_;

  DISALLOW_COPY_AND_ASSIGN(ThemedSolidBackground);
};

class BackgroundPainter : public Background {
 public:
  explicit BackgroundPainter(std::unique_ptr<Painter> painter)
      : painter_(std::move(painter)) {
    DCHECK(painter_);
  }

  ~BackgroundPainter() override {}

  void Paint(gfx::Canvas* canvas, View* view) const override {
    Painter::PaintPainterAt(canvas, painter_.get(), view->GetLocalBounds());
  }

 private:
  std::unique_ptr<Painter> painter_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundPainter);
};

Background::Background() : color_(SK_ColorWHITE) {}

Background::~Background() {}

void Background::SetNativeControlColor(SkColor color) {
  color_ = color;
}

std::unique_ptr<Background> CreateSolidBackground(SkColor color) {
  return std::make_unique<SolidBackground>(color);
}

std::unique_ptr<Background> CreateThemedSolidBackground(
    View* view,
    ui::NativeTheme::ColorId color_id) {
  return std::make_unique<ThemedSolidBackground>(view, color_id);
}

std::unique_ptr<Background> CreateStandardPanelBackground() {
  // TODO(beng): Should be in NativeTheme.
  return CreateSolidBackground(SK_ColorWHITE);
}

std::unique_ptr<Background> CreateBackgroundFromPainter(
    std::unique_ptr<Painter> painter) {
  return std::make_unique<BackgroundPainter>(std::move(painter));
}

}  // namespace views
