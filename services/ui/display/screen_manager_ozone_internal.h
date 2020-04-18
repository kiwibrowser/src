// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DISPLAY_SCREEN_MANAGER_OZONE_INTERNAL_H_
#define SERVICES_UI_DISPLAY_SCREEN_MANAGER_OZONE_INTERNAL_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/ui/display/screen_manager.h"
#include "services/ui/display/viewport_metrics.h"
#include "services/ui/public/interfaces/display/display_controller.mojom.h"
#include "services/ui/public/interfaces/display/output_protection.mojom.h"
#include "ui/display/display.h"
#include "ui/display/display_observer.h"
#include "ui/display/manager/display_configurator.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/mojo/dev_display_controller.mojom.h"
#include "ui/display/types/display_constants.h"

namespace display {

class DisplayChangeObserver;
class FakeDisplayController;
class ScreenBase;
class TouchTransformController;

// ScreenManagerOzoneInternal provides the necessary functionality to configure
// all attached physical displays on the the ozone platform when operating in
// internal window mode.
class ScreenManagerOzoneInternal : public ScreenManager,
                                   public mojom::DevDisplayController,
                                   public mojom::DisplayController,
                                   public DisplayObserver,
                                   public DisplayManager::Delegate {
 public:
  ScreenManagerOzoneInternal();
  ~ScreenManagerOzoneInternal() override;

  void SetPrimaryDisplayId(int64_t display_id);

  // ScreenManager:
  void AddInterfaces(
      service_manager::BinderRegistryWithArgs<
          const service_manager::BindSourceInfo&>* registry) override;
  void Init(ScreenManagerDelegate* delegate) override;
  void RequestCloseDisplay(int64_t display_id) override;
  display::ScreenBase* GetScreen() override;

  // mojom::DevDisplayController:
  void ToggleAddRemoveDisplay() override;

  // mojom::DisplayController:
  void IncreaseInternalDisplayZoom() override;
  void DecreaseInternalDisplayZoom() override;
  void ResetInternalDisplayZoom() override;
  void RotateCurrentDisplayCW() override;
  void SwapPrimaryDisplay() override;
  void ToggleMirrorMode() override;
  void SetDisplayWorkArea(int64_t display_id,
                          const gfx::Size& size,
                          const gfx::Insets& insets) override;
  void TakeDisplayControl(const TakeDisplayControlCallback& callback) override;
  void RelinquishDisplayControl(
      const RelinquishDisplayControlCallback& callback) override;

 private:
  friend class ScreenManagerOzoneInternalTest;

  ViewportMetrics GetViewportMetricsForDisplay(const Display& display);

  // DisplayObserver:
  void OnDisplayAdded(const Display& new_display) override;
  void OnDisplayRemoved(const Display& old_display) override;
  void OnDisplayMetricsChanged(const Display& display,
                               uint32_t changed_metrics) override;

  // DisplayManager::Delegate:
  void CreateOrUpdateMirroringDisplay(
      const DisplayInfoList& display_info_list) override;
  void CloseMirroringDisplayIfNotNecessary() override;
  void PreDisplayConfigurationChange(bool clear_focus) override;
  void PostDisplayConfigurationChange() override;
  DisplayConfigurator* display_configurator() override;

  void BindDisplayControllerRequest(
      mojom::DisplayControllerRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindOutputProtectionRequest(
      mojom::OutputProtectionRequest request,
      const service_manager::BindSourceInfo& source_info);

  void BindDevDisplayControllerRequest(
      mojom::DevDisplayControllerRequest request,
      const service_manager::BindSourceInfo& source_info);

  DisplayConfigurator display_configurator_;
  std::unique_ptr<DisplayManager> display_manager_;
  std::unique_ptr<DisplayChangeObserver> display_change_observer_;
  std::unique_ptr<TouchTransformController> touch_transform_controller_;

  // A Screen instance is created in the constructor because it might be
  // accessed early. The ownership of this object will be transfered to
  // |display_manager_| when that gets initialized.
  std::unique_ptr<ScreenBase> screen_owned_;

  // Used to add/remove/modify displays.
  ScreenBase* screen_;

  ScreenManagerDelegate* delegate_ = nullptr;

  std::unique_ptr<NativeDisplayDelegate> native_display_delegate_;

  // If not null it provides a way to modify the display state when running off
  // device (eg. running mustash on Linux).
  FakeDisplayController* fake_display_controller_ = nullptr;

  int64_t primary_display_id_ = kInvalidDisplayId;

  mojo::BindingSet<mojom::DisplayController> controller_bindings_;
  mojo::BindingSet<mojom::DevDisplayController> test_bindings_;

  DISALLOW_COPY_AND_ASSIGN(ScreenManagerOzoneInternal);
};

}  // namespace display

#endif  // SERVICES_UI_DISPLAY_SCREEN_MANAGER_OZONE_INTERNAL_H_
