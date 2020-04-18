// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/chrome_views_delegate.h"

#include "ash/accelerators/accelerator_controller.h"
#include "ash/shell.h"
#include "ash/wm/window_state.h"
#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace {

void ProcessAcceleratorNow(const ui::Accelerator& accelerator) {
  // TODO(afakhry): See if we need here to send the accelerator to the
  // FocusManager of the active window in a follow-up CL.
  ash::Shell::Get()->accelerator_controller()->Process(accelerator);
}

}  // namespace

views::ViewsDelegate::ProcessMenuAcceleratorResult
ChromeViewsDelegate::ProcessAcceleratorWhileMenuShowing(
    const ui::Accelerator& accelerator) {
  DCHECK(base::MessageLoopForUI::IsCurrent());

  // Early return because mash chrome does not have access to ash::Shell
  if (ash_util::IsRunningInMash())
    return views::ViewsDelegate::ProcessMenuAcceleratorResult::LEAVE_MENU_OPEN;

  ash::AcceleratorController* accelerator_controller =
      ash::Shell::Get()->accelerator_controller();

  accelerator_controller->accelerator_history()->StoreCurrentAccelerator(
      accelerator);
  if (accelerator_controller->ShouldCloseMenuAndRepostAccelerator(
          accelerator)) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(ProcessAcceleratorNow, accelerator));
    return views::ViewsDelegate::ProcessMenuAcceleratorResult::CLOSE_MENU;
  }

  ProcessAcceleratorNow(accelerator);
  return views::ViewsDelegate::ProcessMenuAcceleratorResult::LEAVE_MENU_OPEN;
}

views::NonClientFrameView* ChromeViewsDelegate::CreateDefaultNonClientFrameView(
    views::Widget* widget) {
  return ash::Shell::Get()->CreateDefaultNonClientFrameView(widget);
}

void ChromeViewsDelegate::AdjustSavedWindowPlacementChromeOS(
    const views::Widget* widget,
    gfx::Rect* bounds) const {
  // On ChromeOS a window won't span across displays.  Adjust the bounds to fit
  // the work area.
  aura::Window* window = widget->GetNativeView();
  display::Display display =
      display::Screen::GetScreen()->GetDisplayMatching(*bounds);
  bounds->AdjustToFit(display.work_area());
  ash::wm::GetWindowState(window)->set_minimum_visibility(true);
}

views::Widget::InitParams::WindowOpacity
ChromeViewsDelegate::GetOpacityForInitParams(
    const views::Widget::InitParams& params) {
  return views::Widget::InitParams::TRANSLUCENT_WINDOW;
}

views::NativeWidget* ChromeViewsDelegate::CreateNativeWidget(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  // The context should be associated with a root window. If the context has a
  // null root window (e.g. the context window has no parent) it will trigger
  // the fallback case below. https://crbug.com/828626 https://crrev.com/230793
  if (params->context)
    params->context = params->context->GetRootWindow();

  // Classic ash requires a parent or a context that it can use to look up a
  // root window to find a WindowParentingClient. Mash handles window parenting
  // inside ash, see ash::CreateAndParentTopLevelWindow().
  if (!ash_util::IsRunningInMash() && !params->parent && !params->context)
    params->context = ash::Shell::GetRootWindowForNewWindows();

  // By returning null Widget creates the default NativeWidget implementation,
  // which for chromeos is NativeWidgetAura.
  return nullptr;
}
