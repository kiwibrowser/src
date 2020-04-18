// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_WINDOW_SERVER_H_
#define SERVICES_UI_WS_WINDOW_SERVER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/optional.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/window_manager_window_tree_factory.mojom.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws/async_event_dispatcher_lookup.h"
#include "services/ui/ws/gpu_host_delegate.h"
#include "services/ui/ws/ids.h"
#include "services/ui/ws/operation.h"
#include "services/ui/ws/server_window_delegate.h"
#include "services/ui/ws/server_window_observer.h"
#include "services/ui/ws/server_window_tracker.h"
#include "services/ui/ws/user_display_manager_delegate.h"
#include "services/ui/ws/video_detector_impl.h"

namespace ui {
namespace ws {

class AccessPolicy;
class Display;
class DisplayManager;
class GpuHost;
class ServerWindow;
class ThreadedImageCursorsFactory;
class UserActivityMonitor;
class WindowManagerDisplayRoot;
class WindowManagerState;
class WindowManagerWindowTreeFactory;
class WindowServerDelegate;
class WindowTree;
class WindowTreeBinding;

enum class DisplayCreationConfig;

struct WindowTreeAndWindowId {
  WindowTree* tree = nullptr;
  ClientSpecificId window_id = 0u;
};

// WindowServer manages the set of clients of the window server (all the
// WindowTrees) as well as providing the root of the hierarchy.
class WindowServer : public ServerWindowDelegate,
                     public ServerWindowObserver,
                     public GpuHostDelegate,
                     public UserDisplayManagerDelegate,
                     public AsyncEventDispatcherLookup {
 public:
  WindowServer(WindowServerDelegate* delegate, bool should_host_viz);
  ~WindowServer() override;

  WindowServerDelegate* delegate() { return delegate_; }

  DisplayManager* display_manager() { return display_manager_.get(); }
  const DisplayManager* display_manager() const {
    return display_manager_.get();
  }

  void SetDisplayCreationConfig(DisplayCreationConfig config);
  DisplayCreationConfig display_creation_config() const {
    return display_creation_config_;
  }

  void SetGpuHost(std::unique_ptr<GpuHost> gpu_host);
  GpuHost* gpu_host() { return gpu_host_.get(); }

  bool is_hosting_viz() const { return !!host_frame_sink_manager_; }

  ThreadedImageCursorsFactory* GetThreadedImageCursorsFactory();

  // Creates a new ServerWindow. The return value is owned by the caller, but
  // must be destroyed before WindowServer.
  ServerWindow* CreateServerWindow(
      const viz::FrameSinkId& frame_sink_id,
      const std::map<std::string, std::vector<uint8_t>>& properties);

  // Returns the id for the next WindowTree.
  ClientSpecificId GetAndAdvanceNextClientId();

  // See description of WindowTree::Embed() for details. This assumes
  // |transport_window_id| is valid.
  WindowTree* EmbedAtWindow(ServerWindow* root,
                            mojom::WindowTreeClientPtr client,
                            uint32_t flags,
                            std::unique_ptr<AccessPolicy> access_policy);

  // Adds |tree_impl_ptr| to the set of known trees. Use DestroyTree() to
  // destroy the tree.
  void AddTree(std::unique_ptr<WindowTree> tree_impl_ptr,
               std::unique_ptr<WindowTreeBinding> binding,
               mojom::WindowTreePtr tree_ptr);
  WindowTree* CreateTreeForWindowManager(
      mojom::WindowTreeRequest window_tree_request,
      mojom::WindowTreeClientPtr window_tree_client,
      bool automatically_create_display_roots);
  // Invoked when a WindowTree's connection encounters an error.
  void DestroyTree(WindowTree* tree);

  // Returns the tree by client id.
  WindowTree* GetTreeWithId(ClientSpecificId client_id);

  WindowTree* GetTreeWithClientName(const std::string& client_name);

  size_t num_trees() const { return tree_map_.size(); }

  // Creates and registers a token for use in a future embedding. |window_id|
  // is the window_id portion of the ClientWindowId to use for the window in
  // |tree|. |window_id| is validated during actual embed.
  base::UnguessableToken RegisterEmbedToken(WindowTree* tree,
                                            ClientSpecificId window_id);

  // Unregisters the WindowTree associated with |token| and returns it. Returns
  // null if RegisterEmbedToken() was not previously called for |token|.
  WindowTreeAndWindowId UnregisterEmbedToken(
      const base::UnguessableToken& token);

  OperationType current_operation_type() const {
    return current_operation_ ? current_operation_->type()
                              : OperationType::NONE;
  }

  // Returns true if the specified client issued the current operation.
  bool IsOperationSource(ClientSpecificId client_id) const {
    return current_operation_ &&
           current_operation_->source_tree_id() == client_id;
  }

  // Invoked when a client messages a client about the change. This is used
  // to avoid sending ServerChangeIdAdvanced() unnecessarily.
  void OnTreeMessagedClient(ClientSpecificId id);

  // Returns true if OnTreeMessagedClient() was invoked for id.
  bool DidTreeMessageClient(ClientSpecificId id) const;

  // Returns the WindowTree that has |window| as a root.
  WindowTree* GetTreeWithRoot(const ServerWindow* window) {
    return const_cast<WindowTree*>(
        const_cast<const WindowServer*>(this)->GetTreeWithRoot(window));
  }
  const WindowTree* GetTreeWithRoot(const ServerWindow* window) const;

  UserActivityMonitor* user_activity_monitor() {
    return user_activity_monitor_.get();
  }

  WindowManagerWindowTreeFactory* window_manager_window_tree_factory() {
    return window_manager_window_tree_factory_.get();
  }
  void BindWindowManagerWindowTreeFactory(
      mojo::InterfaceRequest<mojom::WindowManagerWindowTreeFactory> request);

  // Sets focus to |window|. Returns true if |window| already has focus, or
  // focus was successfully changed. Returns |false| if |window| is not a valid
  // window to receive focus.
  bool SetFocusedWindow(ServerWindow* window);
  ServerWindow* GetFocusedWindow();

  void SetHighContrastMode(bool enabled);

  // Returns a change id for the window manager that is associated with
  // |source| and |client_change_id|. When the window manager replies
  // WindowManagerChangeCompleted() is called to obtain the original source
  // and client supplied change_id that initiated the called.
  uint32_t GenerateWindowManagerChangeId(WindowTree* source,
                                         uint32_t client_change_id);

  // Called when a response from the window manager is obtained. Calls to
  // the client that initiated the change with the change id originally
  // supplied by the client.
  void WindowManagerChangeCompleted(uint32_t window_manager_change_id,
                                    bool success);
  void WindowManagerCreatedTopLevelWindow(WindowTree* wm_tree,
                                          uint32_t window_manager_change_id,
                                          ServerWindow* window);

  // Called when we get an unexpected message from the WindowManager.
  // TODO(sky): decide what we want to do here.
  void WindowManagerSentBogusMessage() {}

  // These functions trivially delegate to all WindowTrees, which in
  // term notify their clients.
  void ProcessWindowBoundsChanged(
      const ServerWindow* window,
      const gfx::Rect& old_bounds,
      const gfx::Rect& new_bounds,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id);
  void ProcessWindowTransformChanged(const ServerWindow* window,
                                     const gfx::Transform& old_transform,
                                     const gfx::Transform& new_transform);
  void ProcessClientAreaChanged(
      const ServerWindow* window,
      const gfx::Insets& new_client_area,
      const std::vector<gfx::Rect>& new_additional_client_areas);
  void ProcessCaptureChanged(const ServerWindow* new_capture,
                             const ServerWindow* old_capture);
  void ProcessWillChangeWindowHierarchy(const ServerWindow* window,
                                        const ServerWindow* new_parent,
                                        const ServerWindow* old_parent);
  void ProcessWindowHierarchyChanged(const ServerWindow* window,
                                     const ServerWindow* new_parent,
                                     const ServerWindow* old_parent);
  void ProcessWindowReorder(const ServerWindow* window,
                            const ServerWindow* relative_window,
                            const mojom::OrderDirection direction);
  void ProcessWindowDeleted(ServerWindow* window);
  void ProcessWillChangeWindowCursor(ServerWindow* window,
                                     const ui::CursorData& cursor);

  // Sends an |event| to all WindowTrees that might be observing events. Skips
  // |ignore_tree| if it is non-null. |target_window| is the target of the
  // event.
  void SendToPointerWatchers(const ui::Event& event,
                             ServerWindow* target_window,
                             WindowTree* ignore_tree,
                             int64_t display_id);

  // Sets a callback to be called whenever a surface is activated. This
  // corresponds a client submitting a new CompositorFrame for a Window. This
  // should only be called in a test configuration.
  void SetSurfaceActivationCallback(
      base::OnceCallback<void(ServerWindow*)> callback);

  void StartMoveLoop(uint32_t change_id,
                     ServerWindow* window,
                     WindowTree* initiator,
                     const gfx::Rect& revert_bounds);
  void EndMoveLoop();
  uint32_t GetCurrentMoveLoopChangeId();
  ServerWindow* GetCurrentMoveLoopWindow();
  WindowTree* GetCurrentMoveLoopInitiator();
  gfx::Rect GetCurrentMoveLoopRevertBounds();
  bool in_move_loop() const { return !!current_move_loop_; }

  void StartDragLoop(uint32_t change_id,
                     ServerWindow* window,
                     WindowTree* initiator);
  void EndDragLoop();
  uint32_t GetCurrentDragLoopChangeId();
  ServerWindow* GetCurrentDragLoopWindow();
  WindowTree* GetCurrentDragLoopInitiator();
  bool in_drag_loop() const { return !!current_drag_loop_; }

  void OnDisplayReady(Display* display, bool is_first);
  void OnDisplayDestroyed(Display* display);
  void OnNoMoreDisplays();
  WindowManagerState* GetWindowManagerState();

  VideoDetectorImpl* video_detector() { return &video_detector_; }

  // ServerWindowDelegate:
  VizHostProxy* GetVizHostProxy() override;
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info,
                                ServerWindow* window) override;

