// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_NATIVE_THEME_GTK3_H_
#define CHROME_BROWSER_UI_LIBGTKUI_NATIVE_THEME_GTK3_H_

#include "base/macros.h"
#include "base/optional.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/base/glib/scoped_gobject.h"
#include "ui/native_theme/native_theme_base.h"

typedef struct _GtkCssProvider GtkCssProvider;
typedef struct _GtkParamSpec GtkParamSpec;
typedef struct _GtkSettings GtkSettings;

namespace libgtkui {

using ScopedCssProvider = ScopedGObject<GtkCssProvider>;

// A version of NativeTheme that uses GTK3-rendered widgets.
class NativeThemeGtk3 : public ui::NativeThemeBase {
 public:
  static NativeThemeGtk3* instance();

  // Overridden from ui::NativeThemeBase:
  SkColor GetSystemColor(ColorId color_id) const override;
  void PaintArrowButton(cc::PaintCanvas* canvas,
                        const gfx::Rect& rect,
                        Part direction,
                        State state) const override;
  void PaintScrollbarTrack(cc::PaintCanvas* canvas,
                           Part part,
                           State state,
                           const ScrollbarTrackExtraParams& extra_params,
                           const gfx::Rect& rect) const override;
  void PaintScrollbarThumb(
      cc::PaintCanvas* canvas,
      Part part,
      State state,
      const gfx::Rect& rect,
      NativeTheme::ScrollbarOverlayColorTheme theme) const override;
  void PaintScrollbarCorner(cc::PaintCanvas* canvas,
                            State state,
                            const gfx::Rect& rect) const override;
  void PaintMenuPopupBackground(
      cc::PaintCanvas* canvas,
      const gfx::Size& size,
      const MenuBackgroundExtraParams& menu_background) const override;
  void PaintMenuSeparator(
      cc::PaintCanvas* canvas,
      State state,
      const gfx::Rect& rect,
      const MenuSeparatorExtraParams& menu_separator) const override;
  void PaintMenuItemBackground(
      cc::PaintCanvas* canvas,
      State state,
      const gfx::Rect& rect,
      const MenuItemExtraParams& menu_item) const override;
  void PaintFrameTopArea(
      cc::PaintCanvas* canvas,
      State state,
      const gfx::Rect& rect,
      const FrameTopAreaExtraParams& frame_top_area) const override;

 private:
  NativeThemeGtk3();
  ~NativeThemeGtk3() override;

  void SetThemeCssOverride(ScopedCssProvider provider);

  CHROMEG_CALLBACK_1(NativeThemeGtk3,
                     void,
                     OnThemeChanged,
                     GtkSettings*,
                     GtkParamSpec*);

  mutable base::Optional<SkColor> color_cache_[kColorId_NumColors];

  ScopedCssProvider theme_css_override_;

  DISALLOW_COPY_AND_ASSIGN(NativeThemeGtk3);
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_NATIVE_THEME_GTK3_H_
