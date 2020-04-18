// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/ozone_platform_gbm.h"

#include <gbm.h>
#include <stdlib.h>
#include <xf86drm.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "ui/base/cursor/ozone/bitmap_cursor_factory_ozone.h"
#include "ui/base/ui_features.h"
#include "ui/events/ozone/device/device_manager.h"
#include "ui/events/ozone/evdev/event_factory_evdev.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/platform/drm/gpu/drm_device_generator.h"
#include "ui/ozone/platform/drm/gpu/drm_device_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_display_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_thread_message_proxy.h"
#include "ui/ozone/platform/drm/gpu/drm_thread_proxy.h"
#include "ui/ozone/platform/drm/gpu/gbm_surface_factory.h"
#include "ui/ozone/platform/drm/gpu/proxy_helpers.h"
#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"
#include "ui/ozone/platform/drm/gpu/screen_manager.h"
#include "ui/ozone/platform/drm/host/drm_cursor.h"
#include "ui/ozone/platform/drm/host/drm_device_connector.h"
#include "ui/ozone/platform/drm/host/drm_display_host_manager.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"
#include "ui/ozone/platform/drm/host/drm_native_display_delegate.h"
#include "ui/ozone/platform/drm/host/drm_overlay_manager.h"
#include "ui/ozone/platform/drm/host/drm_window_host.h"
#include "ui/ozone/platform/drm/host/drm_window_host_manager.h"
#include "ui/ozone/platform/drm/host/host_drm_device.h"
#include "ui/ozone/public/cursor_factory_ozone.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/ozone_switches.h"

#if BUILDFLAG(USE_XKBCOMMON)
#include "ui/events/ozone/layout/xkb/xkb_evdev_codes.h"
#include "ui/events/ozone/layout/xkb/xkb_keyboard_layout_engine.h"
#else
#include "ui/events/ozone/layout/stub/stub_keyboard_layout_engine.h"
#endif

namespace ui {

namespace {

class OzonePlatformGbm : public OzonePlatform {
 public:
  OzonePlatformGbm()
      : using_mojo_(false), single_process_(false), weak_factory_(this) {}
  ~OzonePlatformGbm() override {}

  // OzonePlatform:
  ui::SurfaceFactoryOzone* GetSurfaceFactoryOzone() override {
    return surface_factory_.get();
  }
  OverlayManagerOzone* GetOverlayManager() override {
    return overlay_manager_.get();
  }
  CursorFactoryOzone* GetCursorFactoryOzone() override {
    return cursor_factory_ozone_.get();
  }
  InputController* GetInputController() override {
    return event_factory_ozone_->input_controller();
  }
  IPC::MessageFilter* GetGpuMessageFilter() override {
    if (using_mojo_) {
      return nullptr;
    } else {
      return gpu_message_filter_.get();
    }
  }

  GpuPlatformSupportHost* GetGpuPlatformSupportHost() override {
    if (using_mojo_) {
      return drm_device_connector_.get();
    } else {
      return gpu_platform_support_host_.get();
    }
  }

  std::unique_ptr<SystemInputInjector> CreateSystemInputInjector() override {
    return event_factory_ozone_->CreateSystemInputInjector();
  }

  // In multi-process mode, this function must be executed in Viz as it sets up
  // the callbacks needed  for Mojo bindings.  In single process mode, it may be
  // called on any thread.  It must follow one of |InitializeUI| or
  // |InitializeGPU|. Invocations of this method when not using mojo will be
  // ignored. While the caller may choose to invoke this method before entering
  // the sandbox, the actual interface adding has to happen on the DRM Device
  // thread and so will be deferred until the DRM thread is running.
  void AddInterfaces(
      service_manager::BinderRegistryWithArgs<
          const service_manager::BindSourceInfo&>* registry) override {
    if (!using_mojo_)
      return;

    registry->AddInterface<ozone::mojom::DeviceCursor>(
        base::Bind(&OzonePlatformGbm::CreateDeviceCursorBinding,
                   weak_factory_.GetWeakPtr()),
        base::ThreadTaskRunnerHandle::Get());

    registry->AddInterface<ozone::mojom::DrmDevice>(
        base::Bind(&OzonePlatformGbm::CreateDrmDeviceBinding,
                   weak_factory_.GetWeakPtr()),
        base::ThreadTaskRunnerHandle::Get());
  }

  // Runs on the thread where AddInterfaces was invoked. But the endpoint is
  // always bound on the DRM thread.
  void CreateDeviceCursorBinding(
      ozone::mojom::DeviceCursorRequest request,
      const service_manager::BindSourceInfo& source_info) {
    if (drm_thread_started_)
      drm_thread_proxy_->AddBindingCursorDevice(std::move(request));
    else
      pending_cursor_requests_.push_back(std::move(request));
  }
  // Runs on the thread where AddInterfaces was invoked. But the endpoint is
  // always bound on the DRM thread.
  // service_manager::InterfaceFactory<ozone::mojom::DrmDevice>:
  void CreateDrmDeviceBinding(
      ozone::mojom::DrmDeviceRequest request,
      const service_manager::BindSourceInfo& source_info) {
    if (drm_thread_started_)
      drm_thread_proxy_->AddBindingDrmDevice(std::move(request));
    else
      pending_gpu_adapter_requests_.push_back(std::move(request));
  }

