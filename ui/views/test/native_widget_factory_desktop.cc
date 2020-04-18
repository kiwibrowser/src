// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/native_widget_factory.h"

#include "ui/views/test/test_platform_native_widget.h"

#if defined(USE_AURA)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/native_widget_aura.h"
#elif defined(OS_MACOSX)
#include "ui/views/widget/native_widget_mac.h"
#endif

namespace views {
namespace test {

NativeWidget* CreatePlatformNativeWidgetImpl(
    const Widget::InitParams& init_params,
    Widget* widget,
    uint32_t type,
    bool* destroyed) {
#if defined(OS_MACOSX)
  return new TestPlatformNativeWidget<NativeWidgetMac>(
      widget, type == kStubCapture, destroyed);
#else
  return new TestPlatformNativeWidget<NativeWidgetAura>(
      widget, type == kStubCapture, destroyed);
#endif
}

NativeWidget* CreatePlatformDesktopNativeWidgetImpl(
    const Widget::InitParams& init_params,
    Widget* widget,
    bool* destroyed) {
#if defined(OS_MACOSX)
  return new TestPlatformNativeWidget<NativeWidgetMac>(widget, false,
                                                       destroyed);
#elif defined(OS_CHROMEOS)
  // Chromeos only has one NativeWidgetType. Chromeos with aura-mus does not
  // compile this file.
  return new TestPlatformNativeWidget<NativeWidgetAura>(widget, false,
                                                        destroyed);
#elif defined(USE_AURA)
  return new TestPlatformNativeWidget<DesktopNativeWidgetAura>(widget, false,
                                                               destroyed);
#else
  NOTREACHED();
  return nullptr;
#endif
}

}  // namespace test
}  // namespace views
