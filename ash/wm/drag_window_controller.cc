// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/drag_window_controller.h"

#include <algorithm>
#include <memory>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_tree_owner.h"
#include "ui/compositor/paint_context.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/display/display.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_util.h"

namespace ash {

// This keeps track of the drag window's state. It creates/destroys/updates
// bounds and opacity based on the current bounds.
class DragWindowController::DragWindowDetails : public aura::WindowDelegate {
 public:
  DragWindowDetails(const display::Display& display,
                    aura::Window* original_window)
      : root_window_(Shell::GetRootWindowForDisplayId(display.id())) {}

  ~DragWindowDetails() override {
    delete drag_window_;
    DCHECK(!drag_window_);
  }

  void Update(aura::Window* original_window,
              const gfx::Rect& bounds_in_screen,
              const gfx::Point& drag_location_in_screen) {
    gfx::Rect root_bounds_in_screen = root_window_->GetBoundsInScreen();
    if (!root_bounds_in_screen.Intersects(bounds_in_screen)) {
      delete drag_window_;
      // Make sure drag_window_ is reset so that new drag window will be created
      // when it becomes necessary again.
      DCHECK(!drag_window_);
      layer_owner_.reset();
      return;
    }
    if (!drag_window_)
      CreateDragWindow(original_window, bounds_in_screen);

    gfx::Rect bounds_in_root = bounds_in_screen;
    ::wm::ConvertRectFromScreen(drag_window_->parent(), &bounds_in_root);
    drag_window_->SetBounds(bounds_in_root);
    if (root_bounds_in_screen.Contains(drag_location_in_screen)) {
      SetOpacity(original_window, 1.f);
    } else {
      drag_window_->SetBounds(bounds_in_root);
      gfx::Rect visible_bounds = root_bounds_in_screen;
      visible_bounds.Intersect(bounds_in_screen);
      SetOpacity(original_window,
                 GetDragWindowOpacity(bounds_in_screen, visible_bounds));
    }
  }

 private:
  friend class DragWindowController;

  void CreateDragWindow(aura::Window* original_window,
                        const gfx::Rect& bounds_in_screen) {
    DCHECK(!drag_window_);
    original_window_ = original_window;
    drag_window_ = new aura::Window(this);
    int parent_id = original_window->parent()->id();
    aura::Window* container = root_window_->GetChildById(parent_id);

    drag_window_->SetType(aura::client::WINDOW_TYPE_POPUP);
    drag_window_->SetTransparent(true);
    drag_window_->Init(ui::LAYER_TEXTURED);
    drag_window_->SetName("DragWindow");
    drag_window_->set_id(kShellWindowId_PhantomWindow);
    drag_window_->SetProperty(aura::client::kAnimationsDisabledKey, true);
    container->AddChild(drag_window_);
    drag_window_->SetBounds(bounds_in_screen);
    ::wm::SetShadowElevation(drag_window_, ::wm::kShadowElevationActiveWindow);

    RecreateWindowLayers(original_window);
    layer_owner_->root()->SetVisible(true);
    drag_window_->layer()->Add(layer_owner_->root());
    drag_window_->layer()->StackAtTop(layer_owner_->root());

    // Show the widget after all the setups.
    drag_window_->Show();

    // Fade the window in.
    ui::Layer* drag_layer = drag_window_->layer();
    drag_layer->SetOpacity(0);
    ui::ScopedLayerAnimationSettings scoped_setter(drag_layer->GetAnimator());
    drag_layer->SetOpacity(1);
  }

  void RecreateWindowLayers(aura::Window* original_window) {
    DCHECK(!layer_owner_.get());
    layer_owner_ = ::wm::MirrorLayers(original_window, true /* sync_bounds */);
    // Place the layer at (0, 0) of the DragWindowController's window.
    gfx::Rect layer_bounds = layer_owner_->root()->bounds();
    layer_bounds.set_origin(gfx::Point(0, 0));
    layer_owner_->root()->SetBounds(layer_bounds);
    layer_owner_->root()->SetVisible(false);
  }

  void SetOpacity(const aura::Window* original_window, float opacity) {
    ui::Layer* layer = drag_window_->layer();
    ui::ScopedLayerAnimationSettings scoped_setter(layer->GetAnimator());
    layer->SetOpacity(opacity);
    layer_owner_->root()->SetOpacity(1.0f);
  }

