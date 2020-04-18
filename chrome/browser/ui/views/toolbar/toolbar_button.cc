// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/toolbar/toolbar_button.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/models/menu_model.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/widget/widget.h"

ToolbarButton::ToolbarButton(Profile* profile,
                             views::ButtonListener* listener,
                             std::unique_ptr<ui::MenuModel> model)
    : views::ImageButton(listener),
      profile_(profile),
      model_(std::move(model)),
      show_menu_factory_(this) {
  set_has_ink_drop_action_on_click(true);
  set_context_menu_controller(this);
  SetInkDropMode(InkDropMode::ON);
  SetFocusPainter(nullptr);
  SetLeadingMargin(0);
  SetInstallFocusRingOnFocus(views::PlatformStyle::kPreferFocusRings);

  if (ui::MaterialDesignController::IsTouchOptimizedUiEnabled())
    set_ink_drop_visible_opacity(kTouchToolbarInkDropVisibleOpacity);

  const int size = GetLayoutConstant(LOCATION_BAR_HEIGHT);
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::EMPHASIS_HIGH, gfx::Size(size, size));
  set_ink_drop_corner_radii(radii, radii);
}

ToolbarButton::~ToolbarButton() {}

void ToolbarButton::Init() {
  SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
}

void ToolbarButton::SetLeadingMargin(int margin) {
  const gfx::Insets insets =
      GetLayoutInsets(TOOLBAR_BUTTON) + gfx::Insets(0, margin, 0, 0);
  SetBorder(views::CreateEmptyBorder(insets));
}

void ToolbarButton::ClearPendingMenu() {
  show_menu_factory_.InvalidateWeakPtrs();
}

bool ToolbarButton::IsMenuShowing() const {
  return menu_showing_;
}

bool ToolbarButton::OnMousePressed(const ui::MouseEvent& event) {
  if (IsTriggerableEvent(event) && enabled() && ShouldShowMenu() &&
      HitTestPoint(event.location())) {
    // Store the y pos of the mouse coordinates so we can use them later to
    // determine if the user dragged the mouse down (which should pop up the
    // drag down menu immediately, instead of waiting for the timer)
    y_position_on_lbuttondown_ = event.y();

    // Schedule a task that will show the menu.
    const int kMenuTimerDelay = 500;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&ToolbarButton::ShowDropDownMenu,
                       show_menu_factory_.GetWeakPtr(),
                       ui::GetMenuSourceTypeForEvent(event)),
        base::TimeDelta::FromMilliseconds(kMenuTimerDelay));
  }

  return ImageButton::OnMousePressed(event);
}

bool ToolbarButton::OnMouseDragged(const ui::MouseEvent& event) {
  bool result = ImageButton::OnMouseDragged(event);

  if (show_menu_factory_.HasWeakPtrs()) {
    // If the mouse is dragged to a y position lower than where it was when
    // clicked then we should not wait for the menu to appear but show
    // it immediately.
    if (event.y() > y_position_on_lbuttondown_ + GetHorizontalDragThreshold()) {
      show_menu_factory_.InvalidateWeakPtrs();
      ShowDropDownMenu(ui::GetMenuSourceTypeForEvent(event));
    }
  }

  return result;
}

void ToolbarButton::OnMouseReleased(const ui::MouseEvent& event) {
  if (IsTriggerableEvent(event) ||
      (event.IsRightMouseButton() && !HitTestPoint(event.location()))) {
    ImageButton::OnMouseReleased(event);
  }

  if (IsTriggerableEvent(event))
    show_menu_factory_.InvalidateWeakPtrs();
}

void ToolbarButton::OnMouseCaptureLost() {
}

void ToolbarButton::OnMouseExited(const ui::MouseEvent& event) {
  // Starting a drag results in a MouseExited, we need to ignore it.
  // A right click release triggers an exit event. We want to
  // remain in a PUSHED state until the drop down menu closes.
  if (state() != STATE_DISABLED && !InDrag() && state() != STATE_PRESSED)
    SetState(STATE_NORMAL);
}

void ToolbarButton::OnGestureEvent(ui::GestureEvent* event) {
  if (menu_showing_) {
    // While dropdown menu is showing the button should not handle gestures.
    event->StopPropagation();
    return;
  }

  ImageButton::OnGestureEvent(event);
}

