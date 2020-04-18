// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_RESIZE_SHADOW_H_
#define ASH_WM_RESIZE_SHADOW_H_

#include <memory>

#include "base/macros.h"
#include "ui/aura/window_observer.h"

namespace aura {
class Window;
}
namespace ui {
class Layer;
}

namespace ash {

// A class to render the resize edge effect when the user moves their mouse
// over a sizing edge.  This is just a visual effect; the actual resize is
// handled by the EventFilter.
class ResizeShadow : public aura::WindowObserver {
 public:
  explicit ResizeShadow(aura::Window* window);
  ~ResizeShadow() override;

  // aura::WindowObserver:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;
  void OnWindowHierarchyChanged(const HierarchyChangeParams& params) override;
  void OnWindowStackingChanged(aura::Window* window) override;

  // Shows resize effects for one or more edges based on a |hit_test| code, such
  // as HTRIGHT or HTBOTTOMRIGHT.
  void ShowForHitTest(int hit_test);

  // Hides all resize effects.
  void Hide();

  int GetLastHitTestForTest() const { return last_hit_test_; }
  const ui::Layer* GetLayerForTest() const { return layer_.get(); }

 private:
  // Reparents |layer_| so that it's behind the layer of |window_|.
  void ReparentLayer();

  // Updates bounds and visibility of |layer_|.
  void UpdateBoundsAndVisibility();

  aura::Window* window_;

  // The layer to which the shadow is drawn. The layer is stacked beneath the
  // layer of |window_|.
  std::unique_ptr<ui::Layer> layer_;

  // Hit test value from last call to ShowForHitTest().  Used to prevent
  // repeatedly triggering the same animations for the same hit.
  int last_hit_test_;

  DISALLOW_COPY_AND_ASSIGN(ResizeShadow);
};

}  // namespace ash

#endif  // ASH_WM_RESIZE_SHADOW_H_
