// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/drag_drop_controller_mus.h"

#include <map>
#include <string>
#include <vector>

#include "base/auto_reset.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/map.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "ui/aura/client/drag_drop_client_observer.h"
#include "ui/aura/client/drag_drop_delegate.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/drag_drop_controller_host.h"
#include "ui/aura/mus/mus_types.h"
#include "ui/aura/mus/os_exchange_data_provider_mus.h"
#include "ui/aura/mus/window_mus.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/dragdrop/drop_target_event.h"

// Interaction with DragDropDelegate assumes constants are the same.
static_assert(ui::DragDropTypes::DRAG_NONE == ui::mojom::kDropEffectNone,
              "Drag constants must be the same");
static_assert(ui::DragDropTypes::DRAG_MOVE == ui::mojom::kDropEffectMove,
              "Drag constants must be the same");
static_assert(ui::DragDropTypes::DRAG_COPY == ui::mojom::kDropEffectCopy,
              "Drag constants must be the same");
static_assert(ui::DragDropTypes::DRAG_LINK == ui::mojom::kDropEffectLink,
              "Drag constants must be the same");

namespace aura {

// State related to a drag initiated by this client.
struct DragDropControllerMus::CurrentDragState {
  ui::Id window_id;

  // The change id of the drag. Used to identify the drag on the server.
  uint32_t change_id;

  // The result of the drag. This is set on completion and returned to the
  // caller.
  uint32_t completed_action;

  // OSExchangeData supplied to StartDragAndDrop().
  const ui::OSExchangeData& drag_data;