  // UserDisplayManagerDelegate:
  bool GetFrameDecorations(mojom::FrameDecorationValuesPtr* values) override;
  int64_t GetInternalDisplayId() override;

 private:
  struct CurrentMoveLoopState;
  struct CurrentDragLoopState;
  friend class Operation;

  using WindowTreeMap =
      std::map<ClientSpecificId, std::unique_ptr<WindowTree>>;

  struct InFlightWindowManagerChange {
    // Identifies the client that initiated the change.
    ClientSpecificId client_id;

    // Change id supplied by the client.
    uint32_t client_change_id;
  };

  using InFlightWindowManagerChangeMap =
      std::map<uint32_t, InFlightWindowManagerChange>;

  bool GetAndClearInFlightWindowManagerChange(
      uint32_t window_manager_change_id,
      InFlightWindowManagerChange* change);

  // Invoked when a client is about to execute a window server operation.
  // Subsequently followed by FinishOperation() once the change is done.
  //
  // Changes should never nest, meaning each PrepareForOperation() must be
  // balanced with a call to FinishOperation() with no PrepareForOperation()
  // in between.
  void PrepareForOperation(Operation* op);

  // Balances a call to PrepareForOperation().
  void FinishOperation();

  // Updates the native cursor by figuring out what window is under the mouse
  // cursor. This is run in response to events that change the bounds or window
  // hierarchy.
  void UpdateNativeCursorFromMouseLocation(ServerWindow* window);
  void UpdateNativeCursorFromMouseLocation(
      WindowManagerDisplayRoot* display_root);

