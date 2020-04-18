// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_MUS_TEST_WINDOW_TREE_H_
#define UI_AURA_TEST_MUS_TEST_WINDOW_TREE_H_

#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "services/ui/common/types.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "ui/aura/mus/mus_types.h"

namespace display {
class DisplayManager;
}

namespace aura {

enum class WindowTreeChangeType {
  ADD_TRANSIENT,
  BOUNDS,
  // Used for both set and release capture.
  CAPTURE,
  FOCUS,
  MODAL,
  NEW_TOP_LEVEL,
  NEW_WINDOW,
  PROPERTY,
  REMOVE_TRANSIENT,
  REORDER,
  TRANSFORM,
  VISIBLE,

  // This covers all cases that aren't used in tests.
  OTHER,
};

struct TransientData {
  ui::Id parent_id;
  ui::Id child_id;
};

// WindowTree implementation for tests. TestWindowTree maintains a list of all
// calls that take a change_id and are expected to be acked back to the client.
// Various functions are provided to respond to the changes.
class TestWindowTree : public ui::mojom::WindowTree {
 public:
  TestWindowTree();
  ~TestWindowTree() override;

  void set_client(ui::mojom::WindowTreeClient* client) { client_ = client; }
  void set_window_manager(ui::mojom::WindowManager* window_manager) {
    window_manager_ = window_manager;
  }

  uint32_t window_id() const { return window_id_; }

  bool WasEventAcked(uint32_t event_id) const;

  // Returns the result of the specified event. UNHANDLED if |event_id| was
  // not acked (use WasEventAcked() to determine if the event was acked).
  ui::mojom::EventResult GetEventResult(uint32_t event_id) const;

  base::Optional<std::vector<uint8_t>> GetLastPropertyValue();

  base::Optional<base::flat_map<std::string, std::vector<uint8_t>>>
  GetLastNewWindowProperties();

  // True if at least one function has been called that takes a change id.
  bool has_change() const { return !changes_.empty(); }

  size_t number_of_changes() const { return changes_.size(); }

  // Notifies the client about the accelerated widget when mus is not hosting
  // viz.
  void NotifyClientAboutAcceleratedWidgets(
      display::DisplayManager* display_manager);

  // Pretends that there is a scheduled embed request for |token|.
  void AddScheduledEmbedToken(const base::UnguessableToken& token);

  // Pretends the other side has called EmbedUsingToken for |token|.
  void AddEmbedRootForToken(const base::UnguessableToken& token);

  // Pretends the embedder window goes away.
  void RemoveEmbedderWindow(ui::Id embedder_window_id);

  // Acks all changes with a value of true.
  void AckAllChanges();

  // Returns false if there are no, or more than one, changes of the specified
  // type. If there is only one of the matching type it is acked with a result
  // of |result| and true is returned.
  bool AckSingleChangeOfType(WindowTreeChangeType type, bool result);

  // Same as AckSingleChangeOfType(), but doesn't fail if there is more than
  // one change of the specified type.
  bool AckFirstChangeOfType(WindowTreeChangeType type, bool result);

  void AckAllChangesOfType(WindowTreeChangeType type, bool result);

  bool GetAndRemoveFirstChangeOfType(WindowTreeChangeType type,
                                     uint32_t* change_id);

  size_t GetChangeCountForType(WindowTreeChangeType type);

  // Data from the most recently added/removed transient window.
  const TransientData& transient_data() const { return transient_data_; }

  const gfx::Insets& last_client_area() const { return last_client_area_; }

  const base::Optional<gfx::Rect>& last_hit_test_mask() const {
    return last_hit_test_mask_;
  }

  const base::Optional<viz::LocalSurfaceId>& last_local_surface_id() const {
    return last_local_surface_id_;
  }

  const gfx::Rect& last_set_window_bounds() const {
    return last_set_window_bounds_;
  }

  const std::string& last_wm_action() const { return last_wm_action_; }

 private:
  struct Change {
    WindowTreeChangeType type;
    uint32_t id;
  };

  void OnChangeReceived(
      uint32_t change_id,
      WindowTreeChangeType type = WindowTreeChangeType::OTHER);

