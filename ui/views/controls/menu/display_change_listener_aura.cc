// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/widget/widget.h"

namespace views {
namespace internal {

// DisplayChangeListener implementation for aura. Cancels the menu any time the
// root window bounds change.
class AuraDisplayChangeListener
    : public DisplayChangeListener,
      public aura::WindowObserver {
 public:
  AuraDisplayChangeListener(Widget* widget, MenuRunner* menu_runner);
  ~AuraDisplayChangeListener() override;

  // aura::WindowObserver overrides:
  void OnWindowBoundsChanged(aura::Window* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds,
                             ui::PropertyChangeReason reason) override;
  void OnWindowDestroying(aura::Window* window) override;

 private:
  MenuRunner* menu_runner_;
  aura::Window* root_window_;

  DISALLOW_COPY_AND_ASSIGN(AuraDisplayChangeListener);
};

AuraDisplayChangeListener::AuraDisplayChangeListener(Widget* widget,
                                                     MenuRunner* menu_runner)
    : menu_runner_(menu_runner),
      root_window_(widget->GetNativeView()->GetRootWindow()) {
  if (root_window_)
    root_window_->AddObserver(this);
}

AuraDisplayChangeListener::~AuraDisplayChangeListener() {
  if (root_window_)
    root_window_->RemoveObserver(this);
}

void AuraDisplayChangeListener::OnWindowBoundsChanged(
    aura::Window* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds,
    ui::PropertyChangeReason reason) {
  menu_runner_->Cancel();
}

void AuraDisplayChangeListener::OnWindowDestroying(aura::Window* window) {
  DCHECK_EQ(window, root_window_);
  root_window_->RemoveObserver(this);
  root_window_ = NULL;
}

// static
DisplayChangeListener* DisplayChangeListener::Create(Widget* widget,
                                                     MenuRunner* runner) {
  return new AuraDisplayChangeListener(widget, runner);
}

}  // namespace internal
}  // namespace views
