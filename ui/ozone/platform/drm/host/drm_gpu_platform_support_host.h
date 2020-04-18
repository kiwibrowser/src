// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_PLATFORM_SUPPORT_HOST_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_PLATFORM_SUPPORT_HOST_H_

#include <vector>

#include "base/callback.h"
#include "base/observer_list.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "ui/display/types/display_constants.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
#include "ui/ozone/platform/drm/host/gpu_thread_adapter.h"
#include "ui/ozone/public/gpu_platform_support_host.h"

namespace gfx {
class Point;
}

namespace ui {

class DrmCursor;
class DrmDisplayHostMananger;
class DrmOverlayManager;
class GpuThreadObserver;

class DrmGpuPlatformSupportHost : public GpuPlatformSupportHost,
                                  public GpuThreadAdapter,
                                  public IPC::Sender {
 public:
  explicit DrmGpuPlatformSupportHost(DrmCursor* cursor);
  ~DrmGpuPlatformSupportHost() override;

  // GpuPlatformSupportHost:
  void OnGpuProcessLaunched(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& send_callback) override;
  void OnChannelDestroyed(int host_id) override;
  void OnGpuServiceLaunched(
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_runner,
      GpuHostBindInterfaceCallback binder) override;

  void OnMessageReceived(const IPC::Message& message) override;

  // IPC::Sender:
  bool Send(IPC::Message* message) override;

  // GpuThreadAdapter.
  // Core functionality.
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

  // Methods needed for DrmOverlayManager.
  // Methods for DrmOverlayManager.
  void RegisterHandlerForDrmOverlayManager(DrmOverlayManager* handler) override;
  void UnRegisterHandlerForDrmOverlayManager() override;

  // Services needed by DrmOverlayManager
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
  void OnChannelEstablished(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& send_callback);
  bool OnMessageReceivedForDrmDisplayHostManager(const IPC::Message& message);
  void OnUpdateNativeDisplays(
      const std::vector<DisplaySnapshot_Params>& displays);
  void OnDisplayConfigured(int64_t display_id, bool status);
  void OnHDCPStateReceived(int64_t display_id,
                           bool status,
                           display::HDCPState state);
  void OnHDCPStateUpdated(int64_t display_id, bool status);
  void OnTakeDisplayControl(bool status);
  void OnRelinquishDisplayControl(bool status);

  bool OnMessageReceivedForDrmOverlayManager(const IPC::Message& message);
  void OnOverlayResult(gfx::AcceleratedWidget widget,
                       const std::vector<OverlayCheck_Params>& params,
                       const std::vector<OverlayCheckReturn_Params>& returns);

  int host_id_ = -1;
  base::Lock host_id_lock_;

  scoped_refptr<base::SingleThreadTaskRunner> ui_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> send_runner_;
  base::Callback<void(IPC::Message*)> send_callback_;

  DrmDisplayHostManager* display_manager_;  // Not owned.
  DrmOverlayManager* overlay_manager_;      // Not owned.

  DrmCursor* cursor_;                              // Not owned.
  base::ObserverList<GpuThreadObserver> gpu_thread_observers_;

  base::WeakPtr<DrmGpuPlatformSupportHost> weak_ptr_;
  base::WeakPtrFactory<DrmGpuPlatformSupportHost> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DrmGpuPlatformSupportHost);
};

}  // namespace ui

#endif  // UI_OZONE_GPU_DRM_GPU_PLATFORM_SUPPORT_HOST_H_
