// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_COMPONENTS_AUTOCLICK_AUTOCLICK_APPLICATION_H_
#define ASH_COMPONENTS_AUTOCLICK_AUTOCLICK_APPLICATION_H_

#include <map>

#include "ash/autoclick/common/autoclick_controller_common.h"
#include "ash/autoclick/common/autoclick_controller_common_delegate.h"
#include "ash/components/autoclick/public/mojom/autoclick.mojom.h"
#include "base/macros.h"
#include "mash/public/mojom/launchable.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/service_manager/public/cpp/service.h"

namespace views {
class AuraInit;
class Widget;
}  // namespace views

namespace autoclick {

// AutoclickApplication is a mojo mini-app that implements the accessibility
// autoclick feature. The feature watches for the mouse to stop moving then
// generates a click after a short delay.
class AutoclickApplication : public service_manager::Service,
                             public mash::mojom::Launchable,
                             public mojom::AutoclickController,
                             public ash::AutoclickControllerCommonDelegate {
 public:
  AutoclickApplication();
  ~AutoclickApplication() override;

  void set_running_standalone(bool value) { running_standalone_ = value; }

 private:
  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& remote_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // mojom::Launchable:
  void Launch(uint32_t what, mash::mojom::LaunchMode how) override;

  // mojom::AutoclickController:
  void SetAutoclickDelay(uint32_t delay_in_milliseconds) override;

  void BindLaunchableRequest(mash::mojom::LaunchableRequest request);

  void BindAutoclickControllerRequest(
      mojom::AutoclickControllerRequest request);

  // ash::AutoclickControllerCommonDelegate:
  views::Widget* CreateAutoclickRingWidget(
      const gfx::Point& point_in_screen) override;
  void UpdateAutoclickRingWidget(views::Widget* widget,
                                 const gfx::Point& point_in_screen) override;
  void DoAutoclick(const gfx::Point& point_in_screen,
                   const int mouse_event_flags) override;
  void OnAutoclickCanceled() override;

  service_manager::BinderRegistry registry_;
  mojo::Binding<mash::mojom::Launchable> launchable_binding_;
  mojo::Binding<mojom::AutoclickController> autoclick_binding_;

  std::unique_ptr<views::AuraInit> aura_init_;
  std::unique_ptr<ash::AutoclickControllerCommon> autoclick_controller_common_;
  std::unique_ptr<views::Widget> widget_;

  bool running_standalone_ = false;

  DISALLOW_COPY_AND_ASSIGN(AutoclickApplication);
};

}  // namespace autoclick

#endif  // ASH_COMPONENTS_AUTOCLICK_AUTOCLICK_APPLICATION_H_