void ToolbarButton::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  Button::GetAccessibleNodeData(node_data);
  node_data->role = ax::mojom::Role::kButton;
  node_data->SetHasPopup(ax::mojom::HasPopup::kMenu);
  if (enabled())
    node_data->SetDefaultActionVerb(ax::mojom::DefaultActionVerb::kPress);
}

std::unique_ptr<views::InkDrop> ToolbarButton::CreateInkDrop() {
  return CreateToolbarInkDrop<ImageButton>(this);
}

std::unique_ptr<views::InkDropRipple> ToolbarButton::CreateInkDropRipple()
    const {
  return CreateToolbarInkDropRipple<ImageButton>(
      this, GetInkDropCenterBasedOnLastEvent());
}

std::unique_ptr<views::InkDropHighlight> ToolbarButton::CreateInkDropHighlight()
    const {
  return CreateToolbarInkDropHighlight<ImageButton>(
      this, GetMirroredRect(GetContentsBounds()).CenterPoint());
}

std::unique_ptr<views::InkDropMask> ToolbarButton::CreateInkDropMask() const {
  return CreateToolbarInkDropMask<ImageButton>(this);
}

SkColor ToolbarButton::GetInkDropBaseColor() const {
  return GetToolbarInkDropBaseColor(this);
}

void ToolbarButton::ShowContextMenuForView(View* source,
                                           const gfx::Point& point,
                                           ui::MenuSourceType source_type) {
  if (!enabled())
    return;

  show_menu_factory_.InvalidateWeakPtrs();
  ShowDropDownMenu(source_type);
}

bool ToolbarButton::ShouldShowMenu() {
  return model_ != nullptr;
}

void ToolbarButton::ShowDropDownMenu(ui::MenuSourceType source_type) {
  if (!ShouldShowMenu())
    return;

  gfx::Rect lb = GetLocalBounds();

  // Both the menu position and the menu anchor type change if the UI layout
  // is right-to-left.
  gfx::Point menu_position(lb.origin());
  menu_position.Offset(0, lb.height() - 1);
  if (base::i18n::IsRTL())
    menu_position.Offset(lb.width() - 1, 0);

  View::ConvertPointToScreen(this, &menu_position);

#if defined(OS_CHROMEOS)
  // A window won't overlap between displays on ChromeOS.
  // Use the left bound of the display on which
  // the menu button exists.
  gfx::NativeView view = GetWidget()->GetNativeView();
  display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestView(view);
  int left_bound = display.bounds().x();
#else
  // The window might be positioned over the edge between two screens. We'll
  // want to position the dropdown on the screen the mouse cursor is on.
  display::Screen* screen = display::Screen::GetScreen();
  display::Display display =
      screen->GetDisplayNearestPoint(screen->GetCursorScreenPoint());
  int left_bound = display.bounds().x();
#endif
  if (menu_position.x() < left_bound)
    menu_position.set_x(left_bound);

  // Make the button look depressed while the menu is open.
  SetState(STATE_PRESSED);

  menu_showing_ = true;

  AnimateInkDrop(views::InkDropState::ACTIVATED, nullptr /* event */);

  // Exit if the model is null.
  if (!model_.get())
    return;

  // Create and run menu.
  menu_model_adapter_.reset(new views::MenuModelAdapter(
      model_.get(),
      base::Bind(&ToolbarButton::OnMenuClosed, base::Unretained(this))));
  menu_model_adapter_->set_triggerable_event_flags(triggerable_event_flags());
  menu_runner_.reset(new views::MenuRunner(menu_model_adapter_->CreateMenu(),
                                           views::MenuRunner::HAS_MNEMONICS));
  menu_runner_->RunMenuAt(GetWidget(), nullptr,
                          gfx::Rect(menu_position, gfx::Size(0, 0)),
                          views::MENU_ANCHOR_TOPLEFT, source_type);
}

void ToolbarButton::OnMenuClosed() {
  AnimateInkDrop(views::InkDropState::DEACTIVATED, nullptr /* event */);

  menu_showing_ = false;

  // Set the state back to normal after the drop down menu is closed.
  if (state() != STATE_DISABLED)
    SetState(STATE_NORMAL);

  menu_runner_.reset();
  menu_model_adapter_.reset();
}

const char* ToolbarButton::GetClassName() const {
  return "ToolbarButton";
}