  // StartDragDrop() runs a nested run loop. This closure is used to quit
  // the run loop when the drag completes.
  base::Closure runloop_quit_closure;
};

DragDropControllerMus::DragDropControllerMus(
    DragDropControllerHost* drag_drop_controller_host,
    ui::mojom::WindowTree* window_tree)
    : drag_drop_controller_host_(drag_drop_controller_host),
      window_tree_(window_tree) {}

DragDropControllerMus::~DragDropControllerMus() {}

bool DragDropControllerMus::DoesChangeIdMatchDragChangeId(uint32_t id) const {
  return current_drag_state_ && current_drag_state_->change_id == id;
}

void DragDropControllerMus::OnDragDropStart(
    std::map<std::string, std::vector<uint8_t>> data) {
  os_exchange_data_ = std::make_unique<ui::OSExchangeData>(
      std::make_unique<aura::OSExchangeDataProviderMus>(std::move(data)));
}

uint32_t DragDropControllerMus::OnDragEnter(WindowMus* window,
                                            uint32_t event_flags,
                                            const gfx::Point& screen_location,
                                            uint32_t effect_bitmask) {
  return HandleDragEnterOrOver(window, event_flags, screen_location,
                               effect_bitmask, true);
}

uint32_t DragDropControllerMus::OnDragOver(WindowMus* window,
                                           uint32_t event_flags,
                                           const gfx::Point& screen_location,
                                           uint32_t effect_bitmask) {
  return HandleDragEnterOrOver(window, event_flags, screen_location,
                               effect_bitmask, false);
}

void DragDropControllerMus::OnDragLeave(WindowMus* window) {
  if (drop_target_window_tracker_.windows().empty())
    return;
  DCHECK(window);
  Window* current_target = drop_target_window_tracker_.Pop();
  DCHECK_EQ(window->GetWindow(), current_target);
  client::GetDragDropDelegate(current_target)->OnDragExited();
}

uint32_t DragDropControllerMus::OnCompleteDrop(
    WindowMus* window,
    uint32_t event_flags,
    const gfx::Point& screen_location,
    uint32_t effect_bitmask) {
  if (drop_target_window_tracker_.windows().empty())
    return ui::mojom::kDropEffectNone;

  DCHECK(window);
  Window* current_target = drop_target_window_tracker_.Pop();
  DCHECK_EQ(window->GetWindow(), current_target);
  std::unique_ptr<ui::DropTargetEvent> event = CreateDropTargetEvent(
      window->GetWindow(), event_flags, screen_location, effect_bitmask);
  return client::GetDragDropDelegate(current_target)->OnPerformDrop(*event);
}

void DragDropControllerMus::OnPerformDragDropCompleted(uint32_t action_taken) {
  DCHECK(current_drag_state_);
  for (client::DragDropClientObserver& observer : observers_)
    observer.OnDragEnded();
  current_drag_state_->completed_action = action_taken;
  current_drag_state_->runloop_quit_closure.Run();
  current_drag_state_ = nullptr;
}

void DragDropControllerMus::OnDragDropDone() {
  os_exchange_data_.reset();
}

int DragDropControllerMus::StartDragAndDrop(
    const ui::OSExchangeData& data,
    Window* root_window,
    Window* source_window,
    const gfx::Point& screen_location,
    int drag_operations,
    ui::DragDropTypes::DragEventSource source) {
  DCHECK(!current_drag_state_);

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  WindowMus* root_window_mus = WindowMus::Get(root_window);
  const uint32_t change_id =
      drag_drop_controller_host_->CreateChangeIdForDrag(root_window_mus);
  CurrentDragState current_drag_state = {root_window_mus->server_id(),
                                         change_id, ui::mojom::kDropEffectNone,
                                         data, run_loop.QuitClosure()};

  // current_drag_state_ will be reset in |OnPerformDragDropCompleted| before
  // run_loop.Run() quits.
  current_drag_state_ = &current_drag_state;

  ui::mojom::PointerKind mojo_source = ui::mojom::PointerKind::MOUSE;
  if (source != ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE) {
    // TODO(erg): This collapses both touch and pen events to touch.
    mojo_source = ui::mojom::PointerKind::TOUCH;
  }

  std::map<std::string, std::vector<uint8_t>> drag_data =
      static_cast<const aura::OSExchangeDataProviderMus&>(data.provider())
          .GetData();

  for (client::DragDropClientObserver& observer : observers_)
    observer.OnDragStarted();

  window_tree_->PerformDragDrop(
      change_id, root_window_mus->server_id(), screen_location,
      mojo::MapToFlatMap(drag_data), *data.provider().GetDragImage().bitmap(),
      data.provider().GetDragImageOffset(), drag_operations, mojo_source);

  run_loop.Run();
  return current_drag_state.completed_action;
}

void DragDropControllerMus::DragCancel() {
  DCHECK(current_drag_state_);
  // Server will clean up drag and fail the in-flight change.
  window_tree_->CancelDragDrop(current_drag_state_->window_id);
}

bool DragDropControllerMus::IsDragDropInProgress() {
  return current_drag_state_ != nullptr;
}

void DragDropControllerMus::AddObserver(
    client::DragDropClientObserver* observer) {
  observers_.AddObserver(observer);
}

void DragDropControllerMus::RemoveObserver(
    client::DragDropClientObserver* observer) {
  observers_.RemoveObserver(observer);
}

uint32_t DragDropControllerMus::HandleDragEnterOrOver(
    WindowMus* window,
    uint32_t event_flags,
    const gfx::Point& screen_location,
    uint32_t effect_bitmask,
    bool is_enter) {
  client::DragDropDelegate* drag_drop_delegate =
      window ? client::GetDragDropDelegate(window->GetWindow()) : nullptr;
  WindowTreeHost* window_tree_host =
      window ? window->GetWindow()->GetHost() : nullptr;
  if ((!is_enter && drop_target_window_tracker_.windows().empty()) ||
      !drag_drop_delegate || !window_tree_host) {
    drop_target_window_tracker_.RemoveAll();
    return ui::mojom::kDropEffectNone;
  }
  drop_target_window_tracker_.Add(window->GetWindow());

  std::unique_ptr<ui::DropTargetEvent> event = CreateDropTargetEvent(
      window->GetWindow(), event_flags, screen_location, effect_bitmask);
  if (is_enter)
    drag_drop_delegate->OnDragEntered(*event);
  return drag_drop_delegate->OnDragUpdated(*event);
}

std::unique_ptr<ui::DropTargetEvent>
DragDropControllerMus::CreateDropTargetEvent(Window* window,
                                             uint32_t event_flags,
                                             const gfx::Point& screen_location,
                                             uint32_t effect_bitmask) {
  DCHECK(window->GetHost());
  gfx::Point root_location = screen_location;
  window->GetHost()->ConvertScreenInPixelsToDIP(&root_location);
  gfx::Point location = root_location;
  Window::ConvertPointToTarget(window->GetRootWindow(), window, &location);
  std::unique_ptr<ui::DropTargetEvent> event =
      std::make_unique<ui::DropTargetEvent>(
          current_drag_state_ ? current_drag_state_->drag_data
                              : *(os_exchange_data_.get()),
          location, root_location, effect_bitmask);
  event->set_flags(event_flags);
  ui::Event::DispatcherApi(event.get()).set_target(window);
  return event;
}

}  // namespace aura
