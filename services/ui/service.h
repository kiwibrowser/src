// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_SERVICE_H_
#define SERVICES_UI_SERVICE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "components/discardable_memory/public/interfaces/discardable_shared_memory_manager.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/ui/ime/ime_driver_bridge.h"
#include "services/ui/ime/ime_registrar_impl.h"
#include "services/ui/input_devices/input_device_server.h"
#include "services/ui/public/interfaces/accessibility_manager.mojom.h"
#include "services/ui/public/interfaces/clipboard.mojom.h"
#include "services/ui/public/interfaces/event_injector.mojom.h"
#include "services/ui/public/interfaces/gpu.mojom.h"
#include "services/ui/public/interfaces/ime/ime.mojom.h"
#include "services/ui/public/interfaces/screen_provider.mojom.h"
#include "services/ui/public/interfaces/user_activity_monitor.mojom.h"
#include "services/ui/public/interfaces/video_detector.mojom.h"
#include "services/ui/public/interfaces/window_manager_window_tree_factory.mojom.h"
#include "services/ui/public/interfaces/window_server_test.mojom.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/public/interfaces/window_tree_host_factory.mojom.h"
#include "services/ui/ws/window_server_delegate.h"

#if defined(OS_CHROMEOS)
#include "services/ui/input_devices/touch_device_server.h"
#include "services/ui/public/interfaces/arc.mojom.h"
#endif  // defined(OS_CHROMEOS)

namespace discardable_memory {
class DiscardableSharedMemoryManager;
}

namespace display {
class ScreenManager;
}

namespace service_manager {
class Connector;
class Identity;
}

namespace ui {

class ImageCursorsSet;
class InputDeviceController;
class PlatformEventSource;

namespace clipboard {
class ClipboardImpl;
}

namespace ws {
class AccessibilityManager;
class ThreadedImageCursorsFactory;
class WindowServer;
class WindowTreeHostFactory;
}

class Service : public service_manager::Service,
                public ws::WindowServerDelegate {
 public:
  // TODO(jamescook): Audit these. Some may be unused after the elimination of
  // "mus" mode.
  struct InitParams {
    InitParams();
    ~InitParams();

    // UI service runs in its own process (i.e. not embedded in browser or ash).
    bool running_standalone = false;

    // Can be used to load resources.
    scoped_refptr<base::SingleThreadTaskRunner> resource_runner = nullptr;

    // Can only be de-referenced on |resource_runner_|.
    base::WeakPtr<ImageCursorsSet> image_cursors_set_weak_ptr = nullptr;

    // If null Service creates a DiscardableSharedMemoryManager.
    discardable_memory::DiscardableSharedMemoryManager* memory_manager =
        nullptr;

    // Whether mus should host viz, or whether an external client (e.g. the
    // window manager) would be responsible for hosting viz.
    bool should_host_viz = true;

   private:
    DISALLOW_COPY_AND_ASSIGN(InitParams);
  };

  explicit Service(const InitParams& params);
  ~Service() override;

 private:
  // Holds InterfaceRequests received before the first WindowTreeHost Display
  // has been established.
  struct PendingRequest;

  // Attempts to initialize the resource bundle. Returns true if successful,
  // otherwise false if resources cannot be loaded.
  bool InitializeResources(service_manager::Connector* connector);

  void AddUserIfNecessary(const service_manager::Identity& remote_identity);

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // WindowServerDelegate:
  void StartDisplayInit() override;
  void OnFirstDisplayReady() override;
  void OnNoMoreDisplays() override;
  bool IsTestConfig() const override;
  void OnWillCreateTreeForWindowManager(
      bool automatically_create_display_roots) override;
  ws::ThreadedImageCursorsFactory* GetThreadedImageCursorsFactory() override;

  void BindAccessibilityManagerRequest(
      mojom::AccessibilityManagerRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindClipboardRequest(mojom::ClipboardRequest request,
                            const service_manager::BindSourceInfo& source_info);

  void BindScreenProviderRequest(
      mojom::ScreenProviderRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindGpuRequest(mojom::GpuRequest request);

  void BindIMERegistrarRequest(mojom::IMERegistrarRequest request);

  void BindIMEDriverRequest(mojom::IMEDriverRequest request);

  void BindInputDeviceServerRequest(mojom::InputDeviceServerRequest request);

  void BindUserActivityMonitorRequest(
      mojom::UserActivityMonitorRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindWindowManagerWindowTreeFactoryRequest(
      mojom::WindowManagerWindowTreeFactoryRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindWindowTreeFactoryRequest(
      mojom::WindowTreeFactoryRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindWindowTreeHostFactoryRequest(
      mojom::WindowTreeHostFactoryRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindDiscardableSharedMemoryManagerRequest(
      discardable_memory::mojom::DiscardableSharedMemoryManagerRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindWindowServerTestRequest(mojom::WindowServerTestRequest request);

  void BindEventInjectorRequest(mojom::EventInjectorRequest request);

  void BindVideoDetectorRequest(mojom::VideoDetectorRequest request);

#if defined(OS_CHROMEOS)
  void BindArcRequest(mojom::ArcRequest request);
  void BindTouchDeviceServerRequest(mojom::TouchDeviceServerRequest request);
#endif  // defined(OS_CHROMEOS)

  std::unique_ptr<ws::WindowServer> window_server_;
  std::unique_ptr<PlatformEventSource> event_source_;
  using PendingRequests = std::vector<std::unique_ptr<PendingRequest>>;
  PendingRequests pending_requests_;

  // Provides input-device information via Mojo IPC. Registers Mojo interfaces
  // and must outlive |registry_|.
  InputDeviceServer input_device_server_;

  // True if the UI Service runs runs inside its own process, false if it is
  // embedded in another process.
  const bool running_standalone_;

  std::unique_ptr<ws::ThreadedImageCursorsFactory>
      threaded_image_cursors_factory_;

  bool test_config_;

#if defined(OS_CHROMEOS)
  std::unique_ptr<InputDeviceController> input_device_controller_;
  TouchDeviceServer touch_device_server_;
#endif

  // Manages display hardware and handles display management. May register Mojo
  // interfaces and must outlive |registry_|.
  std::unique_ptr<display::ScreenManager> screen_manager_;

  IMERegistrarImpl ime_registrar_;
  IMEDriverBridge ime_driver_;

  discardable_memory::DiscardableSharedMemoryManager*
      discardable_shared_memory_manager_;

  // non-null if this created the DiscardableSharedMemoryManager. Null when
  // running in-process.
  std::unique_ptr<discardable_memory::DiscardableSharedMemoryManager>
      owned_discardable_shared_memory_manager_;

  const bool should_host_viz_;

  service_manager::BinderRegistryWithArgs<
      const service_manager::BindSourceInfo&>
      registry_with_source_info_;
  service_manager::BinderRegistry registry_;

  // Set to true in StartDisplayInit().
  bool is_gpu_ready_ = false;

  bool in_destructor_ = false;

  std::unique_ptr<clipboard::ClipboardImpl> clipboard_;
  std::unique_ptr<ws::AccessibilityManager> accessibility_;
  std::unique_ptr<ws::WindowTreeHostFactory> window_tree_host_factory_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace ui

#endif  // SERVICES_UI_SERVICE_H_