  // Runs on the thread that invoked |AddInterfaces| to drain the queue of
  // binding requests that could not be satisfied until the DRM thread is
  // available (i.e. if waiting until the sandbox has been entered.)
  void DrainBindingRequests() {
    for (auto& request : pending_cursor_requests_)
      drm_thread_proxy_->AddBindingCursorDevice(std::move(request));
    pending_cursor_requests_.clear();
    for (auto& request : pending_gpu_adapter_requests_)
      drm_thread_proxy_->AddBindingDrmDevice(std::move(request));
    pending_gpu_adapter_requests_.clear();

    drm_thread_started_ = true;
  }

  std::unique_ptr<PlatformWindow> CreatePlatformWindow(
      PlatformWindowDelegate* delegate,
      const gfx::Rect& bounds) override {
    GpuThreadAdapter* adapter = gpu_platform_support_host_.get();
    if (using_mojo_) {
      adapter = host_drm_device_.get();
    }

    std::unique_ptr<DrmWindowHost> platform_window(new DrmWindowHost(
        delegate, bounds, adapter, event_factory_ozone_.get(), cursor_.get(),
        window_manager_.get(), display_manager_.get(), overlay_manager_.get()));
    platform_window->Initialize();
    return std::move(platform_window);
  }
  std::unique_ptr<display::NativeDisplayDelegate> CreateNativeDisplayDelegate()
      override {
    return std::make_unique<DrmNativeDisplayDelegate>(display_manager_.get());
  }

  void InitializeUI(const InitParams& args) override {
    // Ozone drm can operate in four modes configured at
    // runtime. Three process modes:
    //   1. legacy mode where host and viz components communicate
    //      via param traits IPC.
    //   2. single-process mode where host and viz components
    //      communicate via in-process mojo. Single-process mode can be single
    //      or multi-threaded.
    //   3. multi-process mode where host and viz components communicate
    //      via mojo IPC.
    //
    // and 2 connection modes
    //   a. Viz is launched via content::GpuProcessHost and it notifies the
    //   ozone host when Viz becomes available. b. The ozone host uses a service
    //   manager to launch and connect to Viz.
    //
    // Combinations 1a, 2b, and 3a, and 3b are supported and expected to work.
    // Combination 1a will hopefully be deprecated and replaced with 3a.
    // Combination 2b adds undesirable code-debt and the intent is to remove it.

    single_process_ = args.single_process;
    using_mojo_ = args.using_mojo || args.connector != nullptr;
    host_thread_ = base::PlatformThread::CurrentRef();

    device_manager_ = CreateDeviceManager();
    window_manager_.reset(new DrmWindowHostManager());
    cursor_.reset(new DrmCursor(window_manager_.get()));
#if BUILDFLAG(USE_XKBCOMMON)
    KeyboardLayoutEngineManager::SetKeyboardLayoutEngine(
        std::make_unique<XkbKeyboardLayoutEngine>(xkb_evdev_code_converter_));
#else
    KeyboardLayoutEngineManager::SetKeyboardLayoutEngine(
        std::make_unique<StubKeyboardLayoutEngine>());
#endif

    event_factory_ozone_.reset(new EventFactoryEvdev(
        cursor_.get(), device_manager_.get(),
        KeyboardLayoutEngineManager::GetKeyboardLayoutEngine()));

    GpuThreadAdapter* adapter;

    if (using_mojo_) {
      host_drm_device_ = base::MakeRefCounted<HostDrmDevice>(cursor_.get());
      drm_device_connector_ = std::make_unique<DrmDeviceConnector>(
          args.connector, host_drm_device_);
      adapter = host_drm_device_.get();
    } else {
      gpu_platform_support_host_.reset(
          new DrmGpuPlatformSupportHost(cursor_.get()));
      adapter = gpu_platform_support_host_.get();
    }

    overlay_manager_.reset(
        new DrmOverlayManager(adapter, window_manager_.get()));
    display_manager_.reset(new DrmDisplayHostManager(
        adapter, device_manager_.get(), overlay_manager_.get(),
        event_factory_ozone_->input_controller()));
    cursor_factory_ozone_.reset(new BitmapCursorFactoryOzone);

    if (using_mojo_) {
      host_drm_device_->ProvideManagers(display_manager_.get(),
                                        overlay_manager_.get());
      host_drm_device_->AsyncStartDrmDevice(*drm_device_connector_);
    }
  }

