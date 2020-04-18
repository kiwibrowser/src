// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/shadow_border.h"

#include "third_party/skia/include/core/SkDrawLooper.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/view.h"

namespace views {

namespace {

gfx::Insets GetInsetsFromShadowValue(const gfx::ShadowValue& shadow) {
  std::vector<gfx::ShadowValue> shadows;
  shadows.push_back(shadow);
  return -gfx::ShadowValue::GetMargin(shadows);
}

}  // namespace

ShadowBorder::ShadowBorder(const gfx::ShadowValue& shadow)
    : views::Border(),
      shadow_value_(shadow),
      insets_(GetInsetsFromShadowValue(shadow)) {
}

ShadowBorder::~ShadowBorder() {
}

// TODO(sidharthms): Re-painting a shadow looper on every paint call may yield
// poor performance. Ideally we should be caching the border to bitmaps.
void ShadowBorder::Paint(const views::View& view, gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  std::vector<gfx::ShadowValue> shadows;
  shadows.push_back(shadow_value_);
  flags.setLooper(gfx::CreateShadowDrawLooper(shadows));
  flags.setColor(SK_ColorTRANSPARENT);
  flags.setStrokeJoin(cc::PaintFlags::kRound_Join);
  gfx::Rect bounds(view.size());
  bounds.Inset(-gfx::ShadowValue::GetMargin(shadows));
  canvas->DrawRect(bounds, flags);
}

gfx::Insets ShadowBorder::GetInsets() const {
  return insets_;
}

gfx::Size ShadowBorder::GetMinimumSize() const {
  return gfx::Size(shadow_value_.blur(), shadow_value_.blur());
}

}  // namespace views
