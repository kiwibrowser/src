// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/gtk3_background_painter.h"

#include "ui/gfx/canvas.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace libgtkui {

namespace {

GtkStateFlags ButtonStateToStateFlags(views::Button::ButtonState state) {
  switch (state) {
    case views::Button::STATE_DISABLED:
      return GTK_STATE_FLAG_INSENSITIVE;
    case views::Button::STATE_HOVERED:
      return GTK_STATE_FLAG_PRELIGHT;
    case views::Button::STATE_NORMAL:
      return GTK_STATE_FLAG_NORMAL;
    case views::Button::STATE_PRESSED:
      return static_cast<GtkStateFlags>(GTK_STATE_FLAG_PRELIGHT |
                                        GTK_STATE_FLAG_ACTIVE);
    default:
      NOTREACHED();
      return GTK_STATE_FLAG_NORMAL;
  }
}

}  // namespace

Gtk3BackgroundPainter::Gtk3BackgroundPainter(const views::Button* button,
                                             ScopedStyleContext context)
    : button_(button), context_(std::move(context)) {}

Gtk3BackgroundPainter::~Gtk3BackgroundPainter() {}

void Gtk3BackgroundPainter::Paint(gfx::Canvas* canvas,
                                  views::View* view) const {
  float scale = canvas->image_scale();
  SkBitmap bitmap;
  bitmap.allocN32Pixels(scale * view->width(), scale * view->height());
  bitmap.eraseColor(0);
  CairoSurface surface(bitmap);
  cairo_t* cr = surface.cairo();
  gtk_style_context_set_state(context_, CalculateStateFlags());
  cairo_scale(cr, scale, scale);
  gtk_render_background(context_, cr, 0, 0, view->width(), view->height());
  gtk_render_frame(context_, cr, 0, 0, view->width(), view->height());
  canvas->DrawImageInt(gfx::ImageSkia(gfx::ImageSkiaRep(bitmap, scale)), 0, 0);
}

GtkStateFlags Gtk3BackgroundPainter::CalculateStateFlags() const {
  GtkStateFlags state = ButtonStateToStateFlags(button_->state());
  if (!button_->GetWidget()->IsActive())
    state = static_cast<GtkStateFlags>(state | GTK_STATE_FLAG_BACKDROP);
  return state;
}

}  // namespace libgtkui
