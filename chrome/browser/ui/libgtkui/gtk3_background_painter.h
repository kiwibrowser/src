// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_GTK3_BACKGROUND_PAINTER_H_
#define CHROME_BROWSER_UI_LIBGTKUI_GTK3_BACKGROUND_PAINTER_H_

#include "base/macros.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "ui/views/background.h"

namespace views {
class Button;
}

namespace libgtkui {

// A background that paints a button using GTK foreign drawing.  The
// type and style of widget to be drawn is decided by a
// GtkStyleContext.  Always renders a background and a frame.
class Gtk3BackgroundPainter : public views::Background {
 public:
  Gtk3BackgroundPainter(const views::Button* button,
                        ScopedStyleContext context);
  ~Gtk3BackgroundPainter() override;

  void Paint(gfx::Canvas* canvas, views::View* view) const override;

 private:
  GtkStateFlags CalculateStateFlags() const;

  const views::Button* button_;
  mutable ScopedStyleContext context_;

  DISALLOW_COPY_AND_ASSIGN(Gtk3BackgroundPainter);
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_GTK3_BACKGROUND_PAINTER_H_
