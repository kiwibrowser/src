// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"

namespace views {

// static
DesktopWindowTreeHost* DesktopWindowTreeHost::Create(
    internal::NativeWidgetDelegate* native_widget_delegate,
    DesktopNativeWidgetAura* desktop_native_widget_aura) {
  // DesktopNativeWidgetAura is only used with mus, and MusClient sets itself
  // as the factory for both NativeWidgets and DesktopWindowTreeHosts, so this
  // should never be called.
  NOTREACHED();
  return nullptr;
}

}  // namespace views
