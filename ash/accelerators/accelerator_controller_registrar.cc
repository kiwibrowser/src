// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/accelerator_controller_registrar.h"

#include <limits>

#include "ash/accelerators/accelerator_controller.h"
#include "ash/accelerators/accelerator_ids.h"
#include "ash/accelerators/accelerator_router.h"
#include "ash/public/interfaces/event_properties.mojom.h"
#include "ash/shell.h"
#include "ash/window_manager.h"
#include "ash/wm/window_cycle_controller.h"
#include "ash/wm/window_util.h"
#include "base/logging.h"
#include "services/ui/common/accelerator_util.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/accelerator_history.h"

namespace ash {
namespace {

// Callback from registering the accelerators.
void OnAcceleratorsAdded(const std::vector<ui::Accelerator>& accelerators,
                         bool added) {
  // All our accelerators should be registered, so we expect |added| to be true.
  DCHECK(added) << "Unexpected accelerator vector registration failure.";
}

}  // namespace

AcceleratorControllerRegistrar::AcceleratorControllerRegistrar(
    WindowManager* window_manager,
    uint16_t id_namespace)
    : window_cycle_complete_accelerator_(ui::VKEY_MENU,
                                         ui::EF_NONE,
                                         ui::Accelerator::KeyState::RELEASED),
      window_cycle_cancel_accelerator_(ui::VKEY_ESCAPE, ui::EF_ALT_DOWN),
      window_manager_(window_manager),
      id_namespace_(id_namespace),
      next_id_(0),
      router_(new AcceleratorRouter) {
  window_manager_->AddAcceleratorHandler(id_namespace, this);
  RegisterWindowCycleAccelerators();
}

AcceleratorControllerRegistrar::~AcceleratorControllerRegistrar() {
  window_manager_->RemoveAcceleratorHandler(id_namespace_);

  if (!window_manager_->window_manager_client())
    return;

  // TODO(sky): consider not doing this. If we assume the destructor is called
  // during shutdown, then this is unnecessary and results in a bunch of
  // messages that are dropped.
  for (uint16_t local_id : ids_) {
    window_manager_->window_manager_client()->RemoveAccelerator(
        ComputeAcceleratorId(id_namespace_, local_id));
  }
}

ui::mojom::EventResult AcceleratorControllerRegistrar::OnAccelerator(
    uint32_t id,
    const ui::Event& event,
    base::flat_map<std::string, std::vector<uint8_t>>* properties) {
  const ui::Accelerator accelerator(*event.AsKeyEvent());
  auto iter = accelerator_to_ids_.find(accelerator);
  if (iter == accelerator_to_ids_.end()) {
    // Because of timing we may have unregistered the accelerator already,
    // ignore in that case.
    return ui::mojom::EventResult::UNHANDLED;
  }

  const Ids& ids = iter->second;
  AcceleratorController* accelerator_controller =
      Shell::Get()->accelerator_controller();
  const bool is_pre = GetAcceleratorLocalId(id) == ids.pre_id;
  if (is_pre) {
    // TODO(sky): this does not exactly match ash code. In particular ash code
    // is called for *all* key events, where as this is only called for
    // registered accelerators. This means the previous accelerator isn't the
    // same as it was in ash. We need to figure out exactly what is needed of
    // previous accelerator so that we can either register for the right set of
    // accelerators, or make WS send the previous accelerator.
    // http://crbug.com/630683.
    accelerator_controller->accelerator_history()->StoreCurrentAccelerator(
        accelerator);
    if (HandleWindowCycleAccelerator(accelerator))
      return ui::mojom::EventResult::HANDLED;
    aura::Window* target_window = wm::GetFocusedWindow();
    if (!target_window)
      target_window = Shell::GetRootWindowForNewWindows();
    DCHECK(target_window);
    if (router_->ProcessAccelerator(target_window, *(event.AsKeyEvent()),
                                    accelerator)) {
      return ui::mojom::EventResult::HANDLED;
    }
    if (accelerator_controller->IsActionForAcceleratorEnabled(accelerator)) {
      // We do have an accelerator for the key. Set a property so that the real
      // target knows we have an accelerator.
      (*properties)[mojom::kWillProcessAccelerator_KeyEventProperty] =
          std::vector<uint8_t>();
    }
    return ui::mojom::EventResult::UNHANDLED;
  }
  DCHECK_EQ(GetAcceleratorLocalId(id), ids.post_id);
  // NOTE: for post return value doesn't really matter.
  return Shell::Get()->accelerator_controller()->Process(accelerator)
             ? ui::mojom::EventResult::HANDLED
             : ui::mojom::EventResult::UNHANDLED;
}

void AcceleratorControllerRegistrar::OnAcceleratorsRegistered(
    const std::vector<ui::Accelerator>& accelerators) {
  std::vector<ui::mojom::WmAcceleratorPtr> accelerator_vector;

  for (const ui::Accelerator& accelerator : accelerators)
    AddAcceleratorToVector(accelerator, accelerator_vector);

  window_manager_->window_manager_client()->AddAccelerators(
      std::move(accelerator_vector),
      base::Bind(OnAcceleratorsAdded, accelerators));
}

void AcceleratorControllerRegistrar::OnAcceleratorUnregistered(
    const ui::Accelerator& accelerator) {
  auto iter = accelerator_to_ids_.find(accelerator);
  DCHECK(iter != accelerator_to_ids_.end());
  Ids ids = iter->second;
  accelerator_to_ids_.erase(iter);
  ids_.erase(ids.pre_id);
  ids_.erase(ids.post_id);
  DCHECK_EQ(accelerator_to_ids_.size() * 2, ids_.size());
  window_manager_->window_manager_client()->RemoveAccelerator(
      ComputeAcceleratorId(id_namespace_, ids.pre_id));
  window_manager_->window_manager_client()->RemoveAccelerator(
      ComputeAcceleratorId(id_namespace_, ids.post_id));
}

void AcceleratorControllerRegistrar::AddAcceleratorToVector(
    const ui::Accelerator& accelerator,
    std::vector<ui::mojom::WmAcceleratorPtr>& accelerator_vector) {
  Ids ids;
  if (!GenerateIds(&ids)) {
    LOG(ERROR) << "max number of accelerators registered, dropping request";
    return;
  }
  DCHECK_EQ(0u, accelerator_to_ids_.count(accelerator));
  accelerator_to_ids_[accelerator] = ids;
  DCHECK_EQ(accelerator_to_ids_.size() * 2, ids_.size());

  ui::mojom::EventMatcherPtr pre_event_matcher = ui::CreateKeyMatcher(
      static_cast<ui::mojom::KeyboardCode>(accelerator.key_code()),
      accelerator.modifiers());
  pre_event_matcher->accelerator_phase =
      ui::mojom::AcceleratorPhase::PRE_TARGET;
  pre_event_matcher->type_matcher->type =
      accelerator.key_state() == ui::Accelerator::KeyState::PRESSED
          ? ui::mojom::EventType::KEY_PRESSED
          : ui::mojom::EventType::KEY_RELEASED;

  ui::mojom::EventMatcherPtr post_event_matcher = pre_event_matcher.Clone();
  post_event_matcher->accelerator_phase =
      ui::mojom::AcceleratorPhase::POST_TARGET;

  accelerator_vector.push_back(
      ui::CreateAccelerator(ComputeAcceleratorId(id_namespace_, ids.pre_id),
                            std::move(pre_event_matcher)));

  accelerator_vector.push_back(
      ui::CreateAccelerator(ComputeAcceleratorId(id_namespace_, ids.post_id),
                            std::move(post_event_matcher)));
}

bool AcceleratorControllerRegistrar::GenerateIds(Ids* ids) {
  if (ids_.size() + 2 >= std::numeric_limits<uint16_t>::max())
    return false;
  ids->pre_id = GetNextLocalAcceleratorId();
  ids->post_id = GetNextLocalAcceleratorId();
  return true;
}

uint16_t AcceleratorControllerRegistrar::GetNextLocalAcceleratorId() {
  DCHECK_LT(ids_.size(), std::numeric_limits<uint16_t>::max());
  // Common case is we never wrap once, so this is typically cheap. Additionally
  // we expect there not to be too many accelerators.
  while (ids_.count(next_id_) > 0)
    ++next_id_;
  ids_.insert(next_id_);
  return next_id_++;
}

void AcceleratorControllerRegistrar::RegisterWindowCycleAccelerators() {
  std::vector<ui::Accelerator> accelerators;
  accelerators.push_back(window_cycle_complete_accelerator_);
  accelerators.push_back(window_cycle_cancel_accelerator_);
  OnAcceleratorsRegistered(accelerators);
}

bool AcceleratorControllerRegistrar::HandleWindowCycleAccelerator(
    const ui::Accelerator& accelerator) {
  ash::WindowCycleController* window_cycle_controller =
      Shell::Get()->window_cycle_controller();
  if (!window_cycle_controller->IsCycling())
    return false;

  if (accelerator == window_cycle_complete_accelerator_) {
    window_cycle_controller->CompleteCycling();
    return true;
  }

  if (accelerator == window_cycle_cancel_accelerator_) {
    window_cycle_controller->CancelCycling();
    return true;
  }

  return false;
}

}  // namespace ash
