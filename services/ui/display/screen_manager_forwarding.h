// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DISPLAY_SCREEN_MANAGER_FORWARDING_H_
#define SERVICES_UI_DISPLAY_SCREEN_MANAGER_FORWARDING_H_

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/display/screen_manager.h"
#include "ui/display/mojo/dev_display_controller.mojom.h"
#include "ui/display/mojo/native_display_delegate.mojom.h"
#include "ui/display/types/native_display_observer.h"

namespace display {

class FakeDisplayController;
class NativeDisplayDelegate;

// ScreenManager implementation that implements mojom::NativeDisplayDelegate.
// This will own a real NativeDisplayDelegate and forwards calls to and
// responses from it over Mojo.
class ScreenManagerForwarding : public ScreenManager,
                                public mojom::DevDisplayController,
                                public NativeDisplayObserver,
                                public mojom::NativeDisplayDelegate {
 public:
  enum class Mode {
    IN_WM_PROCESS,
    OWN_PROCESS,
  };

  // |in_process|  is true if the UI Service runs inside WM's process, false if
  // it runs inside its own process.
  explicit ScreenManagerForwarding(Mode mode);
  ~ScreenManagerForwarding() override;

  // ScreenManager:
  void AddInterfaces(
      service_manager::BinderRegistryWithArgs<
          const service_manager::BindSourceInfo&>* registry) override;
  void Init(ScreenManagerDelegate* delegate) override;
  void RequestCloseDisplay(int64_t display_id) override;
  display::ScreenBase* GetScreen() override;

  // NativeDisplayObserver:
  void OnConfigurationChanged() override;
  void OnDisplaySnapshotsInvalidated() override;

  // mojom::NativeDisplayDelegate:
  void Initialize(mojom::NativeDisplayObserverPtr observer,
                  InitializeCallback callback) override;
  void TakeDisplayControl(TakeDisplayControlCallback callback) override;
  void RelinquishDisplayControl(
      RelinquishDisplayControlCallback callback) override;
  void GetDisplays(GetDisplaysCallback callback) override;
  void Configure(int64_t display_id,
                 base::Optional<std::unique_ptr<display::DisplayMode>> mode,
                 const gfx::Point& origin,
                 ConfigureCallback callback) override;
  void GetHDCPState(int64_t display_id, GetHDCPStateCallback callback) override;
  void SetHDCPState(int64_t display_id,
                    display::HDCPState state,
                    SetHDCPStateCallback callback) override;
  void SetColorCorrection(
      int64_t display_id,
      const std::vector<display::GammaRampRGBEntry>& degamma_lut,
      const std::vector<display::GammaRampRGBEntry>& gamma_lut,
      const std::vector<float>& correction_matrix) override;

  // mojom::DevDisplayController:
  void ToggleAddRemoveDisplay() override;

 private:
  void BindNativeDisplayDelegateRequest(
      mojom::NativeDisplayDelegateRequest request,
      const service_manager::BindSourceInfo& source_info);
  void BindDevDisplayControllerRequest(
      mojom::DevDisplayControllerRequest request,
      const service_manager::BindSourceInfo& source_info);

  // Forwards results from GetDisplays() back with |callback|.
  void ForwardGetDisplays(GetDisplaysCallback callback,
                          const std::vector<DisplaySnapshot*>& displays);

  // Forwards results from call to Configure() back with |callback|.
  void ForwardConfigure(
      DisplaySnapshot* snapshot,
      const DisplayMode* mode,
      const gfx::Point& origin,
      mojom::NativeDisplayDelegate::ConfigureCallback callback,
      bool status);

  // True if the UI Service runs inside WM's process, false if it runs inside
  // its own process.
  const bool is_in_process_;
  std::unique_ptr<display::ScreenBase> screen_;
  mojo::Binding<mojom::NativeDisplayDelegate> binding_;
  mojom::NativeDisplayObserverPtr observer_;

  mojo::Binding<mojom::DevDisplayController> dev_controller_binding_;

  std::unique_ptr<display::NativeDisplayDelegate> native_display_delegate_;

  // Cached pointers to snapshots owned by the |native_display_delegate_|.
  base::flat_map<int64_t, DisplaySnapshot*> snapshot_map_;

  // If not null it provides a way to modify the display state when running off
  // device (eg. running mustash on Linux).
  FakeDisplayController* fake_display_controller_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ScreenManagerForwarding);
};

}  // namespace display

#endif  // SERVICES_UI_DISPLAY_SCREEN_MANAGER_FORWARDING_H_
