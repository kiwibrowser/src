// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_DISPLAY_MANAGER_H_
#define UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_DISPLAY_MANAGER_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
#include "ui/ozone/platform/drm/common/display_types.h"

namespace display {
class DisplayMode;
struct GammaRampRGBEntry;
}

namespace ui {

class DrmDeviceManager;
class DrmDisplay;
class ScreenManager;

class DrmGpuDisplayManager {
 public:
  DrmGpuDisplayManager(ScreenManager* screen_manager,
                       DrmDeviceManager* drm_device_manager);
  ~DrmGpuDisplayManager();

  // Returns a list of the connected displays. When this is called the list of
  // displays is refreshed.
  MovableDisplaySnapshots GetDisplays();

  // Returns all scanout formats for |widget| representing a particular display
  // controller or default display controller for kNullAcceleratedWidget.
  void GetScanoutFormats(gfx::AcceleratedWidget widget,
                         std::vector<gfx::BufferFormat>* scanout_formats);

  // Takes/releases the control of the DRM devices.
  bool TakeDisplayControl();
  void RelinquishDisplayControl();

  bool ConfigureDisplay(int64_t id,
                        const display::DisplayMode& display_mode,
                        const gfx::Point& origin);
  bool DisableDisplay(int64_t id);
  bool GetHDCPState(int64_t display_id, display::HDCPState* state);
  bool SetHDCPState(int64_t display_id, display::HDCPState state);
  void SetColorCorrection(
      int64_t id,
      const std::vector<display::GammaRampRGBEntry>& degamma_lut,
      const std::vector<display::GammaRampRGBEntry>& gamma_lut,
      const std::vector<float>& correction_matrix);

 private:
  DrmDisplay* FindDisplay(int64_t display_id);

  // Notify ScreenManager of all the displays that were present before the
  // update but are gone after the update.
  void NotifyScreenManager(
      const std::vector<std::unique_ptr<DrmDisplay>>& new_displays,
      const std::vector<std::unique_ptr<DrmDisplay>>& old_displays) const;

  ScreenManager* screen_manager_;  // Not owned.
  DrmDeviceManager* drm_device_manager_;  // Not owned.

  std::vector<std::unique_ptr<DrmDisplay>> displays_;

  DISALLOW_COPY_AND_ASSIGN(DrmGpuDisplayManager);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_GPU_DRM_GPU_DISPLAY_MANAGER_H_
