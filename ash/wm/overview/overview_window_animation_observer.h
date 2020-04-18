// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_OVERVIEW_WINDOW_ANIMATION_OBSERVER_H_
#define ASH_WM_OVERVIEW_OVERVIEW_WINDOW_ANIMATION_OBSERVER_H_

#include <map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/layer_observer.h"

namespace ash {

// An observer sets transforms of a list of overview windows when the observed
// animation is complete. Currently only the exit animation is observed, because
// the windows beneath the first window covering the available workspace need to
// defer SetTransform until the first window completes its animation.
class OverviewWindowAnimationObserver : public ui::ImplicitAnimationObserver,
                                        public ui::LayerObserver {
 public:
  explicit OverviewWindowAnimationObserver();
  ~OverviewWindowAnimationObserver() override;

  // ui::ImplicitAnimationObserver:
  void OnImplicitAnimationsCompleted() override;

  // ui::LayerObserver overrides:
  void LayerDestroyed(ui::Layer* layer) override;

  void AddLayerTransformPair(ui::Layer* layer, const gfx::Transform& transform);

  base::WeakPtr<OverviewWindowAnimationObserver> GetWeakPtr();

 private:
  // Stores the windows' layers and corresponding transforms.
  std::map<ui::Layer*, gfx::Transform> layer_transform_map_;

  base::WeakPtrFactory<OverviewWindowAnimationObserver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OverviewWindowAnimationObserver);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_OVERVIEW_WINDOW_ANIMATION_OBSERVER_H_