  // aura::WindowDelegate:
  gfx::Size GetMinimumSize() const override { return gfx::Size(); }
  gfx::Size GetMaximumSize() const override { return gfx::Size(); }
  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override {}
  gfx::NativeCursor GetCursor(const gfx::Point& point) override {
    return gfx::kNullCursor;
  }
  int GetNonClientComponent(const gfx::Point& point) const override {
    return HTNOWHERE;
  }
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override {
    return false;
  }
  bool CanFocus() override { return false; }
  void OnCaptureLost() override {}
  void OnPaint(const ui::PaintContext& context) override {}
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}
  void OnWindowDestroyed(aura::Window* window) override {}
  void OnWindowTargetVisibilityChanged(bool visible) override {}
  bool HasHitTestMask() const override { return false; }
  void GetHitTestMask(gfx::Path* mask) const override {}
  void OnWindowDestroying(aura::Window* window) override {
    DCHECK_EQ(drag_window_, window);
    drag_window_ = nullptr;
  }

  aura::Window* root_window_;

  aura::Window* drag_window_ = nullptr;  // Owned by the container.

  aura::Window* original_window_ = nullptr;

  // The copy of window_->layer() and its descendants.
  std::unique_ptr<ui::LayerTreeOwner> layer_owner_;

  DISALLOW_COPY_AND_ASSIGN(DragWindowDetails);
};

// static
float DragWindowController::GetDragWindowOpacity(
    const gfx::Rect& window_bounds,
    const gfx::Rect& visible_bounds) {
  // The maximum opacity of the drag phantom window.
  static const float kMaxOpacity = 0.8f;

  return kMaxOpacity * visible_bounds.size().GetArea() /
         window_bounds.size().GetArea();
}

DragWindowController::DragWindowController(aura::Window* window)
    : window_(window) {
  DCHECK(drag_windows_.empty());
  display::Screen* screen = display::Screen::GetScreen();
  display::Display current = screen->GetDisplayNearestWindow(window_);

  for (const display::Display& display : screen->GetAllDisplays()) {
    if (current.id() == display.id())
      continue;
    drag_windows_.push_back(
        std::make_unique<DragWindowDetails>(display, window_));
  }
}

DragWindowController::~DragWindowController() = default;

void DragWindowController::Update(const gfx::Rect& bounds_in_screen,
                                  const gfx::Point& drag_location_in_screen) {
  for (std::unique_ptr<DragWindowDetails>& details : drag_windows_)
    details->Update(window_, bounds_in_screen, drag_location_in_screen);
}

int DragWindowController::GetDragWindowsCountForTest() const {
  int count = 0;
  for (const std::unique_ptr<DragWindowDetails>& details : drag_windows_) {
    if (details->drag_window_)
      count++;
  }
  return count;
}

const aura::Window* DragWindowController::GetDragWindowForTest(
    size_t index) const {
  for (const std::unique_ptr<DragWindowDetails>& details : drag_windows_) {
    if (details->drag_window_) {
      if (index == 0)
        return details->drag_window_;
      index--;
    }
  }
  return nullptr;
}

const ui::LayerTreeOwner* DragWindowController::GetDragLayerOwnerForTest(
    size_t index) const {
  for (const std::unique_ptr<DragWindowDetails>& details : drag_windows_) {
    if (details->layer_owner_) {
      if (index == 0)
        return details->layer_owner_.get();
      index--;
    }
  }
  return nullptr;
}

void DragWindowController::RequestLayerPaintForTest() {
  ui::PaintContext context(nullptr, 1.0f, gfx::Rect(),
                           window_->GetHost()->compositor()->is_pixel_canvas());
  for (auto& details : drag_windows_) {
    std::vector<ui::Layer*> layers;
    layers.push_back(details->drag_window_->layer());
    while (layers.size()) {
      ui::Layer* layer = layers.back();
      layers.pop_back();
      if (layer->delegate())
        layer->delegate()->OnPaintLayer(context);
      for (auto* child : layer->children())
        layers.push_back(child);
    }
  }
}

}  // namespace ash
