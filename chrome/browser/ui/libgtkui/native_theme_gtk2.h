// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_NATIVE_THEME_GTK2_H_
#define CHROME_BROWSER_UI_LIBGTKUI_NATIVE_THEME_GTK2_H_

#include "base/macros.h"
#include "ui/native_theme/native_theme_base.h"

typedef struct _GtkWidget GtkWidget;

namespace libgtkui {

// A version of NativeTheme that uses GTK2 supplied colours instead of the
// default aura colours. Analogue to NativeThemeWin, except that can't be
// compiled into the main chrome binary like the Windows code can.
class NativeThemeGtk2 : public ui::NativeThemeBase {
 public:
  static NativeThemeGtk2* instance();

  // Overridden from ui::NativeThemeBase:
  SkColor GetSystemColor(ColorId color_id) const override;
  void PaintMenuPopupBackground(
      cc::PaintCanvas* canvas,
      const gfx::Size& size,
      const MenuBackgroundExtraParams& menu_background) const override;
  void PaintMenuItemBackground(
      cc::PaintCanvas* canvas,
      State state,
      const gfx::Rect& rect,
      const MenuItemExtraParams& menu_item) const override;

 private:
  NativeThemeGtk2();
  ~NativeThemeGtk2() override;

  // Returns various widgets for theming use.
  GtkWidget* GetWindow() const;
  GtkWidget* GetEntry() const;
  GtkWidget* GetLabel() const;
  GtkWidget* GetButton() const;
  GtkWidget* GetBlueButton() const;
  GtkWidget* GetTree() const;
  GtkWidget* GetTooltip() const;
  GtkWidget* GetMenu() const;
  GtkWidget* GetMenuItem() const;
  GtkWidget* GetSeparator() const;

  DISALLOW_COPY_AND_ASSIGN(NativeThemeGtk2);
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_NATIVE_THEME_GTK2_H_
