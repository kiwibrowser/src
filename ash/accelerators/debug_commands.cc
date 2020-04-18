// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/debug_commands.h"

#include "ash/accelerators/accelerator_commands.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/system/toast/toast_data.h"
#include "ash/system/toast/toast_manager.h"
#include "ash/touch/touch_devices_controller.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/widget_finder.h"
#include "ash/wm/window_properties.h"
#include "ash/wm/window_util.h"
#include "base/command_line.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/compositor/debug_utils.h"
#include "ui/compositor/layer.h"
#include "ui/display/manager/display_manager.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/views/debug_utils.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace debug {
namespace {

void HandlePrintLayerHierarchy() {
  for (aura::Window* root : Shell::Get()->GetAllRootWindows()) {
    ui::Layer* layer = root->layer();
    if (layer)
      ui::PrintLayerHierarchy(
          layer,
          RootWindowController::ForWindow(root)->GetLastMouseLocationInRoot());
  }
}

void HandlePrintViewHierarchy() {
  aura::Window* active_window = wm::GetActiveWindow();
  if (!active_window)
    return;
  views::Widget* widget = GetInternalWidgetForWindow(active_window);
  if (!widget)
    return;
  views::PrintViewHierarchy(widget->GetRootView());
}

void PrintWindowHierarchy(const aura::Window* active_window,
                          aura::Window* window,
                          int indent,
                          std::ostringstream* out) {
  std::string indent_str(indent, ' ');
  std::string name(window->GetName());
  if (name.empty())
    name = "\"\"";
  *out << indent_str << name << " (" << window << ")"
       << " type=" << window->type()
       << ((window == active_window) ? " [active] " : " ")
       << (window->IsVisible() ? " visible " : " ")
       << window->bounds().ToString()
       << (window->GetProperty(kSnapChildrenToPixelBoundary) ? " [snapped] "
                                                             : "")
       << ", subpixel offset="
       << window->layer()->subpixel_position_offset().ToString() << '\n';

  for (aura::Window* child : window->children())
    PrintWindowHierarchy(active_window, child, indent + 3, out);
}

void HandlePrintWindowHierarchy() {
  aura::Window* active_window = wm::GetActiveWindow();
  aura::Window::Windows roots = Shell::Get()->GetAllRootWindows();
  for (size_t i = 0; i < roots.size(); ++i) {
    std::ostringstream out;
    out << "RootWindow " << i << ":\n";
    PrintWindowHierarchy(active_window, roots[i], 0, &out);
    // Error so logs can be collected from end-users.
    LOG(ERROR) << out.str();
  }
}

gfx::ImageSkia CreateWallpaperImage(SkColor fill, SkColor rect) {
  // TODO(oshima): Consider adding a command line option to control wallpaper
  // images for testing. The size is randomly picked.
  gfx::Size image_size(1366, 768);
  SkBitmap bitmap;
  bitmap.allocN32Pixels(image_size.width(), image_size.height(), true);
  SkCanvas canvas(bitmap);
  canvas.drawColor(fill);
  SkPaint paint;
  paint.setColor(rect);
  paint.setStrokeWidth(10);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setBlendMode(SkBlendMode::kSrcOver);
  canvas.drawRoundRect(gfx::RectToSkRect(gfx::Rect(image_size)), 100.f, 100.f,
                       paint);
  return gfx::ImageSkia(gfx::ImageSkiaRep(std::move(bitmap), 1.f));
}

void HandleToggleWallpaperMode() {
  static int index = 0;
  WallpaperController* wallpaper_controller =
      Shell::Get()->wallpaper_controller();
  WallpaperInfo info("", WALLPAPER_LAYOUT_STRETCH, DEFAULT,
                     base::Time::Now().LocalMidnight());
  switch (++index % 4) {
    case 0:
      wallpaper_controller->ShowDefaultWallpaperForTesting();
      break;
    case 1:
      wallpaper_controller->ShowWallpaperImage(
          CreateWallpaperImage(SK_ColorRED, SK_ColorBLUE), info,
          false /*preview_mode=*/);
      break;
    case 2:
      info.layout = WALLPAPER_LAYOUT_CENTER;
      wallpaper_controller->ShowWallpaperImage(
          CreateWallpaperImage(SK_ColorBLUE, SK_ColorGREEN), info,
          false /*preview_mode=*/);
      break;
    case 3:
      info.layout = WALLPAPER_LAYOUT_CENTER_CROPPED;
      wallpaper_controller->ShowWallpaperImage(
          CreateWallpaperImage(SK_ColorGREEN, SK_ColorRED), info,
          false /*preview_mode=*/);
      break;
  }
}

void HandleToggleTouchpad() {
  base::RecordAction(base::UserMetricsAction("Accel_Toggle_Touchpad"));
  Shell::Get()->touch_devices_controller()->ToggleTouchpad();
}

void HandleToggleTouchscreen() {
  base::RecordAction(base::UserMetricsAction("Accel_Toggle_Touchscreen"));
  TouchDevicesController* controller = Shell::Get()->touch_devices_controller();
  controller->SetTouchscreenEnabled(
      !controller->GetTouchscreenEnabled(TouchDeviceEnabledSource::USER_PREF),
      TouchDeviceEnabledSource::USER_PREF);
}

void HandleToggleTabletMode() {
  TabletModeController* controller = Shell::Get()->tablet_mode_controller();
  controller->EnableTabletModeWindowManager(
      !controller->IsTabletModeWindowManagerEnabled());
}

void HandleTriggerCrash() {
  CHECK(false) << "Intentional crash via debug accelerator.";
}

}  // namespace

void PrintUIHierarchies() {
  // This is a separate command so the user only has to hit one key to generate
  // all the logs. Developers use the individual dumps repeatedly, so keep
  // those as separate commands to avoid spamming their logs.
  HandlePrintLayerHierarchy();
  HandlePrintWindowHierarchy();
  HandlePrintViewHierarchy();
}

bool DebugAcceleratorsEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kAshDebugShortcuts);
}

bool DeveloperAcceleratorsEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kAshDeveloperShortcuts);
}

void PerformDebugActionIfEnabled(AcceleratorAction action) {
  if (!DebugAcceleratorsEnabled())
    return;

  switch (action) {
    case DEBUG_PRINT_LAYER_HIERARCHY:
      HandlePrintLayerHierarchy();
      break;
    case DEBUG_PRINT_VIEW_HIERARCHY:
      HandlePrintViewHierarchy();
      break;
    case DEBUG_PRINT_WINDOW_HIERARCHY:
      HandlePrintWindowHierarchy();
      break;
    case DEBUG_SHOW_TOAST:
      Shell::Get()->toast_manager()->Show(
          ToastData("id", base::ASCIIToUTF16("Toast"), 5000 /* duration_ms */,
                    base::ASCIIToUTF16("Dismiss")));
      break;
    case DEBUG_TOGGLE_DEVICE_SCALE_FACTOR:
      Shell::Get()->display_manager()->ToggleDisplayScaleFactor();
      break;
    case DEBUG_TOGGLE_TOUCH_PAD:
      HandleToggleTouchpad();
      break;
    case DEBUG_TOGGLE_TOUCH_SCREEN:
      HandleToggleTouchscreen();
      break;
    case DEBUG_TOGGLE_TABLET_MODE:
      HandleToggleTabletMode();
      break;
    case DEBUG_TOGGLE_WALLPAPER_MODE:
      HandleToggleWallpaperMode();
      break;
    case DEBUG_TRIGGER_CRASH:
      HandleTriggerCrash();
      break;
    default:
      break;
  }
}

}  // namespace debug
}  // namespace ash
