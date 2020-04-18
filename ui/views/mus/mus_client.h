// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_MUS_CLIENT_H_
#define UI_VIEWS_MUS_MUS_CLIENT_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/ui/public/interfaces/event_injector.mojom.h"
#include "services/ui/public/interfaces/window_server_test.mojom.h"
#include "ui/aura/client/capture_client.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/mus/window_tree_client_delegate.h"
#include "ui/views/mus/mus_export.h"
#include "ui/views/mus/screen_mus_delegate.h"
#include "ui/views/widget/widget.h"

namespace aura {
class PropertyConverter;
class Window;
class WindowTreeClient;
}

namespace base {
class SingleThreadTaskRunner;
class Thread;
}

namespace service_manager {
class Connector;
}

namespace ui {
class CursorDataFactoryOzone;
class InputDeviceClient;
}

namespace wm {
class WMState;
}

namespace views {

class DesktopNativeWidgetAura;
class MusClientObserver;
class MusPropertyMirror;
class PointerWatcherEventRouter;
class ScreenMus;

namespace internal {
class NativeWidgetDelegate;
}

// MusClient establishes a connection to mus and sets up necessary state so that
// aura and views target mus. This class is useful for typical clients, not the
// WindowManager. Most clients don't create this directly, rather use AuraInit.
class VIEWS_MUS_EXPORT MusClient : public aura::WindowTreeClientDelegate,
                                   public ScreenMusDelegate {
 public:
  struct VIEWS_MUS_EXPORT InitParams {
    InitParams();
    ~InitParams();

    // Production code should provide |connector|, |identity|, |wtc_config|
    // and an |io_task_runner| if the process already has one. Test code may
    // skip these parameters (e.g. a unit test that does not need to connect
    // to the window service does not need to provide a connector).
    service_manager::Connector* connector = nullptr;
    service_manager::Identity identity;
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner = nullptr;
    aura::WindowTreeClient::Config wtc_config =
        aura::WindowTreeClient::Config::kMash;

    // Create a wm::WMState. Some processes (e.g. the browser) may already
    // have one.
    bool create_wm_state = true;

    // Tests may need to control objects owned by MusClient.
    bool create_cursor_factory = true;
    bool bind_test_ws_interfaces = false;

    // If provided, MusClient will not create the WindowTreeClient. Not owned.
    // Must outlive MusClient.
    aura::WindowTreeClient* window_tree_client = nullptr;
  };

  // Most clients should use AuraInit, which creates a MusClient.
  explicit MusClient(const InitParams& params);
  ~MusClient() override;

  static MusClient* Get() { return instance_; }
  static bool Exists() { return instance_ != nullptr; }

  // Returns true if a DesktopNativeWidgetAura should be created given the
  // specified params. If this returns false a NativeWidgetAura should be
  // created.
  static bool ShouldCreateDesktopNativeWidgetAura(
      const Widget::InitParams& init_params);

  // Returns the properties to supply to mus when creating a window.
  static std::map<std::string, std::vector<uint8_t>>
  ConfigurePropertiesFromParams(const Widget::InitParams& init_params);

  aura::WindowTreeClient* window_tree_client() { return window_tree_client_; }

  PointerWatcherEventRouter* pointer_watcher_event_router() {
    return pointer_watcher_event_router_.get();
  }

  // Getter for type safety. Most code can use display::Screen::GetScreen().
  ScreenMus* screen() { return screen_.get(); }

  // Creates DesktopNativeWidgetAura with DesktopWindowTreeHostMus. This is
  // set as the factory function used for creating NativeWidgets when a
  //  NativeWidget has not been explicitly set.
  NativeWidget* CreateNativeWidget(const Widget::InitParams& init_params,
                                   internal::NativeWidgetDelegate* delegate);
  // Called when the capture client has been set for a window to notify
  // PointerWatcherEventRouter and CaptureSynchronizer.
  void OnCaptureClientSet(aura::client::CaptureClient* capture_client);

  // Called when the capture client will be unset for a window to notify
  // PointerWatcherEventRouter and CaptureSynchronizer.
  void OnCaptureClientUnset(aura::client::CaptureClient* capture_client);

  void AddObserver(MusClientObserver* observer);
  void RemoveObserver(MusClientObserver* observer);

  void SetMusPropertyMirror(std::unique_ptr<MusPropertyMirror> mirror);
  MusPropertyMirror* mus_property_mirror() {
    return mus_property_mirror_.get();
  }

  // Close all widgets this client knows.
  void CloseAllWidgets();

  // Returns an interface to test drawing in mus. Only available when created
  // with MusClientTestingState::CREATE_TESTING_STATE.
  ui::mojom::WindowServerTest* GetTestingInterface() const;

  // Returns an interface to inject events into the Window Service. Only
  // available when created with MusClientTestingState::CREATE_TESTING_STATE.
  ui::mojom::EventInjector* GetTestingEventInjector() const;

 private:
  friend class AuraInit;

  // Creates a DesktopWindowTreeHostMus. This is set as the factory function
  // ViewsDelegate such that if DesktopNativeWidgetAura is created without a
  // DesktopWindowTreeHost this is created.
  std::unique_ptr<DesktopWindowTreeHost> CreateDesktopWindowTreeHost(
      const Widget::InitParams& init_params,
      internal::NativeWidgetDelegate* delegate,
      DesktopNativeWidgetAura* desktop_native_widget_aura);

  // aura::WindowTreeClientDelegate:
  void OnEmbed(
      std::unique_ptr<aura::WindowTreeHostMus> window_tree_host) override;
  void OnLostConnection(aura::WindowTreeClient* client) override;
  void OnEmbedRootDestroyed(aura::WindowTreeHostMus* window_tree_host) override;
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id,
                              aura::Window* target) override;
  aura::PropertyConverter* GetPropertyConverter() override;

  // ScreenMusDelegate:
  void OnWindowManagerFrameValuesChanged() override;
  aura::Window* GetWindowAtScreenPoint(const gfx::Point& point) override;

  static MusClient* instance_;

  service_manager::Identity identity_;

  std::unique_ptr<base::Thread> io_thread_;

  base::ObserverList<MusClientObserver> observer_list_;

#if defined(USE_OZONE)
  std::unique_ptr<ui::CursorDataFactoryOzone> cursor_factory_ozone_;
#endif

  // NOTE: this may be null (creation is based on argument supplied to
  // constructor).
  std::unique_ptr<wm::WMState> wm_state_;

  std::unique_ptr<ScreenMus> screen_;

  std::unique_ptr<aura::PropertyConverter> property_converter_;
  std::unique_ptr<MusPropertyMirror> mus_property_mirror_;

  // Non-null if MusClient created the WindowTreeClient.
  std::unique_ptr<aura::WindowTreeClient> owned_window_tree_client_;

  // Never null.
  aura::WindowTreeClient* window_tree_client_;

  std::unique_ptr<PointerWatcherEventRouter> pointer_watcher_event_router_;

  // Gives services transparent remote access the InputDeviceManager.
  std::unique_ptr<ui::InputDeviceClient> input_device_client_;

  ui::mojom::WindowServerTestPtr server_test_ptr_;
  ui::mojom::EventInjectorPtr event_injector_;

  DISALLOW_COPY_AND_ASSIGN(MusClient);
};

}  // namespace views

#endif  // UI_VIEWS_MUS_MUS_CLIENT_H_