  // Updates the native cursor if the cursor is currently inside |window|. This
  // is run in response to events that change the mouse cursor properties of
  // |window|.
  void UpdateNativeCursorIfOver(ServerWindow* window);

  // Finds the parent client that will embed |surface_id| and claims ownership
  // of the temporary reference. If no parent client is found then tell GPU to
  // immediately drop the temporary reference. |window| is the ServerWindow
  // that corresponds to |surface_id|.
  void HandleTemporaryReferenceForNewSurface(const viz::SurfaceId& surface_id,
                                             ServerWindow* window);

  void CreateFrameSinkManager();

  // AsyncEventDispatcherLookup:
  AsyncEventDispatcher* GetAsyncEventDispatcherById(
      ClientSpecificId id) override;

  // Overridden from ServerWindowDelegate:
  ServerWindow* GetRootWindowForDrawn(const ServerWindow* window) override;

  // Overridden from ServerWindowObserver:
  void OnWindowDestroyed(ServerWindow* window) override;
  void OnWillChangeWindowHierarchy(ServerWindow* window,
                                   ServerWindow* new_parent,
                                   ServerWindow* old_parent) override;
  void OnWindowHierarchyChanged(ServerWindow* window,
                                ServerWindow* new_parent,
                                ServerWindow* old_parent) override;
  void OnWindowBoundsChanged(ServerWindow* window,
                             const gfx::Rect& old_bounds,
                             const gfx::Rect& new_bounds) override;
  void OnWindowTransformChanged(ServerWindow* window,
                                const gfx::Transform& old_transform,
                                const gfx::Transform& new_transform) override;
  void OnWindowClientAreaChanged(
      ServerWindow* window,
      const gfx::Insets& new_client_area,
      const std::vector<gfx::Rect>& new_additional_client_areas) override;
  void OnWindowReordered(ServerWindow* window,
                         ServerWindow* relative,
                         mojom::OrderDirection direction) override;
  void OnWillChangeWindowVisibility(ServerWindow* window) override;
  void OnWindowVisibilityChanged(ServerWindow* window) override;
  void OnWindowOpacityChanged(ServerWindow* window,
                              float old_opacity,
                              float new_opacity) override;
  void OnWindowSharedPropertyChanged(
      ServerWindow* window,
      const std::string& name,
      const std::vector<uint8_t>* new_data) override;
  void OnWindowCursorChanged(ServerWindow* window,
                             const ui::CursorData& cursor) override;
  void OnWindowNonClientCursorChanged(ServerWindow* window,
                                      const ui::CursorData& cursor) override;
  void OnWindowTextInputStateChanged(ServerWindow* window,
                                     const ui::TextInputState& state) override;
  void OnTransientWindowAdded(ServerWindow* window,
                              ServerWindow* transient_child) override;
  void OnTransientWindowRemoved(ServerWindow* window,
                                ServerWindow* transient_child) override;
  void OnWindowModalTypeChanged(ServerWindow* window,
                                ModalType old_modal_type) override;

