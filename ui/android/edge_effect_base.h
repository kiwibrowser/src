// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ANDROID_EDGE_EFFECT_BASE_H_
#define UI_ANDROID_EDGE_EFFECT_BASE_H_

#include "base/time/time.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/transform.h"

namespace cc {
class Layer;
}

namespace ui {

// A base class for overscroll-related Android effects.
class EdgeEffectBase {
 public:
  enum State {
    STATE_IDLE = 0,
    STATE_PULL,
    STATE_ABSORB,
    STATE_RECEDE,
    STATE_PULL_DECAY
  };

  enum Edge { EDGE_TOP, EDGE_LEFT, EDGE_BOTTOM, EDGE_RIGHT, EDGE_COUNT };

  virtual ~EdgeEffectBase() {}

  virtual void Pull(base::TimeTicks current_time,
                    float delta_distance,
                    float displacement) = 0;
  virtual void Absorb(base::TimeTicks current_time, float velocity) = 0;
  virtual bool Update(base::TimeTicks current_time) = 0;
  virtual void Release(base::TimeTicks current_time) = 0;

  virtual void Finish() = 0;
  virtual bool IsFinished() const = 0;
  virtual float GetAlpha() const = 0;

  virtual void ApplyToLayers(Edge edge,
                             const gfx::SizeF& viewport_size,
                             float offset) = 0;
  virtual void SetParent(cc::Layer* parent) = 0;

 protected:
  // Computes the transform for an edge effect given the |edge|, |viewport_size|
  // and edge |offset|. This assumes the the effect transform anchor is at the
  // centered edge of the effect.
  static gfx::Transform ComputeTransform(Edge edge,
                                         const gfx::SizeF& viewport_size,
                                         float offset);

  // Computes the maximum effect size relative to the screen |edge|. For
  // top/bottom edges, thsi is simply |viewport_size|, while for left/right
  // edges this is |viewport_size| with coordinates swapped.
  static gfx::SizeF ComputeOrientedSize(Edge edge,
                                        const gfx::SizeF& viewport_size);
};

}  // namespace ui

#endif  // UI_ANDROID_EDGE_EFFECT_BASE_H_
