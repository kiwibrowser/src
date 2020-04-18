// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/accessibility_highlight_controller.h"

#include <vector>

#include "ash/accessibility/accessibility_focus_ring_controller.h"
#include "ash/public/cpp/config.h"
#include "ash/public/interfaces/accessibility_focus_ring_controller.mojom.h"
#include "ash/shell.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/events/event.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/cursor_manager.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

namespace {

ui::InputMethod* GetInputMethod(aura::Window* root_window) {
  if (root_window->GetHost())
    return root_window->GetHost()->GetInputMethod();
  return nullptr;
}

}  // namespace

AccessibilityHighlightController::AccessibilityHighlightController() {
  Shell::Get()->AddPreTargetHandler(this);
  // TODO: CursorManager not created in mash. https://crbug.com/631103.
  if (Shell::GetAshConfig() != Config::MASH)
    Shell::Get()->cursor_manager()->AddObserver(this);
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  ui::InputMethod* input_method = GetInputMethod(root_window);
  input_method->AddObserver(this);
}

AccessibilityHighlightController::~AccessibilityHighlightController() {
  AccessibilityFocusRingController* controller =
      Shell::Get()->accessibility_focus_ring_controller();
  controller->SetFocusRing(std::vector<gfx::Rect>(),
                           mojom::FocusRingBehavior::FADE_OUT_FOCUS_RING);
  controller->HideCaretRing();
  controller->HideCursorRing();

  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  ui::InputMethod* input_method = GetInputMethod(root_window);
  input_method->RemoveObserver(this);
  // TODO: CursorManager not created in mash. https://crbug.com/631103.
  if (Shell::GetAshConfig() != Config::MASH)
    Shell::Get()->cursor_manager()->RemoveObserver(this);
  Shell::Get()->RemovePreTargetHandler(this);
}

void AccessibilityHighlightController::HighlightFocus(bool focus) {
  focus_ = focus;
  UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightController::HighlightCursor(bool cursor) {
  cursor_ = cursor;
  UpdateCursorHighlight();
}

void AccessibilityHighlightController::HighlightCaret(bool caret) {
  caret_ = caret;
  UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightController::SetFocusHighlightRect(
    const gfx::Rect& bounds_in_screen) {
  focus_rect_ = bounds_in_screen;
  UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightController::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_MOVED) {
    cursor_point_ = event->location();
    if (event->target()) {
      ::wm::ConvertPointToScreen(static_cast<aura::Window*>(event->target()),
                                 &cursor_point_);
    }
    UpdateCursorHighlight();
  }
}

void AccessibilityHighlightController::OnKeyEvent(ui::KeyEvent* event) {
  if (event->type() == ui::ET_KEY_PRESSED)
    UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightController::OnTextInputStateChanged(
    const ui::TextInputClient* client) {
  if (!client || client->GetTextInputType() == ui::TEXT_INPUT_TYPE_NONE) {
    caret_visible_ = false;
    UpdateFocusAndCaretHighlights();
  }
}

void AccessibilityHighlightController::OnCaretBoundsChanged(
    const ui::TextInputClient* client) {
  if (!client || client->GetTextInputType() == ui::TEXT_INPUT_TYPE_NONE) {
    caret_visible_ = false;
    return;
  }
  gfx::Rect caret_bounds = client->GetCaretBounds();
  gfx::Point new_caret_point = caret_bounds.CenterPoint();
  ::wm::ConvertPointFromScreen(Shell::GetPrimaryRootWindow(), &new_caret_point);
  if (new_caret_point == caret_point_)
    return;
  caret_point_ = new_caret_point;
  caret_visible_ = IsCaretVisible(caret_bounds);
  UpdateFocusAndCaretHighlights();
}

void AccessibilityHighlightController::OnCursorVisibilityChanged(
    bool is_visible) {
  UpdateCursorHighlight();
}

bool AccessibilityHighlightController::IsCursorVisible() {
  // TODO: CursorManager not created in mash. https://crbug.com/631103.
  if (Shell::GetAshConfig() == Config::MASH)
    return false;
  return Shell::Get()->cursor_manager()->IsCursorVisible();
}

bool AccessibilityHighlightController::IsCaretVisible(
    const gfx::Rect& caret_bounds) {
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  aura::Window* active_window =
      ::wm::GetActivationClient(root_window)->GetActiveWindow();
  if (!active_window)
    active_window = root_window;
  return (caret_bounds.width() || caret_bounds.height()) &&
         active_window->GetBoundsInScreen().Contains(caret_point_);
}

void AccessibilityHighlightController::UpdateFocusAndCaretHighlights() {
  AccessibilityFocusRingController* controller =
      Shell::Get()->accessibility_focus_ring_controller();

  // The caret highlight takes precedence over the focus highlight if
  // both are visible.
  if (caret_ && caret_visible_) {
    controller->SetCaretRing(caret_point_);
    controller->SetFocusRing(std::vector<gfx::Rect>(),
                             mojom::FocusRingBehavior::FADE_OUT_FOCUS_RING);
  } else if (focus_) {
    controller->HideCaretRing();
    std::vector<gfx::Rect> rects;
    if (!focus_rect_.IsEmpty())
      rects.push_back(focus_rect_);
    controller->SetFocusRing(rects,
                             mojom::FocusRingBehavior::FADE_OUT_FOCUS_RING);
  } else {
    controller->HideCaretRing();
    controller->SetFocusRing(std::vector<gfx::Rect>(),
                             mojom::FocusRingBehavior::FADE_OUT_FOCUS_RING);
  }
}

void AccessibilityHighlightController::UpdateCursorHighlight() {
  AccessibilityFocusRingController* controller =
      Shell::Get()->accessibility_focus_ring_controller();
  if (cursor_ && IsCursorVisible())
    controller->SetCursorRing(cursor_point_);
  else
    controller->HideCursorRing();
}

}  // namespace ash
