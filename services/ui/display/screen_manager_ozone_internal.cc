// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/display/screen_manager_ozone_internal.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/system/devicemode.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/ui/display/output_protection.h"
#include "ui/display/manager/default_touch_transform_setter.h"
#include "ui/display/manager/display_change_observer.h"
#include "ui/display/manager/display_layout_store.h"
#include "ui/display/manager/display_manager_utilities.h"
#include "ui/display/manager/touch_transform_controller.h"
#include "ui/display/screen.h"
#include "ui/display/screen_base.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/display/types/fake_display_controller.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/ozone/public/ozone_platform.h"

namespace display {

// static
std::unique_ptr<ScreenManager> ScreenManager::Create() {
  return std::make_unique<ScreenManagerOzoneInternal>();
}

ScreenManagerOzoneInternal::ScreenManagerOzoneInternal()
    : screen_owned_(std::make_unique<ScreenBase>()),
      screen_(screen_owned_.get()) {
  Screen::SetScreenInstance(screen_owned_.get());
}

ScreenManagerOzoneInternal::~ScreenManagerOzoneInternal() {
  // We are shutting down and don't want to make anymore display changes.
  fake_display_controller_ = nullptr;

  // At this point |display_manager_| likely owns the Screen instance. It never
  // cleans up the instance pointer though, which could cause problems in tests.
  Screen::SetScreenInstance(nullptr);

  touch_transform_controller_.reset();

  if (display_manager_)
    display_manager_->RemoveObserver(this);

  if (display_change_observer_) {
    display_configurator_.RemoveObserver(display_change_observer_.get());
    display_change_observer_.reset();
  }

  if (display_manager_)
    display_manager_.reset();
}

void ScreenManagerOzoneInternal::SetPrimaryDisplayId(int64_t display_id) {
  DCHECK_NE(kInvalidDisplayId, display_id);
  if (primary_display_id_ == display_id)
    return;

  const Display& new_primary_display =
      display_manager_->GetDisplayForId(display_id);
  if (!new_primary_display.is_valid()) {
    LOG(ERROR) << "Invalid or non-existent display is requested:"
               << new_primary_display.ToString();
    return;
  }

  int64_t old_primary_display_id = primary_display_id_;

  const DisplayLayout& layout = display_manager_->GetCurrentDisplayLayout();
  // The requested primary id can be same as one in the stored layout
  // when the primary id is set after new displays are connected.
  // Only update the layout if it is requested to swap primary display.
  if (layout.primary_id != new_primary_display.id()) {
    std::unique_ptr<display::DisplayLayout> swapped_layout = layout.Copy();
    swapped_layout->SwapPrimaryDisplay(new_primary_display.id());
    DisplayIdList list = display_manager_->GetCurrentDisplayIdList();
    display_manager_->layout_store()->RegisterLayoutForDisplayIdList(
        list, std::move(swapped_layout));
  }

  primary_display_id_ = new_primary_display.id();
  screen_->display_list().UpdateDisplay(new_primary_display,
                                        DisplayList::Type::PRIMARY);

  // Force updating display bounds for new primary display.
  display_manager_->set_force_bounds_changed(true);
  display_manager_->UpdateDisplays();
  display_manager_->set_force_bounds_changed(false);

  DVLOG(1) << "Primary display changed from " << old_primary_display_id
           << " to " << primary_display_id_;
  delegate_->OnPrimaryDisplayChanged(primary_display_id_);
}

void ScreenManagerOzoneInternal::AddInterfaces(
    service_manager::BinderRegistryWithArgs<
        const service_manager::BindSourceInfo&>* registry) {
  registry->AddInterface<mojom::DisplayController>(
      base::Bind(&ScreenManagerOzoneInternal::BindDisplayControllerRequest,
                 base::Unretained(this)));
  registry->AddInterface<mojom::OutputProtection>(
      base::Bind(&ScreenManagerOzoneInternal::BindOutputProtectionRequest,
                 base::Unretained(this)));
  registry->AddInterface<mojom::DevDisplayController>(
      base::Bind(&ScreenManagerOzoneInternal::BindDevDisplayControllerRequest,
                 base::Unretained(this)));
}

void ScreenManagerOzoneInternal::Init(ScreenManagerDelegate* delegate) {
  DCHECK(delegate);
  delegate_ = delegate;

  // Tests may inject a NativeDisplayDelegate, otherwise get it from
  // OzonePlatform.
  if (!native_display_delegate_) {
    native_display_delegate_ =
        ui::OzonePlatform::GetInstance()->CreateNativeDisplayDelegate();
  }

  // The FakeDisplayController gives us a way to make the NativeDisplayDelegate
  // pretend something display related has happened.
  if (!chromeos::IsRunningAsSystemCompositor()) {
    fake_display_controller_ =
        native_display_delegate_->GetFakeDisplayController();
  }

  // Configure display manager. ScreenManager acts as an observer to find out
  // display changes and as a delegate to find out when changes start/stop.
  display_manager_ = std::make_unique<DisplayManager>(std::move(screen_owned_));
  display_manager_->set_configure_displays(true);
  display_manager_->AddObserver(this);
  display_manager_->set_delegate(this);

  // DisplayChangeObserver observes DisplayConfigurator and sends updates to
  // DisplayManager.
  display_change_observer_ = std::make_unique<DisplayChangeObserver>(
      &display_configurator_, display_manager_.get());

  // We want display configuration to happen even off device to keep the control
  // flow similar.
  display_configurator_.set_configure_display(true);
  display_configurator_.AddObserver(display_change_observer_.get());
  display_configurator_.set_state_controller(display_change_observer_.get());
  display_configurator_.set_mirroring_controller(display_manager_.get());

  // Perform initial configuration.
  display_configurator_.Init(std::move(native_display_delegate_), false);
  display_configurator_.ForceInitialConfigure();

  touch_transform_controller_ = std::make_unique<TouchTransformController>(
      &display_configurator_, display_manager_.get(),
      std::make_unique<display::DefaultTouchTransformSetter>());
}

void ScreenManagerOzoneInternal::RequestCloseDisplay(int64_t display_id) {
  if (!fake_display_controller_)
    return;

  // Tell the NDD to remove the display. ScreenManager will get an update
  // that the display configuration has changed and the display will be gone.
  fake_display_controller_->RemoveDisplay(display_id);
}

display::ScreenBase* ScreenManagerOzoneInternal::GetScreen() {
  return screen_;
}

void ScreenManagerOzoneInternal::ToggleAddRemoveDisplay() {
  if (!fake_display_controller_)
    return;
  DVLOG(1) << "ToggleAddRemoveDisplay";

  int num_displays = display_manager_->GetNumDisplays();
  if (num_displays == 1) {
    const gfx::Size& pixel_size =
        display_manager_->GetDisplayInfo(display_manager_->GetDisplayAt(0).id())
            .bounds_in_native()
            .size();
    fake_display_controller_->AddDisplay(pixel_size);
  } else if (num_displays > 1) {
    DisplayIdList displays = display_manager_->GetCurrentDisplayIdList();
    fake_display_controller_->RemoveDisplay(displays.back());
  }
}

void ScreenManagerOzoneInternal::IncreaseInternalDisplayZoom() {
  if (Display::HasInternalDisplay())
    display_manager_->ZoomInternalDisplay(false);
}

void ScreenManagerOzoneInternal::DecreaseInternalDisplayZoom() {
  if (Display::HasInternalDisplay())
    display_manager_->ZoomInternalDisplay(true);
}

void ScreenManagerOzoneInternal::ResetInternalDisplayZoom() {
  if (Display::HasInternalDisplay())
    display_manager_->ResetInternalDisplayZoom();
}

void ScreenManagerOzoneInternal::RotateCurrentDisplayCW() {
  NOTIMPLEMENTED();
}

void ScreenManagerOzoneInternal::SwapPrimaryDisplay() {
  // Can't swap if there is only 1 display and swapping isn't supported for 3 or
  // more displays.
  if (display_manager_->GetNumDisplays() != 2)
    return;

  DVLOG(1) << "SwapPrimaryDisplay()";

  DisplayIdList display_ids = display_manager_->GetCurrentDisplayIdList();

  // Find the next primary display.
  if (primary_display_id_ == display_ids[0])
    SetPrimaryDisplayId(display_ids[1]);
  else
    SetPrimaryDisplayId(display_ids[0]);
}

void ScreenManagerOzoneInternal::ToggleMirrorMode() {
  NOTIMPLEMENTED();
}

void ScreenManagerOzoneInternal::SetDisplayWorkArea(int64_t display_id,
                                                    const gfx::Size& size,
                                                    const gfx::Insets& insets) {
  // TODO(kylechar): Check the size of the display matches the current size.
  display_manager_->UpdateWorkAreaOfDisplay(display_id, insets);
}

void ScreenManagerOzoneInternal::TakeDisplayControl(
    const TakeDisplayControlCallback& callback) {
  display_configurator_.TakeControl(callback);
}

void ScreenManagerOzoneInternal::RelinquishDisplayControl(
    const RelinquishDisplayControlCallback& callback) {
  display_configurator_.RelinquishControl(callback);
}

void ScreenManagerOzoneInternal::OnDisplayAdded(const Display& display) {
  ViewportMetrics metrics = GetViewportMetricsForDisplay(display);
  DVLOG(1) << "OnDisplayAdded: " << display.ToString() << "\n  "
           << metrics.ToString();
  screen_->display_list().AddDisplay(display, DisplayList::Type::NOT_PRIMARY);
  delegate_->OnDisplayAdded(display, metrics);
}

void ScreenManagerOzoneInternal::OnDisplayRemoved(const Display& display) {
  // TODO(kylechar): If we're removing the primary display we need to first set
  // a new primary display. This will crash until then.

  DVLOG(1) << "OnDisplayRemoved: " << display.ToString();
  screen_->display_list().RemoveDisplay(display.id());
  delegate_->OnDisplayRemoved(display.id());
}

void ScreenManagerOzoneInternal::OnDisplayMetricsChanged(
    const Display& display,
    uint32_t changed_metrics) {
  ViewportMetrics metrics = GetViewportMetricsForDisplay(display);
  DVLOG(1) << "OnDisplayModified: " << display.ToString() << "\n  "
           << metrics.ToString();
  screen_->display_list().UpdateDisplay(display);
  delegate_->OnDisplayModified(display, metrics);
}

ViewportMetrics ScreenManagerOzoneInternal::GetViewportMetricsForDisplay(
    const Display& display) {
  const ManagedDisplayInfo& managed_info =
      display_manager_->GetDisplayInfo(display.id());

  ViewportMetrics metrics;
  // TODO(kylechar): The origin of |metrics.bounds_in_pixels| should be updated
  // so that PlatformWindows appear next to one another for multiple displays.
  metrics.bounds_in_pixels = managed_info.bounds_in_native();
  metrics.device_scale_factor = display.device_scale_factor();
  metrics.ui_scale_factor = managed_info.configured_ui_scale();

  return metrics;
}

void ScreenManagerOzoneInternal::CreateOrUpdateMirroringDisplay(
    const DisplayInfoList& display_info_list) {
  NOTIMPLEMENTED();
}

void ScreenManagerOzoneInternal::CloseMirroringDisplayIfNotNecessary() {
  NOTIMPLEMENTED();
}

void ScreenManagerOzoneInternal::PreDisplayConfigurationChange(
    bool clear_focus) {
  DVLOG(1) << "PreDisplayConfigurationChange";
}

void ScreenManagerOzoneInternal::PostDisplayConfigurationChange() {
  // Set primary display if not set yet.
  if (primary_display_id_ == kInvalidDisplayId) {
    const Display& primary_display =
        display_manager_->GetPrimaryDisplayCandidate();
    if (primary_display.is_valid()) {
      primary_display_id_ = primary_display.id();
      DVLOG(1) << "Set primary display to " << primary_display_id_;
      screen_->display_list().UpdateDisplay(primary_display,
                                            DisplayList::Type::PRIMARY);
      delegate_->OnPrimaryDisplayChanged(primary_display_id_);
    }
  }

  touch_transform_controller_->UpdateTouchTransforms();

  DVLOG(1) << "PostDisplayConfigurationChange";
}

DisplayConfigurator* ScreenManagerOzoneInternal::display_configurator() {
  return &display_configurator_;
}

void ScreenManagerOzoneInternal::BindDisplayControllerRequest(
    mojom::DisplayControllerRequest request,
    const service_manager::BindSourceInfo& source_info) {
  controller_bindings_.AddBinding(this, std::move(request));
}

void ScreenManagerOzoneInternal::BindOutputProtectionRequest(
    mojom::OutputProtectionRequest request,
    const service_manager::BindSourceInfo& source_info) {
  mojo::MakeStrongBinding(
      std::make_unique<OutputProtection>(display_configurator()),
      std::move(request));
}

void ScreenManagerOzoneInternal::BindDevDisplayControllerRequest(
    mojom::DevDisplayControllerRequest request,
    const service_manager::BindSourceInfo& source_info) {
  test_bindings_.AddBinding(this, std::move(request));
}

}  // namespace display
