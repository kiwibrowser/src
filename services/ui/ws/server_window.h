// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_SERVER_WINDOW_H_
#define SERVICES_UI_WS_SERVER_WINDOW_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/host/host_frame_sink_client.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws/ids.h"
#include "services/viz/privileged/interfaces/compositing/display_private.mojom.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "ui/base/window_tracker_template.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/transform.h"
#include "ui/platform_window/text_input_state.h"

namespace ui {
namespace ws {

class ServerWindowDelegate;
class ServerWindowObserver;

// Server side representation of a window. Delegate is informed of interesting
// events.
//
// It is assumed that all functions that mutate the tree have validated the
// mutation is possible before hand. For example, Reorder() assumes the supplied
// window is a child and not already in position.
//
// ServerWindows do not own their children. If you delete a window that has
// children the children are implicitly removed. Similarly if a window has a
// parent and the window is deleted the deleted window is implicitly removed
// from the parent.
class ServerWindow : public viz::HostFrameSinkClient {
 public:
  using Properties = std::map<std::string, std::vector<uint8_t>>;
  using Windows = std::vector<ServerWindow*>;

  // |frame_sink_id| needs to be an input here as we are creating frame_sink_id_
  // based on the ClientWindowId clients provided.
  ServerWindow(ServerWindowDelegate* delegate,
               const viz::FrameSinkId& frame_sink_id,
               const Properties& properties = Properties());
  ~ServerWindow() override;

  void AddObserver(ServerWindowObserver* observer);
  void RemoveObserver(ServerWindowObserver* observer);
  bool HasObserver(ServerWindowObserver* observer);

  // Creates a new CompositorFrameSink of the specified type, replacing the
  // existing.
  void CreateRootCompositorFrameSink(
      gfx::AcceleratedWidget widget,
      viz::mojom::CompositorFrameSinkAssociatedRequest sink_request,
      viz::mojom::CompositorFrameSinkClientPtr client,
      viz::mojom::DisplayPrivateAssociatedRequest display_request,
      viz::mojom::DisplayClientPtr display_client);

  void CreateCompositorFrameSink(
      viz::mojom::CompositorFrameSinkRequest request,
      viz::mojom::CompositorFrameSinkClientPtr client);

  // Id of the tree that owns and created this window.
  ClientSpecificId owning_tree_id() const { return owning_tree_id_; }

  const viz::FrameSinkId& frame_sink_id() const { return frame_sink_id_; }
  void UpdateFrameSinkId(const viz::FrameSinkId& frame_sink_id);

  const base::Optional<viz::LocalSurfaceId>& current_local_surface_id() const {
    return current_local_surface_id_;
  }

  // Add the child to this window.
  void Add(ServerWindow* child);

  // Removes the child window, but does not delete it.
  void Remove(ServerWindow* child);

  // Removes all child windows from this window, but does not delete them.
  void RemoveAllChildren();

  void Reorder(ServerWindow* relative, mojom::OrderDirection diretion);
  void StackChildAtBottom(ServerWindow* child);
  void StackChildAtTop(ServerWindow* child);

  const gfx::Rect& bounds() const { return bounds_; }
  // Sets the bounds. If the size changes this implicitly resets the client
  // area to fill the whole bounds.
  void SetBounds(const gfx::Rect& bounds,
                 const base::Optional<viz::LocalSurfaceId>& local_surface_id =
                     base::nullopt);

  const std::vector<gfx::Rect>& additional_client_areas() const {
    return additional_client_areas_;
  }
  const gfx::Insets& client_area() const { return client_area_; }
  void SetClientArea(const gfx::Insets& insets,
                     const std::vector<gfx::Rect>& additional_client_areas);

  const gfx::Rect* hit_test_mask() const { return hit_test_mask_.get(); }
  void SetHitTestMask(const gfx::Rect& mask);
  void ClearHitTestMask();

  bool can_accept_drops() const { return accepts_drops_; }
  void SetCanAcceptDrops(bool accepts_drags);

  const ui::CursorData& cursor() const { return cursor_; }
  const ui::CursorData& non_client_cursor() const { return non_client_cursor_; }

  const ServerWindow* parent() const { return parent_; }
  ServerWindow* parent() { return parent_; }

  // Returns the root window used in checking drawn status. This is not
  // necessarily the same as the root window used in event dispatch.
  // NOTE: this returns null if the window does not have an ancestor associated
  // with a display.
  const ServerWindow* GetRootForDrawn() const;
  ServerWindow* GetRootForDrawn() {
    return const_cast<ServerWindow*>(
        const_cast<const ServerWindow*>(this)->GetRootForDrawn());
  }

  const Windows& children() const { return children_; }

  // Transient window management.
  // Adding transient child fails if the child window is modal to system.
  bool AddTransientWindow(ServerWindow* child);
  void RemoveTransientWindow(ServerWindow* child);

  ServerWindow* transient_parent() { return transient_parent_; }
  const ServerWindow* transient_parent() const { return transient_parent_; }

  const Windows& transient_children() const { return transient_children_; }

  // Returns true if |this| is a transient descendant of |window|.
  bool HasTransientAncestor(const ServerWindow* window) const;

  ModalType modal_type() const { return modal_type_; }
  void SetModalType(ModalType modal_type);

  void SetChildModalParent(ServerWindow* modal_parent);
  const ServerWindow* GetChildModalParent() const;
  ServerWindow* GetChildModalParent() {
    return const_cast<ServerWindow*>(
        const_cast<const ServerWindow*>(this)->GetChildModalParent());
  }

