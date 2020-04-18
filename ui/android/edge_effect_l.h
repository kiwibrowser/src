// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ANDROID_EDGE_EFFECT_L_H_
#define UI_ANDROID_EDGE_EFFECT_L_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "ui/android/edge_effect_base.h"
#include "ui/android/ui_android_export.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"

namespace cc {
class Layer;
class UIResourceLayer;
}

namespace ui {
class ResourceManager;
}

namespace ui {

// |EdgeEffectL| mirrors its Android L counterpart, EdgeEffect.java.
// Conscious tradeoffs were made to align this as closely as possible with the
// the original Android java version.
// All coordinates and dimensions are in device pixels.
class UI_ANDROID_EXPORT EdgeEffectL : public EdgeEffectBase {
 public:
  explicit EdgeEffectL(ui::ResourceManager* resource_manager);
  ~EdgeEffectL() override;

  void Pull(base::TimeTicks current_time,
            float delta_distance,
            float displacement) override;
  void Absorb(base::TimeTicks current_time, float velocity) override;
  bool Update(base::TimeTicks current_time) override;
  void Release(base::TimeTicks current_time) override;

  void Finish() override;
  bool IsFinished() const override;
  float GetAlpha() const override;

  void ApplyToLayers(Edge edge,
                     const gfx::SizeF& viewport_size,
                     float offset) override;
  void SetParent(cc::Layer* parent) override;

  // Thread-safe trigger to load resources.
  static void PreloadResources(ui::ResourceManager* resource_manager);

 private:
  ui::ResourceManager* const resource_manager_;

  scoped_refptr<cc::UIResourceLayer> glow_;

  float glow_alpha_;
  float glow_scale_y_;

  float glow_alpha_start_;
  float glow_alpha_finish_;
  float glow_scale_y_start_;
  float glow_scale_y_finish_;

  gfx::RectF arc_rect_;
  gfx::Size bounds_;
  float displacement_;
  float target_displacement_;

  base::TimeTicks start_time_;
  base::TimeDelta duration_;

  State state_;

  float pull_distance_;

  DISALLOW_COPY_AND_ASSIGN(EdgeEffectL);
};

}  // namespace ui

#endif  // UI_ANDROID_EDGE_EFFECT_L_H_
