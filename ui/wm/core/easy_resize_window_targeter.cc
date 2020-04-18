// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/easy_resize_window_targeter.h"

#include <algorithm>

#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/transient_window_client.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/insets_f.h"
#include "ui/gfx/geometry/rect.h"

namespace wm {
namespace {

// Returns an insets whose values are all negative or 0. Any positive value is
// forced to 0.
gfx::Insets InsetsWithOnlyNegativeValues(const gfx::Insets& insets) {
  return gfx::Insets(std::min(0, insets.top()), std::min(0, insets.left()),
                     std::min(0, insets.bottom()), std::min(0, insets.right()));
}

gfx::Insets InsetsWithOnlyPositiveValues(const gfx::Insets& insets) {
  return gfx::Insets(std::max(0, insets.top()), std::max(0, insets.left()),
                     std::max(0, insets.bottom()), std::max(0, insets.right()));
}

}  // namespace

// HitMaskSetter is responsible for setting the hit-test mask on a Window.
class EasyResizeWindowTargeter::HitMaskSetter : public aura::WindowObserver {
 public:
  explicit HitMaskSetter(aura::Window* window) : window_(window) {
    window_->AddObserver(this);
  }
  ~HitMaskSetter() override {
    if (window_) {
      aura::WindowPortMus::Get(window_)->SetHitTestMask(base::nullopt);
      window_->RemoveObserver(this);
    }
  }

  void SetHitMaskInsets(const gfx::Insets& insets) {
    if (insets == insets_)
      return;

    insets_ = insets;
    ApplyHitTestMask();
  }

 private:
  void ApplyHitTestMask() {
    base::Optional<gfx::Rect> hit_test_mask(
        gfx::Rect(window_->bounds().size()));
    hit_test_mask->Inset(insets_);
    aura::WindowPortMus::Get(window_)->SetHitTestMask(hit_test_mask);
  }

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override {
    window_->RemoveObserver(this);
    window_ = nullptr;
  }
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override {
    ApplyHitTestMask();
  }

 private:
  aura::Window* window_;
  gfx::Insets insets_;

  DISALLOW_COPY_AND_ASSIGN(HitMaskSetter);
};

EasyResizeWindowTargeter::EasyResizeWindowTargeter(
    aura::Window* container,
    const gfx::Insets& mouse_extend,
    const gfx::Insets& touch_extend)
    : container_(container) {
  DCHECK(container_);
  SetInsets(mouse_extend, touch_extend);
}

EasyResizeWindowTargeter::~EasyResizeWindowTargeter() {}

void EasyResizeWindowTargeter::OnSetInsets(
    const gfx::Insets& last_mouse_extend,
    const gfx::Insets& last_touch_extend) {
  if (aura::Env::GetInstance()->mode() != aura::Env::Mode::MUS)
    return;

  // Mus only accepts 0 or negative values, force all values to fit that.
  const gfx::Insets effective_last_mouse_extend =
      InsetsWithOnlyNegativeValues(last_mouse_extend);
  const gfx::Insets effective_last_touch_extend =
      InsetsWithOnlyNegativeValues(last_touch_extend);
  const gfx::Insets effective_mouse_extend =
      InsetsWithOnlyNegativeValues(mouse_extend());
  const gfx::Insets effective_touch_extend =
      InsetsWithOnlyNegativeValues(touch_extend());
  if (effective_last_touch_extend != effective_touch_extend ||
      effective_last_mouse_extend != effective_mouse_extend) {
    aura::WindowPortMus::Get(container_)
        ->SetExtendedHitRegionForChildren(effective_mouse_extend,
                                          effective_touch_extend);
  }

  // Positive values equate to a hit test mask.
  const gfx::Insets positive_mouse_insets =
      InsetsWithOnlyPositiveValues(mouse_extend());
  if (positive_mouse_insets.IsEmpty()) {
    hit_mask_setter_.reset();
  } else {
    if (!hit_mask_setter_)
      hit_mask_setter_ = std::make_unique<HitMaskSetter>(container_);
    hit_mask_setter_->SetHitMaskInsets(positive_mouse_insets);
  }
}

bool EasyResizeWindowTargeter::EventLocationInsideBounds(
    aura::Window* target,
    const ui::LocatedEvent& event) const {
  return WindowTargeter::EventLocationInsideBounds(target, event);
}

bool EasyResizeWindowTargeter::ShouldUseExtendedBounds(
    const aura::Window* window) const {
  // Use the extended bounds only for immediate child windows of |container_|.
  // Use the default targeter otherwise.
  if (window->parent() != container_)
    return false;

  // Only resizable windows benefit from the extended hit-test region.
  if ((window->GetProperty(aura::client::kResizeBehaviorKey) &
       ui::mojom::kResizeBehaviorCanResize) == 0) {
    return false;
  }

  // For transient children use extended bounds if a transient parent or if
  // transient parent's parent is a top level window in |container_|.
  aura::client::TransientWindowClient* transient_window_client =
      aura::client::GetTransientWindowClient();
  const aura::Window* transient_parent =
      transient_window_client
          ? transient_window_client->GetTransientParent(window)
          : nullptr;
  return !transient_parent || transient_parent == container_ ||
         transient_parent->parent() == container_;
}

}  // namespace wm
