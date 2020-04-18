// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_STYLE_TYPOGRAPHY_PROVIDER_H_
#define UI_VIEWS_STYLE_TYPOGRAPHY_PROVIDER_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/font.h"
#include "ui/views/views_export.h"

namespace gfx {
class FontList;
}

namespace views {

class View;

// Provides fonts to use in toolkit-views UI.
class VIEWS_EXPORT TypographyProvider {
 public:
  virtual ~TypographyProvider() = default;

  // Gets the FontList for the given |context| and |style|.
  virtual const gfx::FontList& GetFont(int context, int style) const = 0;

  // Gets the color for the given |context| and |style|. |view| is the View
  // requesting the color.
  virtual SkColor GetColor(const views::View& view,
                           int context,
                           int style) const = 0;

  // Gets the line spacing, or 0 if it should be provided by gfx::FontList.
  virtual int GetLineHeight(int context, int style) const = 0;

  // Returns the weight that will result in the ResourceBundle returning an
  // appropriate "medium" weight for UI. This caters for systems that are known
  // to be unable to provide a system font with weight other than NORMAL or BOLD
  // and for user configurations where the NORMAL font is already BOLD. In both
  // of these cases, NORMAL is returned instead.
  static gfx::Font::Weight MediumWeightForUI();

 protected:
  TypographyProvider() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(TypographyProvider);
};

// The default provider aims to provide values to match pre-Harmony constants
// for the given contexts so that old UI does not change.
class VIEWS_EXPORT DefaultTypographyProvider : public TypographyProvider {
 public:
  DefaultTypographyProvider() = default;

  // TypographyProvider:
  const gfx::FontList& GetFont(int context, int style) const override;
  SkColor GetColor(const views::View& view,
                   int context,
                   int style) const override;
  int GetLineHeight(int context, int style) const override;

  // Sets the |size_delta| and |font_weight| that the the default GetFont()
  // implementation uses. Always sets values, even for styles it doesn't know
  // about.
  static void GetDefaultFont(int context,
                             int style,
                             int* size_delta,
                             gfx::Font::Weight* font_weight);

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultTypographyProvider);
};

}  // namespace views

#endif  // UI_VIEWS_STYLE_TYPOGRAPHY_PROVIDER_H_
