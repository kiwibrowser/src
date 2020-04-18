// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ANDROID_EDGE_EFFECT_H_
#define UI_ANDROID_EDGE_EFFECT_H_

#include <memory>

#include "base/macros.h"
#include "ui/android/edge_effect_base.h"
#include "ui/android/ui_android_export.h"

namespace cc {
class Layer;
}

namespace ui {
class ResourceManager;
}

namespace ui {

// |EdgeEffect| mirrors its Android counterpart, EdgeEffect.java.
// Conscious tradeoffs were made to align this as closely as possible with the
// the original Android java version.
// All coordinates and dimensions are in device pixels.
class UI_ANDROID_EXPORT EdgeEffect : public EdgeEffectBase {
 public:
  explicit EdgeEffect(ui::ResourceManager* resource_manager,
                      float device_scale_factor);
  ~EdgeEffect() override;

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
  class EffectLayer;
  std::unique_ptr<EffectLayer> edge_;
  std::unique_ptr<EffectLayer> glow_;

  float base_edge_height_;
  float base_glow_height_;

  float edge_alpha_;
  float edge_scale_y_;
  float glow_alpha_;
  float glow_scale_y_;

  float edge_alpha_start_;
  float edge_alpha_finish_;
  float edge_scale_y_start_;
  float edge_scale_y_finish_;
  float glow_alpha_start_;
  float glow_alpha_finish_;
  float glow_scale_y_start_;
  float glow_scale_y_finish_;

  base::TimeTicks start_time_;
  base::TimeDelta duration_;

  State state_;

  float pull_distance_;

  DISALLOW_COPY_AND_ASSIGN(EdgeEffect);
};

}  // namespace ui

#endif  // UI_ANDROID_EDGE_EFFECT_H_
