// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_SHADOW_BORDER_H_
#define UI_VIEWS_SHADOW_BORDER_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/border.h"
#include "ui/views/views_export.h"

namespace views {

// Creates a css box-shadow like border which fades into SK_ColorTRANSPARENT.
class VIEWS_EXPORT ShadowBorder : public views::Border {
 public:
  explicit ShadowBorder(const gfx::ShadowValue& shadow);
  ~ShadowBorder() override;

 protected:
  // Overridden from views::Border:
  void Paint(const views::View& view, gfx::Canvas* canvas) override;
  gfx::Insets GetInsets() const override;
  gfx::Size GetMinimumSize() const override;

 private:
  // The shadow value to use for this border.
  const gfx::ShadowValue shadow_value_;

  // The insets of this border.
  const gfx::Insets insets_;

  DISALLOW_COPY_AND_ASSIGN(ShadowBorder);
};

}  // namespace views

#endif  // UI_VIEWS_SHADOW_BORDER_H_
