// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_HOST_DRM_DEVICE_H_
#define UI_OZONE_PLATFORM_DRM_HOST_HOST_DRM_DEVICE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/platform/drm/host/drm_cursor.h"
#include "ui/ozone/platform/drm/host/gpu_thread_adapter.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/public/interfaces/device_cursor.mojom.h"
#include "ui/ozone/public/interfaces/drm_device.mojom.h"

namespace display {
class DisplaySnapshot;
}

namespace ui {
class DrmDisplayHostManager;
class DrmOverlayManager;
class GpuThreadObserver;
class DrmDeviceConnector;
class HostCursorProxy;

// This is the Viz host-side library for the DRM device service provided by the
// viz process.
class HostDrmDevice : public base::RefCountedThreadSafe<HostDrmDevice>,
                      public GpuThreadAdapter {
 public:
  explicit HostDrmDevice(DrmCursor* cursor);

  // Start the DRM service. Runs the |OnDrmServiceStartedCallback| when the
  // service has launched and initiates the remaining startup.
  void AsyncStartDrmDevice(const DrmDeviceConnector& connector);

  // Blocks until the DRM service has come up. Use this entry point only when
  // supporting launch of the service where the ozone UI and GPU
  // reponsibilities are performed by the same underlying thread.
  void BlockingStartDrmDevice();

  void ProvideManagers(DrmDisplayHostManager* display_manager,
                       DrmOverlayManager* overlay_manager);

  void OnGpuServiceLaunched(ui::ozone::mojom::DrmDevicePtr drm_device_ptr,
                            ui::ozone::mojom::DeviceCursorPtr cursor_ptr_ui,
                            ui::ozone::mojom::DeviceCursorPtr cursor_ptr_io);

  void OnGpuServiceLaunchedCompositor(
      ui::ozone::mojom::DrmDevicePtr drm_device_ptr_compositor);

  // GpuThreadAdapter
  void AddGpuThreadObserver(GpuThreadObserver* observer) override;
  void RemoveGpuThreadObserver(GpuThreadObserver* observer) override;
  bool IsConnected() override;

  // Services needed for DrmDisplayHostMananger.
  void RegisterHandlerForDrmDisplayHostManager(
      DrmDisplayHostManager* handler) override;
  void UnRegisterHandlerForDrmDisplayHostManager() override;

  bool GpuTakeDisplayControl() override;
  bool GpuRefreshNativeDisplays() override;
  bool GpuRelinquishDisplayControl() override;
  bool GpuAddGraphicsDevice(const base::FilePath& path,
                            base::ScopedFD fd) override;
  bool GpuRemoveGraphicsDevice(const base::FilePath& path) override;

  // Services needed for DrmOverlayManager.
  void RegisterHandlerForDrmOverlayManager(DrmOverlayManager* handler) override;
  void UnRegisterHandlerForDrmOverlayManager() override;
  bool GpuCheckOverlayCapabilities(
      gfx::AcceleratedWidget widget,
      const OverlaySurfaceCandidateList& new_params) override;

  // Services needed by DrmDisplayHost
  bool GpuConfigureNativeDisplay(int64_t display_id,
                                 const ui::DisplayMode_Params& display_mode,
                                 const gfx::Point& point) override;
  bool GpuDisableNativeDisplay(int64_t display_id) override;
  bool GpuGetHDCPState(int64_t display_id) override;
  bool GpuSetHDCPState(int64_t display_id, display::HDCPState state) override;
  bool GpuSetColorCorrection(
      int64_t display_id,
      const std::vector<display::GammaRampRGBEntry>& degamma_lut,
      const std::vector<display::GammaRampRGBEntry>& gamma_lut,
      const std::vector<float>& correction_matrix) override;

  // Services needed by DrmWindowHost
  bool GpuDestroyWindow(gfx::AcceleratedWidget widget) override;
  bool GpuCreateWindow(gfx::AcceleratedWidget widget) override;
  bool GpuWindowBoundsChanged(gfx::AcceleratedWidget widget,
                              const gfx::Rect& bounds) override;

 private:
  friend class base::RefCountedThreadSafe<HostDrmDevice>;
  ~HostDrmDevice() override;

  void HostOnGpuServiceLaunched();

  // BindInterface arranges for the drm_device_ptr to be wired up.
  void BindInterfaceDrmDevice(
      ui::ozone::mojom::DrmDevicePtr* drm_device_ptr) const;

  // BindInterface arranges for the cursor_ptr to be wired up.
  void BindInterfaceDeviceCursor(
      ui::ozone::mojom::DeviceCursorPtr* cursor_ptr) const;

  void OnDrmServiceStartedCallback(bool success);

  // TODO(rjkroege): Get rid of the need for this method in a subsequent CL.
  void PollForSingleThreadReady(int previous_delay);

  void RunObservers();

  void GpuCheckOverlayCapabilitiesCallback(
      const gfx::AcceleratedWidget& widget,
      const OverlaySurfaceCandidateList& overlays,
      const OverlayStatusList& returns) const;

  void GpuConfigureNativeDisplayCallback(int64_t display_id,
                                         bool success) const;

  void GpuRefreshNativeDisplaysCallback(
      std::vector<std::unique_ptr<display::DisplaySnapshot>> displays) const;
  void GpuDisableNativeDisplayCallback(int64_t display_id, bool success) const;
  void GpuTakeDisplayControlCallback(bool success) const;
  void GpuRelinquishDisplayControlCallback(bool success) const;
  void GpuGetHDCPStateCallback(int64_t display_id,
                               bool success,
                               display::HDCPState state) const;
  void GpuSetHDCPStateCallback(int64_t display_id, bool success) const;

  // Mojo implementation of the DrmDevice. Will be bound on the "main" thread.
  ui::ozone::mojom::DrmDevicePtr drm_device_ptr_;

  // When running under mus, this is the UI thread specific DrmDevice ptr for
  // use by the compositor.
  ui::ozone::mojom::DrmDevicePtr drm_device_ptr_compositor_;

  DrmDisplayHostManager* display_manager_;  // Not owned.
  DrmOverlayManager* overlay_manager_;      // Not owned.
  DrmCursor* cursor_;                       // Not owned.

  std::unique_ptr<HostCursorProxy> cursor_proxy_;

  THREAD_CHECKER(on_io_thread_);  // Needs to be rebound as is allocated on the
                                  // window server  thread.
  THREAD_CHECKER(on_window_server_thread_);
  // When running under mus, some entry points are used from the mus thread
  // and some are used from the ui thread. In general. In that case, the
  // on_ui_thread_ and on_window_server_thread_ will differ. In particular,
  // entry points used by the compositor use the ui thread.
  THREAD_CHECKER(on_ui_thread_);

  bool connected_ = false;
  base::ObserverList<GpuThreadObserver> gpu_thread_observers_;

  DISALLOW_COPY_AND_ASSIGN(HostDrmDevice);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_HOST_HOST_DRM_DEVICE_H_