  // Returns true if this contains |window| or is |window|.
  bool Contains(const ServerWindow* window) const;

  // Returns the visibility requested by this window. IsDrawn() returns whether
  // the window is actually visible on screen.
  bool visible() const { return visible_; }
  void SetVisible(bool value);

  float opacity() const { return opacity_; }
  void SetOpacity(float value);

  void SetCursor(ui::CursorData cursor);
  void SetNonClientCursor(ui::CursorData cursor);

  const gfx::Transform& transform() const { return transform_; }
  void SetTransform(const gfx::Transform& transform);

  const std::map<std::string, std::vector<uint8_t>>& properties() const {
    return properties_;
  }
  void SetProperty(const std::string& name, const std::vector<uint8_t>* value);

  std::string GetName() const;

  void SetTextInputState(const ui::TextInputState& state);
  const ui::TextInputState& text_input_state() const {
    return text_input_state_;
  }

  void set_can_focus(bool can_focus) { can_focus_ = can_focus; }
  bool can_focus() const { return can_focus_; }

  void set_is_activation_parent(bool value) { is_activation_parent_ = value; }
  bool is_activation_parent() const { return is_activation_parent_; }

  bool has_created_compositor_frame_sink() const {
    return has_created_compositor_frame_sink_;
  }

  void set_event_targeting_policy(mojom::EventTargetingPolicy policy) {
    event_targeting_policy_ = policy;
  }
  mojom::EventTargetingPolicy event_targeting_policy() const {
    return event_targeting_policy_;
  }

  // Returns true if this window is attached to a root and all ancestors are
  // visible.
  bool IsDrawn() const;

  mojom::ShowState GetShowState() const;

  const gfx::Insets& extended_mouse_hit_test_region() const {
    return extended_mouse_hit_test_region_;
  }
  const gfx::Insets& extended_touch_hit_test_region() const {
    return extended_touch_hit_test_region_;
  }
  void set_extended_hit_test_regions_for_children(
      const gfx::Insets& mouse_insets,
      const gfx::Insets& touch_insets) {
    extended_mouse_hit_test_region_ = mouse_insets;
    extended_touch_hit_test_region_ = touch_insets;
  }

  // Offset of the underlay from the the window bounds (used for shadows).
  const gfx::Vector2d& underlay_offset() const { return underlay_offset_; }
  void SetUnderlayOffset(const gfx::Vector2d& offset);

  ServerWindowDelegate* delegate() { return delegate_; }

  // Called when the window is no longer an embed root.
  void OnEmbeddedAppDisconnected();

#if DCHECK_IS_ON()
  std::string GetDebugWindowHierarchy() const;
  std::string GetDebugWindowInfo() const;
  void BuildDebugInfo(const std::string& depth, std::string* result) const;
#endif

 private:
  // viz::HostFrameSinkClient implementation.
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info) override;
  void OnFrameTokenChanged(uint32_t frame_token) override;

  // Implementation of removing a window. Doesn't send any notification.
  void RemoveImpl(ServerWindow* window);

  // Called when this window's stacking order among its siblings is changed.
  void OnStackingChanged();

  ServerWindowDelegate* const delegate_;
  // This is the client id of the WindowTree that owns this window. This is
  // cached as the |frame_sink_id_| may change.
  const ClientSpecificId owning_tree_id_;
  // This may change for embed windows.
  viz::FrameSinkId frame_sink_id_;
  base::Optional<viz::LocalSurfaceId> current_local_surface_id_;

  ServerWindow* parent_ = nullptr;
  Windows children_;

  // Transient window management.
  // If non-null we're actively restacking transient as the result of a
  // transient ancestor changing.
  ServerWindow* stacking_target_ = nullptr;
  ServerWindow* transient_parent_ = nullptr;
  Windows transient_children_;

  ModalType modal_type_ = MODAL_TYPE_NONE;
  bool visible_ = false;
  gfx::Rect bounds_;
  gfx::Insets client_area_;
  std::vector<gfx::Rect> additional_client_areas_;
  ui::CursorData cursor_;
  ui::CursorData non_client_cursor_;
  float opacity_ = 1.0f;
  bool can_focus_ = true;
  mojom::EventTargetingPolicy event_targeting_policy_ =
      mojom::EventTargetingPolicy::TARGET_AND_DESCENDANTS;
  gfx::Transform transform_;
  ui::TextInputState text_input_state_;

  Properties properties_;

  gfx::Vector2d underlay_offset_;

  // The hit test for windows extends outside the bounds of the window by this
  // amount.
  gfx::Insets extended_mouse_hit_test_region_;
  gfx::Insets extended_touch_hit_test_region_;

  // Mouse events outside the hit test mask don't hit the window. An empty mask
  // means all events miss the window. If null there is no mask.
  std::unique_ptr<gfx::Rect> hit_test_mask_;

  // Whether this window can be the target in a drag and drop
  // operation. Clients must opt-in to this.
  bool accepts_drops_ = false;

  base::ObserverList<ServerWindowObserver> observers_;

  // Used to track the modal parent of a child modal window.
  std::unique_ptr<WindowTrackerTemplate<ServerWindow, ServerWindowObserver>>
      child_modal_parent_tracker_;

  // Whether the children of this window can be active. Also used to determine
  // when a window is considered top level. That is, if true the children of
  // this window are considered top level windows.
  bool is_activation_parent_ = false;

  // Used by tests to know whether clients have already drawn this window.
  bool has_created_compositor_frame_sink_ = false;

  DISALLOW_COPY_AND_ASSIGN(ServerWindow);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_SERVER_WINDOW_H_
