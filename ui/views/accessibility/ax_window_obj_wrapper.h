// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ACCESSIBILITY_AX_WINDOW_OBJ_WRAPPER_H_
#define UI_VIEWS_ACCESSIBILITY_AX_WINDOW_OBJ_WRAPPER_H_

#include <stdint.h>

#include "base/macros.h"
#include "ui/accessibility/platform/ax_unique_id.h"
#include "ui/aura/window_observer.h"
#include "ui/views/accessibility/ax_aura_obj_wrapper.h"

namespace aura {
class Window;
}  // namespace aura

namespace views {

// Describes a |Window| for use with other AX classes.
class AXWindowObjWrapper : public AXAuraObjWrapper,
                           public aura::WindowObserver {
 public:
  explicit AXWindowObjWrapper(aura::Window* window);
  ~AXWindowObjWrapper() override;

  // Whether this window is an alert window.
  bool is_alert() { return is_alert_; }

  // Sets whether this window is an alert window.
  void set_is_alert(bool is_alert) { is_alert_ = is_alert; }

  // AXAuraObjWrapper overrides.
  AXAuraObjWrapper* GetParent() override;
  void GetChildren(std::vector<AXAuraObjWrapper*>* out_children) override;
  void Serialize(ui::AXNodeData* out_node_data) override;
  const ui::AXUniqueId& GetUniqueId() const final;

  // WindowObserver overrides.
  void OnWindowDestroyed(aura::Window* window) override;
  void OnWindowDestroying(aura::Window* window) override;
  void OnWindowHierarchyChanged(const HierarchyChangeParams& params) override;
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;
  void OnWindowPropertyChanged(aura::Window* window,
                               const void* key,
                               intptr_t old) override;
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override;

 private:
  aura::Window* window_;

  bool is_alert_;

  bool is_root_window_;

  const ui::AXUniqueId unique_id_;

  DISALLOW_COPY_AND_ASSIGN(AXWindowObjWrapper);
};

}  // namespace views

#endif  // UI_VIEWS_ACCESSIBILITY_AX_WINDOW_OBJ_WRAPPER_H_
