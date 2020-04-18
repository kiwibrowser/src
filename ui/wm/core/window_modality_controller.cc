// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/window_modality_controller.h"

#include <stddef.h>

#include <algorithm>

#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/capture_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/class_property.h"
#include "ui/base/ui_base_types.h"
#include "ui/events/event.h"
#include "ui/events/event_target.h"
#include "ui/events/gestures/gesture_recognizer.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_util.h"

namespace wm {
namespace {

bool HasAncestor(aura::Window* window, aura::Window* ancestor) {
  return ancestor && ancestor->Contains(window);
}

bool TransientChildIsWindowModal(aura::Window* window) {
  return window->GetProperty(aura::client::kModalKey) == ui::MODAL_TYPE_WINDOW;
}

bool TransientChildIsSystemModal(aura::Window* window) {
  return window->GetProperty(aura::client::kModalKey) == ui::MODAL_TYPE_SYSTEM;
}

bool TransientChildIsChildModal(aura::Window* window) {
  return window->GetProperty(aura::client::kModalKey) == ui::MODAL_TYPE_CHILD;
}

aura::Window* GetModalParent(aura::Window* window) {
  return window->GetProperty(aura::client::kChildModalParentKey);
}

bool IsModalTransientChild(aura::Window* transient, aura::Window* original) {
  return transient->IsVisible() &&
         (TransientChildIsWindowModal(transient) ||
          TransientChildIsSystemModal(transient) ||
          (TransientChildIsChildModal(transient) &&
           HasAncestor(original, GetModalParent(transient))));
}

aura::Window* GetModalTransientChild(
    aura::Window* activatable,
    aura::Window* original) {
  for (aura::Window::Windows::const_iterator it =
           GetTransientChildren(activatable).begin();
       it != GetTransientChildren(activatable).end();
       ++it) {
    aura::Window* transient = *it;
    if (IsModalTransientChild(transient, original)) {
      if (GetTransientChildren(transient).empty())
        return transient;

      aura::Window* modal_child = GetModalTransientChild(transient, original);
      return modal_child ? modal_child : transient;
    }
  }
  return NULL;
}

}  // namespace

void SetModalParent(aura::Window* child, aura::Window* parent) {
  child->SetProperty(aura::client::kChildModalParentKey, parent);
}

aura::Window* GetModalTransient(aura::Window* window) {
  if (!window)
    return NULL;

  // We always want to check the for the transient child of the toplevel window.
  aura::Window* toplevel = GetToplevelWindow(window);
  if (!toplevel)
    return NULL;

  return GetModalTransientChild(toplevel, window);
}

////////////////////////////////////////////////////////////////////////////////
// WindowModalityController, public:

WindowModalityController::WindowModalityController(
    ui::EventTarget* event_target)
    : event_target_(event_target) {
  aura::Env::GetInstance()->AddObserver(this);
  DCHECK(event_target->IsPreTargetListEmpty());
  event_target_->AddPreTargetHandler(this);
}

WindowModalityController::~WindowModalityController() {
  event_target_->RemovePreTargetHandler(this);
  aura::Env::GetInstance()->RemoveObserver(this);
  for (size_t i = 0; i < windows_.size(); ++i)
    windows_[i]->RemoveObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// WindowModalityController, aura::EventFilter implementation:

void WindowModalityController::OnKeyEvent(ui::KeyEvent* event) {
  aura::Window* target = static_cast<aura::Window*>(event->target());
  if (GetModalTransient(target))
    event->SetHandled();
}

void WindowModalityController::OnMouseEvent(ui::MouseEvent* event) {
  aura::Window* target = static_cast<aura::Window*>(event->target());
  if (ProcessLocatedEvent(target, event))
   event->SetHandled();
}

void WindowModalityController::OnTouchEvent(ui::TouchEvent* event) {
  aura::Window* target = static_cast<aura::Window*>(event->target());
  if (ProcessLocatedEvent(target, event))
    event->SetHandled();
}

////////////////////////////////////////////////////////////////////////////////
// WindowModalityController, aura::EnvObserver implementation:

void WindowModalityController::OnWindowInitialized(aura::Window* window) {
  windows_.push_back(window);
  window->AddObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// WindowModalityController, aura::WindowObserver implementation:

void WindowModalityController::OnWindowPropertyChanged(aura::Window* window,
                                                       const void* key,
                                                       intptr_t old) {
  // In tests, we sometimes create the modality relationship after a window is
  // visible.
  if (key == aura::client::kModalKey &&
      window->GetProperty(aura::client::kModalKey) != ui::MODAL_TYPE_NONE &&
      window->IsVisible()) {
    ActivateWindow(window);
    ui::GestureRecognizer::Get()->CancelActiveTouchesExcept(nullptr);
  }
}

void WindowModalityController::OnWindowVisibilityChanged(
    aura::Window* window,
    bool visible) {
  if (visible &&
      window->GetProperty(aura::client::kModalKey) != ui::MODAL_TYPE_NONE) {
    ui::GestureRecognizer::Get()->CancelActiveTouchesExcept(nullptr);
    // Make sure no other window has capture, otherwise |window| won't get mouse
    // events.
    aura::Window* capture_window = aura::client::GetCaptureWindow(window);
    if (capture_window) {
      bool should_release_capture = true;
      if (window->GetProperty(aura::client::kModalKey) ==
              ui::MODAL_TYPE_CHILD &&
          !HasAncestor(capture_window, GetModalParent(window))) {
        // For child modal windows we only need ensure capture is not on a
        // descendant of the modal parent. This way we block events to the
        // parents subtree appropriately.
        should_release_capture = false;
      }

      if (should_release_capture)
        capture_window->ReleaseCapture();
    }
  }
}

void WindowModalityController::OnWindowDestroyed(aura::Window* window) {
  windows_.erase(std::find(windows_.begin(), windows_.end(), window));
  window->RemoveObserver(this);
}

bool WindowModalityController::ProcessLocatedEvent(aura::Window* target,
                                                   ui::LocatedEvent* event) {
  if (event->handled())
    return false;
  aura::Window* modal_transient_child = GetModalTransient(target);
  if (modal_transient_child && (event->type() == ui::ET_MOUSE_PRESSED ||
                                event->type() == ui::ET_TOUCH_PRESSED)) {
    // Activate top window if transient child window is window modal.
    if (TransientChildIsWindowModal(modal_transient_child)) {
      aura::Window* toplevel = GetToplevelWindow(target);
      DCHECK(toplevel);
      ActivateWindow(toplevel);
    }

    AnimateWindow(modal_transient_child, WINDOW_ANIMATION_TYPE_BOUNCE);
  }
  if (event->type() == ui::ET_TOUCH_CANCELLED)
    return false;
  return !!modal_transient_child;
}

}  // namespace wm
