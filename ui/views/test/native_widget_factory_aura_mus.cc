// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/native_widget_factory.h"

#include "ui/aura/mus/window_tree_host_mus_init_params.h"
#include "ui/views/mus/desktop_window_tree_host_mus.h"
#include "ui/views/mus/mus_client.h"
#include "ui/views/test/test_platform_native_widget.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/native_widget_aura.h"

namespace views {
namespace test {
namespace {

NativeWidget* CreatePlatformNativeWidgetImplAuraMus(
    bool create_desktop_native_widget_aura,
    const Widget::InitParams& init_params,
    Widget* widget,
    int32_t type,
    bool* destroyed) {
  if (!create_desktop_native_widget_aura) {
    return new TestPlatformNativeWidget<NativeWidgetAura>(
        widget, type == kStubCapture, destroyed);
  }
  DesktopNativeWidgetAura* desktop_native_widget_aura =
      new TestPlatformNativeWidget<DesktopNativeWidgetAura>(
          widget, type == kStubCapture, destroyed);
  std::map<std::string, std::vector<uint8_t>> mus_properties =
      MusClient::Get()->ConfigurePropertiesFromParams(init_params);
  aura::WindowTreeHostMusInitParams window_tree_host_init_params =
      aura::CreateInitParamsForTopLevel(MusClient::Get()->window_tree_client(),
                                        std::move(mus_properties));
  desktop_native_widget_aura->SetDesktopWindowTreeHost(
      std::make_unique<DesktopWindowTreeHostMus>(
          std::move(window_tree_host_init_params), widget,
          desktop_native_widget_aura));
  return desktop_native_widget_aura;
}

}  // namespace

NativeWidget* CreatePlatformNativeWidgetImpl(
    const Widget::InitParams& init_params,
    Widget* widget,
    uint32_t type,
    bool* destroyed) {
  // Only create a NativeWidgetAura if necessary, otherwise use
  // DesktopNativeWidgetAura.
  return CreatePlatformNativeWidgetImplAuraMus(
      MusClient::ShouldCreateDesktopNativeWidgetAura(init_params), init_params,
      widget, type, destroyed);
}

NativeWidget* CreatePlatformDesktopNativeWidgetImpl(
    const Widget::InitParams& init_params,
    Widget* widget,
    bool* destroyed) {
  return CreatePlatformNativeWidgetImplAuraMus(true, init_params, widget,
                                               kDefault, destroyed);
}

}  // namespace test
}  // namespace views
