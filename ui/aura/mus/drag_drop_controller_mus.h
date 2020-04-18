// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_DRAG_DROP_CONTROLLER_MUS_H_
#define UI_AURA_MUS_DRAG_DROP_CONTROLLER_MUS_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <vector>

#include "base/observer_list.h"
#include "ui/aura/aura_export.h"
#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/window_tracker.h"
#include "ui/base/dragdrop/drag_drop_types.h"

namespace ui {
class DropTargetEvent;
class OSExchangeData;

namespace mojom {
class WindowTree;
}
}

namespace aura {

class DragDropControllerHost;
class WindowMus;

// DragDropControllerMus acts as the DragDropClient for aura as well as
// handling all drag operations from the server. Drag operations are forwarded
// to the client::DragDropDelegate set on the target Window.
class AURA_EXPORT DragDropControllerMus : public client::DragDropClient {
 public:
  DragDropControllerMus(DragDropControllerHost* drag_drop_controller_host,
                        ui::mojom::WindowTree* window_tree);
  ~DragDropControllerMus() override;

  // Returns true if a drag was initiated and |id| identifies the change if of
  // the drag.
  bool DoesChangeIdMatchDragChangeId(uint32_t id) const;

  // Forwarded from WindowTreeClient. These correspond to the functions of the
  // same name defined in ui::mojom::WindowTreeClient.
  void OnDragDropStart(std::map<std::string, std::vector<uint8_t>> data);
  uint32_t OnDragEnter(WindowMus* window,
                       uint32_t event_flags,
                       const gfx::Point& screen_location,
                       uint32_t effect_bitmask);
  uint32_t OnDragOver(WindowMus* window,
                      uint32_t event_flags,
                      const gfx::Point& screen_location,
                      uint32_t effect_bitmask);
  void OnDragLeave(WindowMus* window);
  uint32_t OnCompleteDrop(WindowMus* window,
                          uint32_t event_flags,
                          const gfx::Point& screen_location,
                          uint32_t effect_bitmask);
  void OnPerformDragDropCompleted(uint32_t action_taken);
  void OnDragDropDone();

  // Overridden from client::DragDropClient:
  int StartDragAndDrop(const ui::OSExchangeData& data,
                       Window* root_window,
                       Window* source_window,
                       const gfx::Point& screen_location,
                       int drag_operations,
                       ui::DragDropTypes::DragEventSource source) override;
  void DragCancel() override;
  bool IsDragDropInProgress() override;
  void AddObserver(client::DragDropClientObserver* observer) override;
  void RemoveObserver(client::DragDropClientObserver* observer) override;

 private:
  struct CurrentDragState;

  // Called from OnDragEnter() and OnDragOver().
  uint32_t HandleDragEnterOrOver(WindowMus* window,
                                 uint32_t event_flags,
                                 const gfx::Point& screen_location,
                                 uint32_t effect_bitmask,
                                 bool is_enter);

  std::unique_ptr<ui::DropTargetEvent> CreateDropTargetEvent(
      Window* window,
      uint32_t event_flags,
      const gfx::Point& screen_location,
      uint32_t effect_bitmask);

  DragDropControllerHost* drag_drop_controller_host_;

  ui::mojom::WindowTree* window_tree_;

  // State related to being the initiator of a drag started with
  // PerformDragDrop(). If non-null a drag was started by this client and is
  // still in progress. This references a value declared on the stack in
  // StartDragAndDrop().
  CurrentDragState* current_drag_state_ = nullptr;

  // The entire drag data payload. We receive this during the drag enter event
  // and cache it so we don't send this multiple times. This value is reset when
  // the drag is done.
  std::unique_ptr<ui::OSExchangeData> os_exchange_data_;

  // Used to track the current drop target.
  WindowTracker drop_target_window_tracker_;

  base::ObserverList<client::DragDropClientObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(DragDropControllerMus);
};

}  // namespace aura

#endif  // UI_AURA_MUS_DRAG_DROP_CONTROLLER_MUS_H_