  void InitializeGPU(const InitParams& args) override {
    using_mojo_ = args.using_mojo;
    gpu_task_runner_ = base::ThreadTaskRunnerHandle::Get();

    InterThreadMessagingProxy* itmp;
    if (!using_mojo_) {
      scoped_refptr<DrmThreadMessageProxy> message_proxy(
          new DrmThreadMessageProxy());
      itmp = message_proxy.get();
      gpu_message_filter_ = std::move(message_proxy);
    }

    // NOTE: Can't start the thread here since this is called before sandbox
    // initialization in multi-process Chrome.
    drm_thread_proxy_.reset(new DrmThreadProxy());

    surface_factory_.reset(new GbmSurfaceFactory(drm_thread_proxy_.get()));
    if (!using_mojo_) {
      drm_thread_proxy_->BindThreadIntoMessagingProxy(itmp);
    }

    // If InitializeGPU and InitializeUI are invoked on the same thread, startup
    // sequencing is complicated because tasks are queued on the unbound mojo
    // pipe connecting the UI (the host) to the DRM thread before the DRM thread
    // is launched above. Special case this sequence via the
    // BlockingStartDrmDevice API.
    // TODO(rjkroege): In a future when we have completed splitting Viz, it will
    // be possible to simplify this logic.
    if (using_mojo_ && single_process_) {
      CHECK(host_drm_device_)
          << "Mojo single-process mode requires a HostDrmDevice.";

      // Wait here if host and gpu are one and the same thread.
      if (host_thread_ == base::PlatformThread::CurrentRef()) {
        // One-thread exection does not permit use of the sandbox.
        AfterSandboxEntry();
        host_drm_device_->BlockingStartDrmDevice();
      }
    }
  }

  // The DRM thread needs to be started late because we need to wait for the
  // sandbox to start. This entry point in the Ozne API gives platforms
  // flexibility in handing this requirement.
  void AfterSandboxEntry() override {
    CHECK(drm_thread_proxy_) << "AfterSandboxEntry before InitializeForGPU is "
                                "invalid startup order.\n";
    // Defer the actual startup of the DRM thread to here.
    auto safe_binding_resquest_drainer = CreateSafeOnceCallback(base::BindOnce(
        &OzonePlatformGbm::DrainBindingRequests, weak_factory_.GetWeakPtr()));

    drm_thread_proxy_->StartDrmThread(std::move(safe_binding_resquest_drainer));
  }

 private:
  bool using_mojo_;
  bool single_process_;

  // Objects in the GPU process.
  std::unique_ptr<DrmThreadProxy> drm_thread_proxy_;
  std::unique_ptr<GbmSurfaceFactory> surface_factory_;
  scoped_refptr<IPC::MessageFilter> gpu_message_filter_;
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;

  // TODO(rjkroege,sadrul): Provide a more elegant solution for this issue when
  // running in single process mode.
  std::vector<ozone::mojom::DeviceCursorRequest> pending_cursor_requests_;
  std::vector<ozone::mojom::DrmDeviceRequest> pending_gpu_adapter_requests_;
  bool drm_thread_started_;

  // gpu_platform_support_host_ is the IPC bridge to the GPU process while
  // host_drm_device_ is the mojo bridge to the Viz process. Only one can be in
  // use at any time.
  // TODO(rjkroege): Remove gpu_platform_support_host_ once ozone/drm with mojo
  // has reached the stable channel.
  // A raw pointer to either |gpu_platform_support_host_| or |host_drm_device_|
  // is passed to |display_manager_| and |overlay_manager_| in IntializeUI.
  // To avoid a use after free, the following two members should be declared
  // before the two managers, so that they're deleted after them.
  std::unique_ptr<DrmGpuPlatformSupportHost> gpu_platform_support_host_;

  // Objects in the host process.
  std::unique_ptr<DrmDeviceConnector> drm_device_connector_;
  scoped_refptr<HostDrmDevice> host_drm_device_;
  base::PlatformThreadRef host_thread_;
  std::unique_ptr<DeviceManager> device_manager_;
  std::unique_ptr<BitmapCursorFactoryOzone> cursor_factory_ozone_;
  std::unique_ptr<DrmWindowHostManager> window_manager_;
  std::unique_ptr<DrmCursor> cursor_;
  std::unique_ptr<EventFactoryEvdev> event_factory_ozone_;
  std::unique_ptr<DrmDisplayHostManager> display_manager_;
  std::unique_ptr<DrmOverlayManager> overlay_manager_;

#if BUILDFLAG(USE_XKBCOMMON)
  XkbEvdevCodes xkb_evdev_code_converter_;
#endif

  base::WeakPtrFactory<OzonePlatformGbm> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(OzonePlatformGbm);
};

}  // namespace

OzonePlatform* CreateOzonePlatformGbm() {
  return new OzonePlatformGbm;
}

}  // namespace ui