  // ui::mojom::WindowTree:
  void NewWindow(
      uint32_t change_id,
      ui::Id window_id,
      const base::Optional<base::flat_map<std::string, std::vector<uint8_t>>>&
          properties) override;
  void NewTopLevelWindow(
      uint32_t change_id,
      ui::Id window_id,
      const base::flat_map<std::string, std::vector<uint8_t>>& properties)
      override;
  void DeleteWindow(uint32_t change_id, ui::Id window_id) override;
  void SetWindowBounds(
      uint32_t change_id,
      ui::Id window_id,
      const gfx::Rect& bounds,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id) override;
  void SetWindowTransform(uint32_t change_id,
                          ui::Id window_id,
                          const gfx::Transform& transform) override;
  void SetClientArea(ui::Id window_id,
                     const gfx::Insets& insets,
                     const base::Optional<std::vector<gfx::Rect>>&
                         additional_client_areas) override;
  void SetHitTestMask(ui::Id window_id,
                      const base::Optional<gfx::Rect>& mask) override;
  void SetCanAcceptDrops(ui::Id window_id, bool accepts_drags) override;
  void SetWindowVisibility(uint32_t change_id,
                           ui::Id window_id,
                           bool visible) override;
  void SetWindowProperty(
      uint32_t change_id,
      ui::Id window_id,
      const std::string& name,
      const base::Optional<std::vector<uint8_t>>& value) override;
  void SetWindowOpacity(uint32_t change_id,
                        ui::Id window_id,
                        float opacity) override;
  void AttachCompositorFrameSink(
      ui::Id window_id,
      mojo::InterfaceRequest<viz::mojom::CompositorFrameSink> surface,
      viz::mojom::CompositorFrameSinkClientPtr client) override;
  void AddWindow(uint32_t change_id, ui::Id parent, ui::Id child) override;
  void RemoveWindowFromParent(uint32_t change_id, ui::Id window_id) override;
  void AddTransientWindow(uint32_t change_id,
                          ui::Id window_id,
                          ui::Id transient_window_id) override;
  void RemoveTransientWindowFromParent(uint32_t change_id,
                                       ui::Id window_id) override;
  void SetModalType(uint32_t change_id,
                    ui::Id window_id,
                    ui::ModalType modal_type) override;
  void SetChildModalParent(uint32_t change_id,
                           ui::Id window_id,
                           ui::Id parent_window_id) override;
  void ReorderWindow(uint32_t change_id,
                     ui::Id window_id,
                     ui::Id relative_window_id,
                     ui::mojom::OrderDirection direction) override;
  void GetWindowTree(ui::Id window_id, GetWindowTreeCallback callback) override;
  void SetCapture(uint32_t change_id, ui::Id window_id) override;
  void ReleaseCapture(uint32_t change_id, ui::Id window_id) override;
  void StartPointerWatcher(bool want_moves) override;
  void StopPointerWatcher() override;
  void Embed(ui::Id window_id,
             ui::mojom::WindowTreeClientPtr client,
             uint32_t flags,
             EmbedCallback callback) override;
  void ScheduleEmbed(ui::mojom::WindowTreeClientPtr client,
                     ScheduleEmbedCallback callback) override;
  void EmbedUsingToken(ui::Id window_id,
                       const base::UnguessableToken& token,
                       uint32_t embed_flags,
                       EmbedUsingTokenCallback callback) override;
  void ScheduleEmbedForExistingClient(
      ui::ClientSpecificId window_id,
      ScheduleEmbedForExistingClientCallback callback) override;
  void SetFocus(uint32_t change_id, ui::Id window_id) override;
  void SetCanFocus(ui::Id window_id, bool can_focus) override;
  void SetEventTargetingPolicy(ui::Id window_id,
                               ui::mojom::EventTargetingPolicy policy) override;
  void SetCursor(uint32_t change_id,
                 ui::Id transport_window_id,
                 ui::CursorData cursor_data) override;
  void SetWindowTextInputState(ui::Id window_id,
                               ui::mojom::TextInputStatePtr state) override;
  void SetImeVisibility(ui::Id window_id,
                        bool visible,
                        ui::mojom::TextInputStatePtr state) override;
  void OnWindowInputEventAck(uint32_t event_id,
                             ui::mojom::EventResult result) override;
  void DeactivateWindow(ui::Id window_id) override;
  void StackAbove(uint32_t change_id,
                  ui::Id above_id,
                  ui::Id below_id) override;
  void StackAtTop(uint32_t change_id, ui::Id window_id) override;
  void PerformWmAction(ui::Id window_id, const std::string& action) override;
  void GetWindowManagerClient(
      mojo::AssociatedInterfaceRequest<ui::mojom::WindowManagerClient> internal)
      override;
  void GetCursorLocationMemory(
      GetCursorLocationMemoryCallback callback) override;
  void PerformDragDrop(
      uint32_t change_id,
      ui::Id source_window_id,
      const gfx::Point& screen_location,
      const base::flat_map<std::string, std::vector<uint8_t>>& drag_data,
      const SkBitmap& drag_image,
      const gfx::Vector2d& drag_image_offset,
      uint32_t drag_operation,
      ui::mojom::PointerKind source) override;
  void CancelDragDrop(ui::Id window_id) override;
  void PerformWindowMove(uint32_t change_id,
                         ui::Id window_id,
                         ui::mojom::MoveLoopSource source,
                         const gfx::Point& cursor_location) override;
  void CancelWindowMove(ui::Id window_id) override;

  struct AckedEvent {
    uint32_t event_id;
    ui::mojom::EventResult result;
  };
  std::vector<AckedEvent> acked_events_;
  ui::Id window_id_ = 0u;

  base::Optional<std::vector<uint8_t>> last_property_value_;

  std::vector<Change> changes_;

  ui::mojom::WindowTreeClient* client_;
  ui::mojom::WindowManager* window_manager_ = nullptr;

  base::Optional<base::flat_map<std::string, std::vector<uint8_t>>>
      last_new_window_properties_;

  TransientData transient_data_;

  gfx::Insets last_client_area_;

  base::Optional<gfx::Rect> last_hit_test_mask_;

  base::Optional<viz::LocalSurfaceId> last_local_surface_id_;

  gfx::Rect last_set_window_bounds_;

  std::string last_wm_action_;

  // Support only one scheduled embed in test.
  base::UnguessableToken scheduled_embed_;

  DISALLOW_COPY_AND_ASSIGN(TestWindowTree);
};

}  // namespace aura

#endif  // UI_AURA_TEST_MUS_TEST_WINDOW_TREE_H_