  // GpuHostDelegate:
  void OnGpuServiceInitialized() override;

  WindowServerDelegate* delegate_;

  // ID to use for next WindowTree.
  ClientSpecificId next_client_id_;

  std::unique_ptr<DisplayManager> display_manager_;

  std::unique_ptr<CurrentDragLoopState> current_drag_loop_;
  std::unique_ptr<CurrentMoveLoopState> current_move_loop_;

  // Set of WindowTrees.
  WindowTreeMap tree_map_;

  // If non-null then we're processing a client operation. The Operation is
  // not owned by us (it's created on the stack by WindowTree).
  Operation* current_operation_;

  bool in_destructor_;
  bool high_contrast_mode_ = false;

  // Maps from window manager change id to the client that initiated the
  // request.
  InFlightWindowManagerChangeMap in_flight_wm_change_map_;

  // Next id supplied to the window manager.
  uint32_t next_wm_change_id_;

  std::unique_ptr<GpuHost> gpu_host_;
  base::OnceCallback<void(ServerWindow*)> surface_activation_callback_;

  std::unique_ptr<UserActivityMonitor> user_activity_monitor_;

  std::unique_ptr<WindowManagerWindowTreeFactory>
      window_manager_window_tree_factory_;

  viz::SurfaceId root_surface_id_;

  // Incremented when the viz process is restarted.
  uint32_t viz_restart_id_ = viz::BeginFrameSource::kNotRestartableId + 1;

  // Provides interfaces to create and manage FrameSinks.
  std::unique_ptr<viz::HostFrameSinkManager> host_frame_sink_manager_;
  std::unique_ptr<VizHostProxy> viz_host_proxy_;

  VideoDetectorImpl video_detector_;

  // System modal windows not attached to a display are added here. Once
  // attached to a display they are removed.
  ServerWindowTracker pending_system_modal_windows_;

  DisplayCreationConfig display_creation_config_;

  // Tokens registered by ScheduleEmbedForExistingClient() that are removed when
  // EmbedUsingToken() is called.
  using ScheduledEmbeds =
      base::flat_map<base::UnguessableToken, WindowTreeAndWindowId>;
  ScheduledEmbeds scheduled_embeds_;

  DISALLOW_COPY_AND_ASSIGN(WindowServer);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_WINDOW_SERVER_H_
