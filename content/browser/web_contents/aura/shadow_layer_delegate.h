// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_AURA_SHADOW_LAYER_DELEGATE_H_
#define CONTENT_BROWSER_WEB_CONTENTS_AURA_SHADOW_LAYER_DELEGATE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/compositor/layer_delegate.h"

namespace ui {
class Layer;
}

namespace content {

// ShadowLayerDelegate takes care of drawing a shadow on the left edge of
// another layer.
class ShadowLayerDelegate : public ui::LayerDelegate {
 public:
  explicit ShadowLayerDelegate(ui::Layer* shadow_for);
  ~ShadowLayerDelegate() override;

  // Returns the layer for the shadow. Note that the ShadowLayerDelegate owns
  // the layer, and the layer is destroyed when the delegate is destroyed.
  ui::Layer* layer() { return layer_.get(); }

 private:
  // Overridden from ui::LayerDelegate:
  void OnPaintLayer(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;

  std::unique_ptr<ui::Layer> layer_;

  DISALLOW_COPY_AND_ASSIGN(ShadowLayerDelegate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_AURA_SHADOW_LAYER_DELEGATE_H_
