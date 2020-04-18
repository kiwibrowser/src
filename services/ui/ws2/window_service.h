// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_SERVICE_H_
#define SERVICES_UI_WS2_WINDOW_SERVICE_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/ui/ime/ime_driver_bridge.h"
#include "services/ui/ime/ime_registrar_impl.h"
#include "services/ui/input_devices/input_device_server.h"
#include "services/ui/public/interfaces/clipboard.mojom.h"
#include "services/ui/public/interfaces/ime/ime.mojom.h"
#include "services/ui/public/interfaces/screen_provider.mojom.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws2/ids.h"
#include "ui/aura/mus/property_converter.h"

namespace aura {
class Window;
namespace client {
class FocusClient;
}
}

namespace gfx {
class Insets;
}

namespace ui {

namespace mojom {
class WindowTreeClient;
}

namespace ws2 {

class ClientWindow;
class GpuSupport;
class ScreenProvider;
class WindowServiceClient;
class WindowServiceDelegate;
class WindowTreeFactory;

// WindowService is the entry point into providing an implementation of
// the ui::mojom::WindowTree related mojoms on top of an aura Window hierarchy.
// A WindowServiceClient is created for each client. Typically you'll
// also create a WindowTreeFactory to handle incoming
// mojom::WindowTreeFactoryRequests.
class COMPONENT_EXPORT(WINDOW_SERVICE) WindowService
    : public service_manager::Service {
 public:
  WindowService(WindowServiceDelegate* delegate,
                std::unique_ptr<GpuSupport> gpu_support,
                aura::client::FocusClient* focus_client);
  ~WindowService() override;

  // Gets the ClientWindow for |window|, creating if necessary.
  ClientWindow* GetClientWindowForWindowCreateIfNecessary(aura::Window* window);

  // Creates a new WindowServiceClient, caller must call one of the Init()
  // functions on the returned object.
  std::unique_ptr<WindowServiceClient> CreateWindowServiceClient(
      mojom::WindowTreeClient* window_tree_client,
      bool intercepts_events);

  // Sets the window frame metrics.
  void SetFrameDecorationValues(const gfx::Insets& client_area_insets,
                                int max_title_bar_button_width);

  WindowServiceDelegate* delegate() { return delegate_; }

  aura::PropertyConverter* property_converter() { return &property_converter_; }

  aura::client::FocusClient* focus_client() { return focus_client_; }

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& remote_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle handle) override;

 private:
  void BindClipboardRequest(mojom::ClipboardRequest request);
  void BindScreenProviderRequest(mojom::ScreenProviderRequest request);
  void BindImeRegistrarRequest(mojom::IMERegistrarRequest request);
  void BindImeDriverRequest(mojom::IMEDriverRequest request);
  void BindInputDeviceServerRequest(mojom::InputDeviceServerRequest request);
  void BindWindowTreeFactoryRequest(
      ui::mojom::WindowTreeFactoryRequest request);

  WindowServiceDelegate* delegate_;

  // GpuSupport may be null in tests.
  std::unique_ptr<GpuSupport> gpu_support_;

  std::unique_ptr<ScreenProvider> screen_provider_;

  aura::client::FocusClient* focus_client_;

  service_manager::BinderRegistry registry_;

  std::unique_ptr<WindowTreeFactory> window_tree_factory_;

  // Helper used to serialize and deserialize window properties.
  aura::PropertyConverter property_converter_;

  // Provides info to InputDeviceClient users, via InputDeviceManager.
  ui::InputDeviceServer input_device_server_;

  // Id for the next WindowServiceClient.
  ClientSpecificId next_client_id_ = kWindowServerClientId + 1;

  // Id used for the next window created locally that is exposed to clients.
  ClientSpecificId next_window_id_ = 1;

  IMERegistrarImpl ime_registrar_;
  IMEDriverBridge ime_driver_;

  DISALLOW_COPY_AND_ASSIGN(WindowService);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_SERVICE_H_
