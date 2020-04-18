// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_ACCELERATOR_CONTROLLER_REGISTRAR_H_
#define ASH_ACCELERATORS_ACCELERATOR_CONTROLLER_REGISTRAR_H_

#include <stdint.h>

#include <map>
#include <vector>

#include "ash/accelerators/accelerator_handler.h"
#include "base/macros.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/accelerator_manager_delegate.h"
#include "ui/events/mojo/event_constants.mojom.h"

namespace ash {

class AcceleratorControllerRegistrarTestApi;
class AcceleratorRouter;
class WindowManager;

// Responsible for registering accelerators created by AcceleratorController
// with the WindowManager, as well as routing handling of accelerators to
// the right place.
class AcceleratorControllerRegistrar : public AcceleratorHandler,
                                       public ui::AcceleratorManagerDelegate {
 public:
  AcceleratorControllerRegistrar(WindowManager* window_manager,
                                 uint16_t id_namespace);
  ~AcceleratorControllerRegistrar() override;

  // AcceleratorHandler:
  ui::mojom::EventResult OnAccelerator(
      uint32_t id,
      const ui::Event& event,
      base::flat_map<std::string, std::vector<uint8_t>>* properties) override;

 private:
  friend class AcceleratorControllerRegistrarTestApi;

  // Accelerators for window cycle.
  const ui::Accelerator window_cycle_complete_accelerator_;
  const ui::Accelerator window_cycle_cancel_accelerator_;

  // ui::AcceleratorManagerDelegate:
  void OnAcceleratorsRegistered(
      const std::vector<ui::Accelerator>& accelerators) override;
  void OnAcceleratorUnregistered(const ui::Accelerator& accelerator) override;

  // Generate id and add the corresponding accelerator to accelerator vector.
  // Creates a PRE_TARGET and POST_TARGET mojom accelerators for the provided
  // |accelerator| and adds them to the provided |accelerator_vector|.
  void AddAcceleratorToVector(
      const ui::Accelerator& accelerator,
      std::vector<ui::mojom::WmAcceleratorPtr>& accelerator_vector);

  // TODO(moshayedi): crbug.com/629191. Handling window cycle accelerators here
  // is just a temporary solution and we should remove these once we have a
  // proper solution.
  void RegisterWindowCycleAccelerators();
  bool HandleWindowCycleAccelerator(const ui::Accelerator& accelerator);

  // The flow of accelerators in ash is:
  // . wm::AcceleratorFilter() sees events first as it's a pre-target handler.
  // . AcceleratorFilter forwards to its delegate, which indirectly is
  //   implemented by AcceleratorRouter.
  // . AcceleratorRouter may early out, if not it calls through to
  //   AcceleratorController. This may stop propagation entirely.
  // . If focus is on a Widget, then NativeWidgetAura gets the key event, calls
  //   to Widget::OnKeyEvent(), which calls to FocusManager::OnKeyEvent(), which
  //   calls to AshFocusManagerFactory::Delegate::ProcessAccelerator() finally
  //   ending up in AcceleratorController::Process().
  // . OTOH if focus is on a content, then
  //   RenderWidgetHostViewAura::OnKeyEvent() is called and may end up consuming
  //   the event.
  //
  // To get this behavior for mash we register accelerators for both pre and
  // post. Pre gives the behavior of AcceleratorRouter and post that of
  // AshFocusManagerFactory.
  //
  // These ids all use the namespace |id_namespace_|.
  struct Ids {
    uint16_t pre_id;
    uint16_t post_id;
  };

  // Returns the next local id for an accelerator, or false if the max number of
  // accelerators have been registered.
  bool GenerateIds(Ids* ids);

  // Used internally by GenerateIds(). Returns the next local id (as well as
  // updating |ids_|). This *must* be called from GenerateIds().
  uint16_t GetNextLocalAcceleratorId();

  WindowManager* window_manager_;

  const uint16_t id_namespace_;

  // Id to use for the next accelerator.
  uint16_t next_id_;

  std::unique_ptr<AcceleratorRouter> router_;

  // Set of registered local ids.
  std::set<uint16_t> ids_;

  // Maps from accelerator to the two ids registered for it.
  std::map<ui::Accelerator, Ids> accelerator_to_ids_;

  DISALLOW_COPY_AND_ASSIGN(AcceleratorControllerRegistrar);
};

}  // namespace ash

#endif  // ASH_ACCELERATORS_ACCELERATOR_CONTROLLER_REGISTRAR_H_
