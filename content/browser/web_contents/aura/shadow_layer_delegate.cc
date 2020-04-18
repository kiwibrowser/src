// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/aura/shadow_layer_delegate.h"

#include "base/macros.h"
#include "cc/paint/paint_shader.h"
#include "third_party/skia/include/effects/SkGradientShader.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/skia_util.h"

namespace {

const SkColor kShadowLightColor = SkColorSetARGB(0x0, 0, 0, 0);
const SkColor kShadowDarkColor = SkColorSetARGB(0x70, 0, 0, 0);
const int kShadowThick = 7;

}  // namespace

namespace content {

ShadowLayerDelegate::ShadowLayerDelegate(ui::Layer* shadow_for)
    : layer_(new ui::Layer(ui::LAYER_TEXTURED)) {
  layer_->set_delegate(this);
  layer_->SetBounds(gfx::Rect(-kShadowThick, 0, kShadowThick,
                              shadow_for->bounds().height()));
  layer_->SetFillsBoundsOpaquely(false);
  shadow_for->Add(layer_.get());
}

ShadowLayerDelegate::~ShadowLayerDelegate() {
}

void ShadowLayerDelegate::OnPaintLayer(const ui::PaintContext& context) {
  SkPoint points[2];
  const SkColor kShadowColors[2] = { kShadowLightColor, kShadowDarkColor };

  points[0].iset(0, 0);
  points[1].iset(kShadowThick, 0);

  gfx::Rect paint_rect = gfx::Rect(0, 0, kShadowThick,
                                   layer_->bounds().height());
  cc::PaintFlags flags;
  flags.setShader(cc::PaintShader::MakeLinearGradient(
      points, kShadowColors, nullptr, arraysize(points),
      SkShader::kRepeat_TileMode));
  ui::PaintRecorder recorder(context, layer_->size());
  recorder.canvas()->DrawRect(paint_rect, flags);
}

void ShadowLayerDelegate::OnDeviceScaleFactorChanged(
    float old_device_scale_factor,
    float new_device_scale_factor) {}

}  // namespace content
