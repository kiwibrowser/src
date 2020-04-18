// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_TREE_H_
#define SERVICES_UI_WS_WINDOW_TREE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws/access_policy_delegate.h"
#include "services/ui/ws/async_event_dispatcher.h"
#include "services/ui/ws/drag_source.h"
#include "services/ui/ws/drag_target_connection.h"
#include "services/ui/ws/ids.h"
#include "services/ui/ws/window_tree_binding.h"
#include "services/viz/public/interfaces/compositing/surface_id.mojom.h"
#include "ui/gfx/native_widget_types.h"

namespace display {
struct ViewportMetrics;
}
namespace gfx {
class Insets;
class Rect;
}

namespace ui {
class Event;
}

namespace ui {
namespace ws {

class AccessPolicy;
class DisplayManager;
class Display;
class DragTargetConnection;
class ServerWindow;
class TargetedEvent;
class WindowManagerDisplayRoot;
class WindowManagerState;
class WindowServer;

struct EventLocation;
struct WindowTreeAndWindowId;

namespace test {
class WindowTreeTestApi;
}

// WindowTree represents a view onto portions of the window tree. The parts of
// the tree exposed to the client start at the root windows. A WindowTree may
// have any number of roots (including none). A WindowTree may not have
// visibility of all the descendants of its roots.
//
// WindowTree notifies its client as changes happen to windows exposed to the
// the client.
//
// See ids.h for details on how WindowTree handles identity of windows.
class WindowTree : public mojom::WindowTree,
                   public AccessPolicyDelegate,
                   public mojom::WindowManagerClient,
                   public DragSource,
                   public DragTargetConnection,
                   public AsyncEventDispatcher {
 public:
  WindowTree(WindowServer* window_server,
             bool is_for_embedding,
             ServerWindow* root,
             std::unique_ptr<AccessPolicy> access_policy);
  ~WindowTree() override;

  void Init(std::unique_ptr<WindowTreeBinding> binding,
            mojom::WindowTreePtr tree);

  // Called if this WindowTree hosts a WindowManager. See mojom for details
  // on |automatically_create_display_roots|.
  void ConfigureWindowManager(bool automatically_create_display_roots);

  bool automatically_create_display_roots() const {
    return automatically_create_display_roots_;
  }

  void OnAcceleratedWidgetAvailableForDisplay(Display* display);

  ClientSpecificId id() const { return id_; }

  void set_embedder_intercepts_events() { embedder_intercepts_events_ = true; }
  bool embedder_intercepts_events() const {
    return embedder_intercepts_events_;
  }

  void set_can_change_root_window_visibility(bool value) {
    can_change_root_window_visibility_ = value;
  }

  bool is_for_embedding() const { return is_for_embedding_; }

  mojom::WindowTreeClient* client() { return binding_->client(); }

  // Returns the Window with the specified client id *only* if known to this
  // client, returns null if not known.
  ServerWindow* GetWindowByClientId(const ClientWindowId& id) {
    return const_cast<ServerWindow*>(
        const_cast<const WindowTree*>(this)->GetWindowByClientId(id));
  }
  const ServerWindow* GetWindowByClientId(const ClientWindowId& id) const;

  bool IsWindowKnown(const ServerWindow* window) const {
    return IsWindowKnown(window, nullptr);
  }
  // Returns whether |window| is known to this tree. If |window| is known and
  // |client_window_id| is non-null |client_window_id| is set to the
  // ClientWindowId of the window.
  bool IsWindowKnown(const ServerWindow* window,
                     ClientWindowId* client_window_id) const;

  // Returns true if |window| is one of this trees roots.
  bool HasRoot(const ServerWindow* window) const;

  std::set<const ServerWindow*> roots() { return roots_; }

  void set_name(const std::string& name) { name_ = name; }
  const std::string& name() const { return name_; }

  bool janky() const { return janky_; }

  const Display* GetDisplay(const ServerWindow* window) const;
  Display* GetDisplay(const ServerWindow* window) {
    return const_cast<Display*>(
        const_cast<const WindowTree*>(this)->GetDisplay(window));
  }

  const WindowManagerDisplayRoot* GetWindowManagerDisplayRoot(
      const ServerWindow* window) const;
  WindowManagerDisplayRoot* GetWindowManagerDisplayRoot(
      const ServerWindow* window) {
    return const_cast<WindowManagerDisplayRoot*>(
        const_cast<const WindowTree*>(this)->GetWindowManagerDisplayRoot(
            window));
  }
  WindowManagerState* window_manager_state() {
    return window_manager_state_.get();
  }

  DisplayManager* display_manager();
  const DisplayManager* display_manager() const;

  WindowServer* window_server() { return window_server_; }
  const WindowServer* window_server() const { return window_server_; }

  // Called from ~WindowServer. Reset WindowTreeClient so that it no longer gets
  // any messages.
  void PrepareForWindowServerShutdown();

  // Adds a new root to this tree. This is only valid for window managers.
  void AddRootForWindowManager(const ServerWindow* root);

  // Invoked when a tree is about to be destroyed.
  void OnWillDestroyTree(WindowTree* tree);

  // Sends updated display information.
  void OnWmDisplayModified(const display::Display& display);

  // These functions are synchronous variants of those defined in the mojom. The
  // WindowTree implementations all call into these. See the mojom for details.
  bool SetCapture(const ClientWindowId& client_window_id);
  bool ReleaseCapture(const ClientWindowId& client_window_id);
  bool NewWindow(const ClientWindowId& client_window_id,
                 const std::map<std::string, std::vector<uint8_t>>& properties);
  bool AddWindow(const ClientWindowId& parent_id,
                 const ClientWindowId& child_id);
  bool AddTransientWindow(const ClientWindowId& window_id,
                          const ClientWindowId& transient_window_id);
  bool RemoveWindowFromParent(const ClientWindowId& window_id);
  bool DeleteWindow(const ClientWindowId& window_id);
  bool SetModalType(const ClientWindowId& window_id, ModalType modal_type);
  bool SetChildModalParent(const ClientWindowId& window_id,
                           const ClientWindowId& modal_parent_window_id);
  std::vector<const ServerWindow*> GetWindowTree(
      const ClientWindowId& window_id) const;
  bool SetWindowVisibility(const ClientWindowId& window_id, bool visible);
  bool SetWindowOpacity(const ClientWindowId& window_id, float opacity);
  bool SetFocus(const ClientWindowId& window_id);
  bool Embed(const ClientWindowId& window_id,
             mojom::WindowTreeClientPtr window_tree_client,
             uint32_t flags);

  // Called from EmbedUsingToken() to embed an existing client that previously
  // called ScheduleEmbedForExistingClient(). |tree_and_window_id.tree| is the
  // client that previously called ScheduleEmbedForExistingClient() and
  // |token| is the token returned to the client.
  bool EmbedExistingTree(const ClientWindowId& window_id,
                         const WindowTreeAndWindowId& tree_and_window_id,
                         const base::UnguessableToken& token);

  bool IsWaitingForNewTopLevelWindow(uint32_t wm_change_id);
  viz::FrameSinkId OnWindowManagerCreatedTopLevelWindow(
      uint32_t wm_change_id,
      uint32_t client_change_id,
      const ServerWindow* window);
  void AddActivationParent(const ClientWindowId& window_id);

  // Calls through to the client.
  void OnChangeCompleted(uint32_t change_id, bool success);

  // If |callback| is valid then an ack is expected from the client. When the
  // ack from the client is received |callback| is Run().
  using AcceleratorCallback = base::OnceCallback<void(
      mojom::EventResult,
      const base::flat_map<std::string, std::vector<uint8_t>>&)>;
  void OnEventOccurredOutsideOfModalWindow(const ServerWindow* modal_window);

  // Called when the cursor touch visibility bit changes. This is only called
  // on the WindowTree associated with a WindowManager.
  void OnCursorTouchVisibleChanged(bool enabled);

  // Called when a display has been removed. This is only called on the
  // WindowTree associated with a WindowManager.
  void OnDisplayDestroying(int64_t display_id);

  // Called when |tree|'s jankiness changes (see janky_ for definition).
  // Notifies the window manager client so it can update UI for the affected
  // window(s).
  void ClientJankinessChanged(WindowTree* tree);

  // The following methods are invoked after the corresponding change has been
  // processed. They do the appropriate bookkeeping and update the client as
  // necessary.
  void ProcessWindowBoundsChanged(
      const ServerWindow* window,
      const gfx::Rect& old_bounds,
      const gfx::Rect& new_bounds,
      bool originated_change,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id);
  void ProcessWindowTransformChanged(const ServerWindow* window,
                                     const gfx::Transform& old_transform,
                                     const gfx::Transform& new_transform,
                                     bool originated_change);
  void ProcessClientAreaChanged(
      const ServerWindow* window,
      const gfx::Insets& new_client_area,
      const std::vector<gfx::Rect>& new_additional_client_areas,
      bool originated_change);
  void ProcessWillChangeWindowHierarchy(const ServerWindow* window,
                                        const ServerWindow* new_parent,
                                        const ServerWindow* old_parent,
                                        bool originated_change);
  void ProcessWindowPropertyChanged(const ServerWindow* window,
                                    const std::string& name,
                                    const std::vector<uint8_t>* new_data,
                                    bool originated_change);
  void ProcessWindowHierarchyChanged(const ServerWindow* window,
                                     const ServerWindow* new_parent,
                                     const ServerWindow* old_parent,
                                     bool originated_change);
  void ProcessWindowReorder(const ServerWindow* window,
                            const ServerWindow* relative_window,
                            mojom::OrderDirection direction,
                            bool originated_change);
  void ProcessWindowDeleted(ServerWindow* window, bool originated_change);
  void ProcessWillChangeWindowVisibility(const ServerWindow* window,
                                         bool originated_change);
  void ProcessWindowOpacityChanged(const ServerWindow* window,
                                   float old_opacity,
                                   float new_opacity,
                                   bool originated_change);
  void ProcessCursorChanged(const ServerWindow* window,
                            const ui::CursorData& cursor,
                            bool originated_change);
  void ProcessFocusChanged(const ServerWindow* old_focused_window,
                           const ServerWindow* new_focused_window);
  void ProcessCaptureChanged(const ServerWindow* new_capture,
                             const ServerWindow* old_capture,
                             bool originated_change);
  void ProcessTransientWindowAdded(const ServerWindow* window,
                                   const ServerWindow* transient_window,
                                   bool originated_change);
  void ProcessTransientWindowRemoved(const ServerWindow* window,
                                     const ServerWindow* transient_window,
                                     bool originated_change);
  void ProcessWindowSurfaceChanged(ServerWindow* window,
                                   const viz::SurfaceInfo& surface_info);

  // Sends this event to the client if it matches an active pointer watcher.
  // |target_window| is the target of the event, and may be null or not known
  // to this tree.
  void SendToPointerWatcher(const ui::Event& event,
                            ServerWindow* target_window,
                            int64_t display_id);

  // Before the ClientWindowId gets sent back to the client, making sure we
  // reset the high 16 bits back to 0 if it's being sent back to the client
  // that created the window.
  Id ClientWindowIdToTransportId(const ClientWindowId& client_window_id) const;

 private:
  friend class test::WindowTreeTestApi;

  struct WaitingForTopLevelWindowInfo {
    WaitingForTopLevelWindowInfo(const ClientWindowId& client_window_id,
                                 uint32_t wm_change_id)
        : client_window_id(client_window_id), wm_change_id(wm_change_id) {}
    ~WaitingForTopLevelWindowInfo() {}

    // Id supplied from the client.
    ClientWindowId client_window_id;

    // Change id we created for the window manager.
    uint32_t wm_change_id;
  };

  enum class RemoveRootReason {
    // The window is being removed.
    DELETED,

    // Another client is being embedded in the window.
    EMBED,

    // The embedded client explicitly asked to be unembedded.
    UNEMBED,
  };

  bool ShouldRouteToWindowManager(const ServerWindow* window) const;

  Id TransportIdForWindow(const ServerWindow* window) const;

  // Returns true if |id| is a valid WindowId for a new window.
  bool IsValidIdForNewWindow(const ClientWindowId& id) const;

  // These functions return true if the corresponding mojom function is allowed
  // for this tree.
  bool CanReorderWindow(const ServerWindow* window,
                        const ServerWindow* relative_window,
                        mojom::OrderDirection direction) const;

  // Deletes a window owned by this tree. Returns true on success. |source| is
  // the tree that originated the change.
  bool DeleteWindowImpl(WindowTree* source, ServerWindow* window);

  // If |window| is known does nothing. Otherwise adds |window| to |windows| (if
  // non-null) and marks |window| as known and recurses. If |window| is not
  // known it assigned the ClientWindowId |id_for_window|, if |id_for_window|
  // is null, then the FrameSinkId of |window| is used.
  void GetUnknownWindowsFrom(const ServerWindow* window,
                             std::vector<const ServerWindow*>* windows,
                             const ClientWindowId* id_for_window);

  // Called to add |window| as an embed root. This matches a call from the
  // client to ScheduleEmbedForExistingClient(). |client_window_id| is the
  // ClientWindowId to use for |window|.
  void AddRootForToken(const base::UnguessableToken& token,
                       ServerWindow* window,
                       const ClientWindowId& client_window_id);

  void AddToMaps(const ServerWindow* window,
                 const ClientWindowId& client_window_id);

  // Removes |window| from the appropriate maps. If |window| is known to this
  // client true is returned.
  bool RemoveFromMaps(const ServerWindow* window);

  // Removes |window| and all its descendants from the necessary maps. This
  // does not recurse through windows that were created by this tree. All
  // windows created by this tree are added to |created_windows|.
  void RemoveFromKnown(const ServerWindow* window,
                       std::vector<ServerWindow*>* created_windows);

  // Removes a root from set of roots of this tree. This does not remove
  // the window from the window tree, only from the set of roots.
  void RemoveRoot(ServerWindow* window, RemoveRootReason reason);

  // Converts Window(s) to WindowData(s) for transport. This assumes all the
  // windows are valid for the client. The parent of windows the client is not
  // allowed to see are set to NULL (in the returned WindowData(s)).
  std::vector<mojom::WindowDataPtr> WindowsToWindowDatas(
      const std::vector<const ServerWindow*>& windows);
  mojom::WindowDataPtr WindowToWindowData(const ServerWindow* window);

  // Implementation of GetWindowTree(). Adds |window| to |windows| and recurses
  // if CanDescendIntoWindowForWindowTree() returns true.
  void GetWindowTreeImpl(const ServerWindow* window,
                         std::vector<const ServerWindow*>* windows) const;

  // Notify the client if the drawn state of any of the roots changes.
  // |window| is the window that is changing to the drawn state
  // |new_drawn_value|.
  void NotifyDrawnStateChanged(const ServerWindow* window,
                               bool new_drawn_value);

  // Deletes all Windows we own.
  void DestroyWindows();

  bool CanEmbed(const ClientWindowId& window_id) const;
  void PrepareForEmbed(ServerWindow* window);
  void RemoveChildrenAsPartOfEmbed(ServerWindow* window);

  // Generates a new event id for an accelerator or event ack, sets it in
  // |event_ack_id_| and returns it.
  uint32_t GenerateEventAckId();

  void DispatchEventImpl(ServerWindow* target,
                         const ui::Event& event,
                         const EventLocation& event_location,
                         DispatchEventCallback callback);

  // Returns true if the client has a pointer watcher and this event matches.
  bool EventMatchesPointerWatcher(const ui::Event& event) const;

  // Calls OnChangeCompleted() on the client.
  void NotifyChangeCompleted(uint32_t change_id,
                             mojom::WindowManagerErrorCode error_code);

  // Callback for when WmMoveDragImage completes. This sends off the next
  // queued move under the image if the mouse had further moves while we were
  // waiting for the last move to be acknowledged.
  void OnWmMoveDragImageAck();

  // Called from SetDisplayRoot(), see mojom for details. Returns the root
  // of the display if successful, otherwise null.
  ServerWindow* ProcessSetDisplayRoot(
      const display::Display& display_to_create,
      const display::ViewportMetrics& transport_viewport_metrics,
      bool is_primary_display,
      const ClientWindowId& client_window_id,
      const std::vector<display::Display>& mirrors);

  bool ProcessSwapDisplayRoots(int64_t display_id1, int64_t display_id2);
  bool ProcessSetBlockingContainers(std::vector<mojom::BlockingContainersPtr>
                                        transport_all_blocking_containers);

  // Returns the ClientWindowId from a transport id. Uses id_ as the
  // ClientWindowId::client_id part if it was invalid. This function
  // do a straight mapping, there may not be a window with the returned id.
  ClientWindowId MakeClientWindowId(Id transport_window_id) const;

  // Returns the WindowTreeClient previously scheduled for an embed with the
  // given |token| from ScheduleEmbed(). If this client is the result of an
  // Embed() and ScheduleEmbed() was not called on this client, then this
  // recurses to the parent WindowTree. Recursing enables an acestor to call
  // ScheduleEmbed() and the ancestor to communicate the token with the client.
  mojom::WindowTreeClientPtr GetAndRemoveScheduledEmbedWindowTreeClient(
      const base::UnguessableToken& token);

  // AsyncEventDispatcher:
  void DispatchEvent(ServerWindow* target,
                     const ui::Event& event,
                     const EventLocation& event_location,
                     DispatchEventCallback callback) override;
  void DispatchAccelerator(uint32_t accelerator_id,
                           const ui::Event& event,
                           AcceleratorCallback callback) override;

  // WindowTree:
  void NewWindow(
      uint32_t change_id,
      Id transport_window_id,
      const base::Optional<base::flat_map<std::string, std::vector<uint8_t>>>&
          transport_properties) override;
  void NewTopLevelWindow(
      uint32_t change_id,
      Id transport_window_id,
      const base::flat_map<std::string, std::vector<uint8_t>>&
          transport_properties) override;
  void DeleteWindow(uint32_t change_id, Id transport_window_id) override;
  void AddWindow(uint32_t change_id, Id parent_id, Id child_id) override;
  void RemoveWindowFromParent(uint32_t change_id, Id window_id) override;
  void AddTransientWindow(uint32_t change_id,
                          Id window,
                          Id transient_window) override;
  void RemoveTransientWindowFromParent(uint32_t change_id,
                                       Id transient_window_id) override;
  void SetModalType(uint32_t change_id, Id window_id, ModalType type) override;
  void SetChildModalParent(uint32_t change_id,
                           Id window_id,
                           Id parent_window_id) override;
  void ReorderWindow(uint32_t change_Id,
                     Id window_id,
                     Id relative_window_id,
                     mojom::OrderDirection direction) override;
  void GetWindowTree(Id window_id, GetWindowTreeCallback callback) override;
  void SetCapture(uint32_t change_id, Id window_id) override;
  void ReleaseCapture(uint32_t change_id, Id window_id) override;
  void StartPointerWatcher(bool want_moves) override;
  void StopPointerWatcher() override;
  void SetWindowBounds(
      uint32_t change_id,
      Id window_id,
      const gfx::Rect& bounds,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id) override;
  void SetWindowTransform(uint32_t change_id,
                          Id window_id,
                          const gfx::Transform& transform) override;
  void SetWindowVisibility(uint32_t change_id,
                           Id window_id,
                           bool visible) override;
  void SetWindowProperty(
      uint32_t change_id,
      Id transport_window_id,
      const std::string& name,
      const base::Optional<std::vector<uint8_t>>& value) override;
  void SetWindowOpacity(uint32_t change_id,
                        Id window_id,
                        float opacity) override;
  void AttachCompositorFrameSink(
      Id transport_window_id,
      viz::mojom::CompositorFrameSinkRequest compositor_frame_sink,
      viz::mojom::CompositorFrameSinkClientPtr client) override;
  void Embed(Id transport_window_id,
             mojom::WindowTreeClientPtr client,
             uint32_t flags,
             EmbedCallback callback) override;
  void ScheduleEmbed(mojom::WindowTreeClientPtr client,
                     ScheduleEmbedCallback callback) override;
  void EmbedUsingToken(Id transport_window_id,
                       const base::UnguessableToken& token,
                       uint32_t flags,
                       EmbedUsingTokenCallback callback) override;
  void ScheduleEmbedForExistingClient(
      ClientSpecificId window_id,
      ScheduleEmbedForExistingClientCallback callback) override;
  void SetFocus(uint32_t change_id, Id transport_window_id) override;
  void SetCanFocus(Id transport_window_id, bool can_focus) override;
  void SetEventTargetingPolicy(Id transport_window_id,
                               mojom::EventTargetingPolicy policy) override;
  void SetCursor(uint32_t change_id,
                 Id transport_window_id,
                 ui::CursorData cursor) override;
  void SetWindowTextInputState(Id transport_window_id,
                               ui::mojom::TextInputStatePtr state) override;
  void SetImeVisibility(Id transport_window_id,
                        bool visible,
                        ui::mojom::TextInputStatePtr state) override;
  void OnWindowInputEventAck(uint32_t event_id,
                             mojom::EventResult result) override;
  void DeactivateWindow(Id window_id) override;
  void SetClientArea(Id transport_window_id,
                     const gfx::Insets& insets,
                     const base::Optional<std::vector<gfx::Rect>>&
                         transport_additional_client_areas) override;
  void SetCanAcceptDrops(Id window_id, bool accepts_drops) override;
  void SetHitTestMask(Id transport_window_id,
                      const base::Optional<gfx::Rect>& mask) override;
  void StackAbove(uint32_t change_id, Id above_id, Id below_id) override;
  void StackAtTop(uint32_t change_id, Id window_id) override;
  void PerformWmAction(Id window_id, const std::string& action) override;
  void GetWindowManagerClient(
      mojo::AssociatedInterfaceRequest<mojom::WindowManagerClient> internal)
      override;
  void GetCursorLocationMemory(
      GetCursorLocationMemoryCallback callback) override;
  void PerformDragDrop(
      uint32_t change_id,
      Id source_window_id,
      const gfx::Point& screen_location,
      const base::flat_map<std::string, std::vector<uint8_t>>& drag_data,
      const SkBitmap& drag_image,
      const gfx::Vector2d& drag_image_offset,
      uint32_t drag_operation,
      ui::mojom::PointerKind source) override;
  void CancelDragDrop(Id window_id) override;
  void PerformWindowMove(uint32_t change_id,
                         Id window_id,
                         ui::mojom::MoveLoopSource source,
                         const gfx::Point& cursor) override;
  void CancelWindowMove(Id window_id) override;

  // mojom::WindowManagerClient:
  void AddAccelerators(std::vector<mojom::WmAcceleratorPtr> accelerators,
                       AddAcceleratorsCallback callback) override;
  void RemoveAccelerator(uint32_t id) override;
  void AddActivationParent(Id transport_window_id) override;
  void RemoveActivationParent(Id transport_window_id) override;
  void SetExtendedHitRegionForChildren(
      Id window_id,
      const gfx::Insets& mouse_insets,
      const gfx::Insets& touch_insets) override;
  void SetKeyEventsThatDontHideCursor(
      std::vector<::ui::mojom::EventMatcherPtr> dont_hide_cursor_list) override;
  void SetDisplayRoot(const display::Display& display,
                      mojom::WmViewportMetricsPtr viewport_metrics,
                      bool is_primary_display,
                      Id window_id,
                      const std::vector<display::Display>& mirrors,
                      SetDisplayRootCallback callback) override;
  void SetDisplayConfiguration(
      const std::vector<display::Display>& displays,
      std::vector<ui::mojom::WmViewportMetricsPtr> transport_metrics,
      int64_t primary_display_id,
      int64_t internal_display_id,
      const std::vector<display::Display>& mirrors,
      SetDisplayConfigurationCallback callback) override;
  void SwapDisplayRoots(int64_t display_id1,
                        int64_t display_id2,
                        SwapDisplayRootsCallback callback) override;
  void SetBlockingContainers(
      std::vector<mojom::BlockingContainersPtr> blocking_containers,
      SetBlockingContainersCallback callback) override;
  void WmResponse(uint32_t change_id, bool response) override;
  void WmSetBoundsResponse(uint32_t change_id) override;
  void WmRequestClose(Id transport_window_id) override;
  void WmSetFrameDecorationValues(
      mojom::FrameDecorationValuesPtr values) override;
  void WmSetNonClientCursor(Id window_id, ui::CursorData cursor) override;
  void WmLockCursor() override;
  void WmUnlockCursor() override;
  void WmSetCursorVisible(bool visible) override;
  void WmSetCursorSize(ui::CursorSize cursor_size) override;
  void WmSetGlobalOverrideCursor(
      base::Optional<ui::CursorData> cursor) override;
  void WmMoveCursorToDisplayLocation(const gfx::Point& display_pixels,
                                     int64_t display_id) override;
  void WmConfineCursorToBounds(const gfx::Rect& bounds_in_pixles,
                               int64_t display_id) override;
  void WmSetCursorTouchVisible(bool enabled) override;
  void OnWmCreatedTopLevelWindow(uint32_t change_id,
                                 Id transport_window_id) override;
  void OnAcceleratorAck(uint32_t event_id,
                        mojom::EventResult result,
                        const base::flat_map<std::string, std::vector<uint8_t>>&
                            properties) override;

  // AccessPolicyDelegate:
  bool HasRootForAccessPolicy(const ServerWindow* window) const override;
  bool IsWindowKnownForAccessPolicy(const ServerWindow* window) const override;
  bool IsWindowRootOfAnotherTreeForAccessPolicy(
      const ServerWindow* window) const override;
  bool IsWindowCreatedByWindowManager(
      const ServerWindow* window) const override;
  bool ShouldInterceptEventsForAccessPolicy(
      const ServerWindow* window) const override;

  // DragSource:
  void OnDragMoved(const gfx::Point& location) override;
  void OnDragCompleted(bool success, uint32_t action_taken) override;
  DragTargetConnection* GetDragTargetForWindow(
      const ServerWindow* window) override;

  // DragTargetConnection:
  void PerformOnDragDropStart(
      const base::flat_map<std::string, std::vector<uint8_t>>& mime_data)
      override;
  void PerformOnDragEnter(
      const ServerWindow* window,
      uint32_t event_flags,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) override;
  void PerformOnDragOver(
      const ServerWindow* window,
      uint32_t event_flags,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) override;
  void PerformOnDragLeave(const ServerWindow* window) override;
  void PerformOnCompleteDrop(
      const ServerWindow* window,
      uint32_t event_flags,
      const gfx::Point& cursor_offset,
      uint32_t effect_bitmask,
      const base::Callback<void(uint32_t)>& callback) override;
  void PerformOnDragDropDone() override;

  WindowServer* window_server_;

  // True if this WindowTree was created by way of Embed().
  bool is_for_embedding_;

  // Id of this tree as assigned by WindowServer.
  const ClientSpecificId id_;
  std::string name_;

  std::unique_ptr<WindowTreeBinding> binding_;

  std::unique_ptr<ui::ws::AccessPolicy> access_policy_;

  // The roots, or embed points, of this tree. A WindowTree may have any
  // number of roots, including 0.
  std::set<const ServerWindow*> roots_;

  // The windows created by this tree. This tree owns these objects.
  std::set<ServerWindow*> created_windows_;

  // The client is allowed to assign ids.
  std::unordered_map<ClientWindowId, const ServerWindow*, ClientWindowIdHash>
      client_id_to_window_map_;
  std::unordered_map<const ServerWindow*, ClientWindowId>
      window_to_client_id_map_;

  // Id passed to the client and expected to be supplied back to
  // OnWindowInputEventAck() or OnAcceleratorAck().
  uint32_t event_ack_id_;

  DispatchEventCallback event_ack_callback_;

  AcceleratorCallback accelerator_ack_callback_;

  // A client is considered janky if it hasn't ACK'ed input events within a
  // reasonable timeframe.
  bool janky_ = false;

  // For performance reasons, only send move events if the client explicitly
  // requests them.
  bool pointer_watcher_want_moves_ = false;

  // True if StartPointerWatcher() was called.
  bool has_pointer_watcher_ = false;

  // WindowManager the current event came from.
  WindowManagerState* event_source_wms_ = nullptr;

  base::queue<std::unique_ptr<TargetedEvent>> event_queue_;

  // TODO(sky): move all window manager specific state into struct to make it
  // clear what applies only to the window manager.
  std::unique_ptr<mojo::AssociatedBinding<mojom::WindowManagerClient>>
      window_manager_internal_client_binding_;
  mojom::WindowManager* window_manager_internal_;
  std::unique_ptr<WindowManagerState> window_manager_state_;
  // See mojom for details.
  bool automatically_create_display_roots_ = true;

  std::unique_ptr<WaitingForTopLevelWindowInfo>
      waiting_for_top_level_window_info_;
  bool embedder_intercepts_events_ = false;

  // State kept while we're waiting for the window manager to ack a
  // WmMoveDragImage. Non-null while we're waiting for a response.
  struct DragMoveState;
  std::unique_ptr<DragMoveState> drag_move_state_;

  // Holds WindowTreeClients passed to ScheduleEmbed(). Entries are removed
  // when EmbedUsingToken() is called.
  using ScheduledEmbeds =
      base::flat_map<base::UnguessableToken, mojom::WindowTreeClientPtr>;
  ScheduledEmbeds scheduled_embeds_;

  // Controls whether the client can change the visibility of the roots.
  bool can_change_root_window_visibility_ = true;

  // A weak ptr factory for callbacks from the window manager for when we send
  // a image move. All weak ptrs are invalidated when a drag is completed.
  base::WeakPtrFactory<WindowTree> drag_weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WindowTree);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_TREE_H_
