// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display.h"

#include "build/build_config.h"
#include "services/ui/ws/platform_display_default.h"
#include "services/ui/ws/platform_display_factory.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/threaded_image_cursors.h"
#include "services/ui/ws/threaded_image_cursors_factory.h"
#include "ui/platform_window/platform_window.h"

#if defined(OS_WIN)
#include "ui/platform_window/win/win_window.h"
#elif defined(USE_X11)
#include "ui/platform_window/x11/x11_window.h"
#elif defined(OS_ANDROID)
#include "ui/platform_window/android/platform_window_android.h"
#elif defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#include "ui/platform_window/platform_window_delegate.h"
#endif

namespace ui {
namespace ws {

// static
PlatformDisplayFactory* PlatformDisplay::factory_ = nullptr;

// static
std::unique_ptr<PlatformDisplay> PlatformDisplay::Create(
    ServerWindow* root,
    const display::ViewportMetrics& metrics,
    ThreadedImageCursorsFactory* threaded_image_cursors_factory) {
  if (factory_)
    return factory_->CreatePlatformDisplay(root, metrics);

#if defined(OS_ANDROID)
  return std::make_unique<PlatformDisplayDefault>(root, metrics,
                                                  nullptr /* image_cursors */);
#else
  return std::make_unique<PlatformDisplayDefault>(
      root, metrics, threaded_image_cursors_factory->CreateCursors());
#endif
}

// static
std::unique_ptr<PlatformWindow> PlatformDisplay::CreatePlatformWindow(
    PlatformWindowDelegate* delegate,
    const gfx::Rect& bounds) {
  DCHECK(!bounds.size().IsEmpty());
  std::unique_ptr<PlatformWindow> platform_window;
#if defined(OS_WIN)
  platform_window = std::make_unique<ui::WinWindow>(delegate, bounds);
#elif defined(USE_X11)
  platform_window = std::make_unique<ui::X11Window>(delegate, bounds);
#elif defined(OS_ANDROID)
  platform_window = std::make_unique<ui::PlatformWindowAndroid>(delegate);
  platform_window->SetBounds(bounds);
#elif defined(USE_OZONE)
  platform_window =
      OzonePlatform::GetInstance()->CreatePlatformWindow(delegate, bounds);
#else
  NOTREACHED() << "Unsupported platform";
#endif
  return platform_window;
}

}  // namespace ws
}  // namespace ui
