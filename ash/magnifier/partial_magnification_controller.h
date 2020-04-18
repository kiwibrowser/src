// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MAGNIFIER_PARTIAL_MAGNIFICATION_CONTROLLER_H_
#define ASH_MAGNIFIER_PARTIAL_MAGNIFICATION_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"
#include "ui/events/event_handler.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget_observer.h"

namespace ui {
class Layer;
class LocatedEvent;
struct PointerDetails;
}

namespace ash {

// Controls the partial screen magnifier, which is a small area of the screen
// which is zoomed in.  The zoomed area follows the mouse cursor when enabled.
class ASH_EXPORT PartialMagnificationController : public ui::EventHandler,
                                                  public aura::WindowObserver,
                                                  public views::WidgetObserver {
 public:
  PartialMagnificationController();
  ~PartialMagnificationController() override;

  // Turns the partial screen magnifier feature on or off. Turning the magnifier
  // on does not imply that it will be displayed; the magnifier is only
  // displayed when it is both enabled and active.
  void SetEnabled(bool enabled);

  // Switch PartialMagnified RootWindow to |new_root_window|. This does
  // following:
  //  - Remove the magnifier from the current root window.
  //  - Create a magnifier in the new root_window |new_root_window|.
  //  - Switch the target window from current window to |new_root_window|.
  void SwitchTargetRootWindowIfNeeded(aura::Window* new_root_window);

 private:
  friend class PartialMagnificationControllerTestApi;

  class BorderRenderer;
  class ContentMask;

  // ui::EventHandler:
  void OnTouchEvent(ui::TouchEvent* event) override;

  // WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  // WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  // Enables or disables the actual magnifier window.
  void SetActive(bool active);

  // Contains common logic between OnMouseEvent and OnTouchEvent.
  void OnLocatedEvent(ui::LocatedEvent* event,
                      const ui::PointerDetails& pointer_details);

  // Create or close the magnifier window.
  void CreateMagnifierWindow(aura::Window* root_window);
  void CloseMagnifierWindow();

  // Removes this as an observer of the zoom widget and the root window.
  void RemoveZoomWidgetObservers();

  bool is_enabled_ = false;
  bool is_active_ = false;

  // The host widget is the root parent for all of the layers. The widget's
  // location follows the mouse, which causes the layers to also move.
  views::Widget* host_widget_ = nullptr;

  // Draws the background with a zoom filter applied.
  std::unique_ptr<ui::Layer> zoom_layer_;
  // Draws an outline that is overlayed on top of |zoom_layer_|.
  std::unique_ptr<ui::Layer> border_layer_;
  // Draws a multicolored black/white/black border on top of |border_layer_|.
  // Also draws a shadow around the border. This must be ordered after
  // |border_layer_| so that it gets destroyed after |border_layer_|, otherwise
  // |border_layer_| will have a pointer to a deleted delegate.
  std::unique_ptr<BorderRenderer> border_renderer_;
  // Masks the content of |zoom_layer_| so that only a circle is magnified.
  std::unique_ptr<ContentMask> zoom_mask_;
  // Masks the content of |border_layer_| so that only a circle outline is
  // drawn.
  std::unique_ptr<ContentMask> border_mask_;

  DISALLOW_COPY_AND_ASSIGN(PartialMagnificationController);
};

}  // namespace ash

#endif  // ASH_MAGNIFIER_PARTIAL_MAGNIFICATION_CONTROLLER_H_
