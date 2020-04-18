// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/mode_indicator_controller.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "mojo/public/cpp/bindings/type_converter.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/base/ime/chromeos/input_method_util.h"

namespace chromeos {
namespace input_method {

namespace {

ModeIndicatorObserverInterface* g_mode_indicator_observer_for_testing = nullptr;

// The non-test version of the interface.
class ModeIndicatorObserver : public ModeIndicatorObserverInterface {
 public:
  ModeIndicatorObserver()
    : active_widget_(NULL) {}

  ~ModeIndicatorObserver() override {
    if (active_widget_)
      active_widget_->RemoveObserver(this);
  }

  // If other active mode indicator widget is shown, close it immedicately
  // without fading animation.  Then store this widget as the active widget.
  void AddModeIndicatorWidget(views::Widget* widget) override {
    DCHECK(widget);
    if (active_widget_)
      active_widget_->Close();
    active_widget_ = widget;
    widget->AddObserver(this);
  }

  // views::WidgetObserver override:
  void OnWidgetDestroying(views::Widget* widget) override {
    if (widget == active_widget_)
      active_widget_ = NULL;
  }

 private:
  views::Widget* active_widget_;
};

}  // namespace

ModeIndicatorController::ModeIndicatorController(InputMethodManager* imm)
    : imm_(imm),
      is_focused_(false),
      mi_observer_(new ModeIndicatorObserver) {
  DCHECK(imm_);
  imm_->AddObserver(this);
}

ModeIndicatorController::~ModeIndicatorController() {
  imm_->RemoveObserver(this);
}

void ModeIndicatorController::SetCursorBounds(
    const gfx::Rect& cursor_bounds) {
  cursor_bounds_ = cursor_bounds;
}

void ModeIndicatorController::FocusStateChanged(bool is_focused) {
  is_focused_ = is_focused;
}

// static
void ModeIndicatorController::SetModeIndicatorObserverForTesting(
    ModeIndicatorObserverInterface* observer) {
  g_mode_indicator_observer_for_testing = observer;
}

void ModeIndicatorController::InputMethodChanged(InputMethodManager* manager,
                                                 Profile* /* profile */,
                                                 bool show_message) {
  if (!show_message)
    return;
  ShowModeIndicator();
}

void ModeIndicatorController::InitWidgetContainer(
    views::Widget::InitParams* params) {
  // The bubble needs to be placed in the proper ash window container, even
  // though it is created by Chrome.
  // TODO(crbug.com/738531): Consider moving the ModeIndicatorView into ash.
  const int container_id = ash::kShellWindowId_SettingBubbleContainer;
  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    using ui::mojom::WindowManager;
    params->mus_properties[WindowManager::kContainerId_InitProperty] =
        mojo::ConvertTo<std::vector<uint8_t>>(container_id);
  } else {
    params->parent = ash::Shell::GetContainer(
        ash::wm::GetActiveWindow()->GetRootWindow(), container_id);
  }
}

void ModeIndicatorController::ShowModeIndicator() {
  // TODO(komatsu): Show the mode indicator in the right bottom of the
  // display when the launch bar is hidden and the focus is out.  To
  // implement it, we should consider to use message center or system
  // notification.  Note, launch bar can be vertical and can be placed
  // right/left side of display.
  if (!is_focused_)
    return;

  // Get the short name of the changed input method (e.g. US, JA, etc.)
  const InputMethodDescriptor descriptor =
      imm_->GetActiveIMEState()->GetCurrentInputMethod();
  const base::string16 short_name =
      imm_->GetInputMethodUtil()->GetInputMethodShortName(descriptor);

  ui::ime::ModeIndicatorView* mi_view =
      new ui::ime::ModeIndicatorView(this, cursor_bounds_, short_name);
  views::BubbleDialogDelegateView::CreateBubble(mi_view);

  views::Widget* mi_widget = mi_view->GetWidget();
  if (g_mode_indicator_observer_for_testing)
    g_mode_indicator_observer_for_testing->AddModeIndicatorWidget(mi_widget);

  mi_observer_->AddModeIndicatorWidget(mi_widget);
  mi_view->ShowAndFadeOut();
}

}  // namespace input_method
}  // namespace chromeos
