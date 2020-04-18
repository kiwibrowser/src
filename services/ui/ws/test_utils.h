// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_TEST_UTILS_H_
#define SERVICES_UI_WS_TEST_UTILS_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <vector>

#include "base/atomicops.h"
#include "base/containers/flat_map.h"
#include "base/message_loop/message_loop.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/ui/display/screen_manager.h"
#include "services/ui/display/viewport_metrics.h"
#include "services/ui/public/interfaces/screen_provider.mojom.h"
#include "services/ui/public/interfaces/window_tree.mojom.h"
#include "services/ui/ws/display.h"
#include "services/ui/ws/display_binding.h"
#include "services/ui/ws/drag_controller.h"
#include "services/ui/ws/event_dispatcher_impl_test_api.h"
#include "services/ui/ws/event_processor.h"
#include "services/ui/ws/event_targeter.h"
#include "services/ui/ws/gpu_host.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/platform_display_factory.h"
#include "services/ui/ws/test_change_tracker.h"
#include "services/ui/ws/user_activity_monitor.h"
#include "services/ui/ws/window_manager_state.h"
#include "services/ui/ws/window_server_delegate.h"
#include "services/ui/ws/window_tree.h"
#include "services/ui/ws/window_tree_binding.h"
#include "ui/display/display.h"
#include "ui/display/screen_base.h"
#include "ui/display/types/display_constants.h"

namespace ui {
namespace ws {

class CursorLocationManager;

namespace test {

const ClientSpecificId kWindowManagerClientId = kWindowServerClientId + 1;
const std::string kWindowManagerClientIdString =
    std::to_string(kWindowManagerClientId);
const ClientSpecificId kEmbedTreeWindowId = 1;

// Collection of utilities useful in creating mus tests.

// Test ScreenManager instance that allows adding/modifying/removing displays.
// Tracks display ids to perform some basic verification that no duplicates are
// added and display was added before being modified or removed. Display ids
// reset when Init() is called.
class TestScreenManager : public display::ScreenManager {
 public:
  TestScreenManager();
  ~TestScreenManager() override;

  // Adds a new display with default metrics, generates a unique display id and
  // returns it. Calls OnDisplayAdded() on delegate.
  int64_t AddDisplay();

  // Adds a new display with provided |display|, generates a unique display id
  // and returns it. Calls OnDisplayAdded() on delegate.
  int64_t AddDisplay(const display::Display& display);

  // Calls OnDisplayModified() on delegate.
  void ModifyDisplay(const display::Display& display,
                     const base::Optional<display::ViewportMetrics>& metrics =
                         base::Optional<display::ViewportMetrics>());

  // Calls OnDisplayRemoved() on delegate.
  void RemoveDisplay(int64_t id);

  // display::ScreenManager:
  void AddInterfaces(
      service_manager::BinderRegistryWithArgs<
          const service_manager::BindSourceInfo&>* registry) override {}
  void Init(display::ScreenManagerDelegate* delegate) override;
  void RequestCloseDisplay(int64_t display_id) override {}
  display::ScreenBase* GetScreen() override;

 private:
  display::ScreenManagerDelegate* delegate_ = nullptr;
  std::unique_ptr<display::ScreenBase> screen_;
  std::set<int64_t> display_ids_;

  DISALLOW_COPY_AND_ASSIGN(TestScreenManager);
};

// -----------------------------------------------------------------------------

class UserActivityMonitorTestApi {
 public:
  explicit UserActivityMonitorTestApi(UserActivityMonitor* monitor)
      : monitor_(monitor) {}

  void SetTimerTaskRunner(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
    monitor_->idle_timer_.SetTaskRunner(task_runner);
  }

 private:
  UserActivityMonitor* monitor_;
  DISALLOW_COPY_AND_ASSIGN(UserActivityMonitorTestApi);
};

// -----------------------------------------------------------------------------

class WindowTreeTestApi {
 public:
  explicit WindowTreeTestApi(WindowTree* tree);
  ~WindowTreeTestApi();

  void set_is_for_embedding(bool value) { tree_->is_for_embedding_ = value; }
  void set_window_manager_internal(mojom::WindowManager* wm_internal) {
    tree_->window_manager_internal_ = wm_internal;
  }
  void SetEventTargetingPolicy(Id transport_window_id,
                               mojom::EventTargetingPolicy policy) {
    tree_->SetEventTargetingPolicy(transport_window_id, policy);
  }
  void AckOldestEvent(
      mojom::EventResult result = mojom::EventResult::UNHANDLED) {
    tree_->OnWindowInputEventAck(tree_->event_ack_id_, result);
  }
  void EnableCapture() { tree_->event_ack_id_ = 1u; }
  void AckLastEvent(mojom::EventResult result) {
    tree_->OnWindowInputEventAck(tree_->event_ack_id_, result);
  }
  void AckLastAccelerator(
      mojom::EventResult result,
      const base::flat_map<std::string, std::vector<uint8_t>>& properties =
          base::flat_map<std::string, std::vector<uint8_t>>()) {
    tree_->OnAcceleratorAck(tree_->event_ack_id_, result, properties);
  }

  void StartPointerWatcher(bool want_moves);
  void StopPointerWatcher();

  bool ProcessSetDisplayRoot(const display::Display& display_to_create,
                             const display::ViewportMetrics& viewport_metrics,
                             bool is_primary_display,
                             const ClientWindowId& client_window_id) {
    return tree_->ProcessSetDisplayRoot(display_to_create, viewport_metrics,
                                        is_primary_display, client_window_id,
                                        std::vector<display::Display>());
  }

  bool ProcessSwapDisplayRoots(int64_t display_id1, int64_t display_id2) {
    return tree_->ProcessSwapDisplayRoots(display_id1, display_id2);
  }
  size_t event_queue_size() const { return tree_->event_queue_.size(); }
  bool HasEventInFlight() const { return tree_->event_ack_id_ != 0u; }

 private:
  WindowTree* tree_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeTestApi);
};

// -----------------------------------------------------------------------------

class EventProcessorTestApi {
 public:
  explicit EventProcessorTestApi(EventProcessor* ep) : ep_(ep) {}
  ~EventProcessorTestApi() {}

  bool AreAnyPointersDown() const { return ep_->AreAnyPointersDown(); }
  bool is_mouse_button_down() const { return ep_->mouse_button_down_; }
  bool IsWindowPointerTarget(const ServerWindow* window) const;
  int NumberPointerTargetsForWindow(ServerWindow* window);
  ModalWindowController* modal_window_controller() const {
    return &ep_->modal_window_controller_;
  }
  ServerWindow* capture_window() { return ep_->capture_window_; }
  EventTargeter* event_targeter() { return ep_->event_targeter_.get(); }
  bool IsObservingWindow(ServerWindow* window);

 private:
  EventProcessor* ep_;

  DISALLOW_COPY_AND_ASSIGN(EventProcessorTestApi);
};

// -----------------------------------------------------------------------------

class EventTargeterTestApi {
 public:
  explicit EventTargeterTestApi(EventTargeter* event_targeter)
      : event_targeter_(event_targeter) {}
  ~EventTargeterTestApi() {}

  bool HasPendingQueries() const {
    return event_targeter_->weak_ptr_factory_.HasWeakPtrs();
  }

 private:
  EventTargeter* event_targeter_;

  DISALLOW_COPY_AND_ASSIGN(EventTargeterTestApi);
};

// -----------------------------------------------------------------------------

class ModalWindowControllerTestApi {
 public:
  explicit ModalWindowControllerTestApi(ModalWindowController* mwc)
      : mwc_(mwc) {}
  ~ModalWindowControllerTestApi() {}

  const ServerWindow* GetActiveSystemModalWindow() const {
    return mwc_->GetActiveSystemModalWindow();
  }

 private:
  ModalWindowController* mwc_;

  DISALLOW_COPY_AND_ASSIGN(ModalWindowControllerTestApi);
};

// -----------------------------------------------------------------------------

class WindowManagerStateTestApi {
 public:
  explicit WindowManagerStateTestApi(WindowManagerState* wms) : wms_(wms) {}
  ~WindowManagerStateTestApi() {}

  ClientSpecificId GetEventTargetClientId(ServerWindow* window,
                                          bool in_nonclient_area) {
    return wms_->GetEventTargetClientId(window, in_nonclient_area);
  }

  void ProcessEvent(ui::Event* event, int64_t display_id = 0) {
    wms_->ProcessEvent(event, display_id);
  }

  ClientSpecificId GetEventTargetClientId(const ServerWindow* window,
                                          bool in_nonclient_area) {
    return wms_->GetEventTargetClientId(window, in_nonclient_area);
  }

  void OnEventAckTimeout(ClientSpecificId client_id) {
    EventDispatcherImplTestApi(&wms_->event_dispatcher_)
        .OnDispatchInputEventTimeout();
  }

  const std::vector<std::unique_ptr<WindowManagerDisplayRoot>>&
  window_manager_display_roots() const {
    return wms_->window_manager_display_roots_;
  }

  // TODO(sky): convert calling code to use EventDispatcherImplTestApi directly.
  void DispatchInputEventToWindow(ServerWindow* target,
                                  ClientSpecificId client_id,
                                  const EventLocation& event_location,
                                  const ui::Event& event,
                                  Accelerator* accelerator) {
    EventDispatcherImplTestApi(&wms_->event_dispatcher_)
        .DispatchInputEventToWindow(target, client_id, event_location, event,
                                    accelerator);
  }
  WindowTree* tree_awaiting_input_ack() {
    return EventDispatcherImplTestApi(&wms_->event_dispatcher_)
        .GetTreeThatWillAckEvent();
  }
  bool is_event_tasks_empty() const {
    return EventDispatcherImplTestApi(&wms_->event_dispatcher_)
        .is_event_tasks_empty();
  }
  bool AckInFlightEvent(mojom::EventResult result) {
    return EventDispatcherImplTestApi(&wms_->event_dispatcher_)
        .OnDispatchInputEventDone(result);
  }

 private:
  WindowManagerState* wms_;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerStateTestApi);
};

// -----------------------------------------------------------------------------

class DragControllerTestApi {
 public:
  explicit DragControllerTestApi(DragController* op) : op_(op) {}
  ~DragControllerTestApi() {}

  size_t GetSizeOfQueueForWindow(ServerWindow* window) {
    return op_->GetSizeOfQueueForWindow(window);
  }

  ServerWindow* GetCurrentTarget() { return op_->current_target_window_; }

 private:
  DragController* op_;

  DISALLOW_COPY_AND_ASSIGN(DragControllerTestApi);
};

// -----------------------------------------------------------------------------

// Factory that always embeds the new WindowTree as the root user id.
class TestDisplayBinding : public DisplayBinding {
 public:
  explicit TestDisplayBinding(WindowServer* window_server,
                              bool automatically_create_display_roots = true)
      : window_server_(window_server),
        automatically_create_display_roots_(
            automatically_create_display_roots) {}
  ~TestDisplayBinding() override {}

 private:
  // DisplayBinding:
  WindowTree* CreateWindowTree(ServerWindow* root) override;

  WindowServer* window_server_;
  const bool automatically_create_display_roots_;

  DISALLOW_COPY_AND_ASSIGN(TestDisplayBinding);
};

// -----------------------------------------------------------------------------

// Factory that dispenses TestPlatformDisplays.
class TestPlatformDisplayFactory : public PlatformDisplayFactory {
 public:
  explicit TestPlatformDisplayFactory(ui::CursorData* cursor_storage);
  ~TestPlatformDisplayFactory();

  // PlatformDisplayFactory:
  std::unique_ptr<PlatformDisplay> CreatePlatformDisplay(
      ServerWindow* root_window,
      const display::ViewportMetrics& metrics) override;

 private:
  ui::CursorData* cursor_storage_;

  DISALLOW_COPY_AND_ASSIGN(TestPlatformDisplayFactory);
};

// -----------------------------------------------------------------------------

class TestWindowManager : public mojom::WindowManager {
 public:
  TestWindowManager();
  ~TestWindowManager() override;

  bool did_call_create_top_level_window(uint32_t* change_id) {
    if (!got_create_top_level_window_)
      return false;

    got_create_top_level_window_ = false;
    *change_id = change_id_;
    return true;
  }

  void ClearAcceleratorCalled() {
    on_accelerator_id_ = 0u;
    on_accelerator_called_ = false;
  }

  const std::string& last_wm_action() const { return last_wm_action_; }
  bool on_perform_move_loop_called() { return on_perform_move_loop_called_; }
  bool on_accelerator_called() { return on_accelerator_called_; }
  uint32_t on_accelerator_id() { return on_accelerator_id_; }
  bool got_display_removed() const { return got_display_removed_; }
  int64_t display_removed_id() const { return display_removed_id_; }
  bool on_set_modal_type_called() { return on_set_modal_type_called_; }
  int connect_count() const { return connect_count_; }
  int display_added_count() const { return display_added_count_; }

 private:
  // WindowManager:
  void OnConnect() override;
  void WmOnAcceleratedWidgetForDisplay(
      int64_t display,
      gpu::SurfaceHandle surface_handle) override;
  void WmNewDisplayAdded(
      const display::Display& display,
      ui::mojom::WindowDataPtr root,
      bool drawn,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id) override;
  void WmDisplayRemoved(int64_t display_id) override;
  void WmDisplayModified(const display::Display& display) override {}
  void WmSetBounds(uint32_t change_id,
                   Id window_id,
                   const gfx::Rect& bounds) override {}
  void WmSetProperty(
      uint32_t change_id,
      Id window_id,
      const std::string& name,
      const base::Optional<std::vector<uint8_t>>& value) override {}
  void WmSetModalType(Id window_id, ui::ModalType type) override;
  void WmSetCanFocus(Id window_id, bool can_focus) override {}
  void WmCreateTopLevelWindow(
      uint32_t change_id,
      const viz::FrameSinkId& frame_sink_id,
      const base::flat_map<std::string, std::vector<uint8_t>>& properties)
      override;
  void WmClientJankinessChanged(ClientSpecificId client_id,
                                bool janky) override;
  void WmBuildDragImage(const gfx::Point& screen_location,
                        const SkBitmap& drag_image,
                        const gfx::Vector2d& drag_image_offset,
                        ui::mojom::PointerKind source) override;
  void WmMoveDragImage(const gfx::Point& screen_location,
                       WmMoveDragImageCallback callback) override;
  void WmDestroyDragImage() override;
  void WmPerformMoveLoop(uint32_t change_id,
                         Id window_id,
                         mojom::MoveLoopSource source,
                         const gfx::Point& cursor_location) override;
  void WmCancelMoveLoop(uint32_t change_id) override;
  void WmDeactivateWindow(Id window_id) override;
  void WmStackAbove(uint32_t change_id, Id above_id, Id below_id) override;
  void WmStackAtTop(uint32_t change_id, Id window_id) override;
  void WmPerformWmAction(Id window_id, const std::string& action) override;
  void OnAccelerator(uint32_t ack_id,
                     uint32_t accelerator_id,
                     std::unique_ptr<ui::Event> event) override;
  void OnCursorTouchVisibleChanged(bool enabled) override;
  void OnEventBlockedByModalWindow(Id window_id) override;

  std::string last_wm_action_;

  bool on_perform_move_loop_called_ = false;
  bool on_set_modal_type_called_ = false;

  bool got_create_top_level_window_ = false;
  uint32_t change_id_ = 0u;

  bool on_accelerator_called_ = false;
  uint32_t on_accelerator_id_ = 0u;

  bool got_display_removed_ = false;
  int64_t display_removed_id_ = 0;

  int connect_count_ = 0;
  int display_added_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestWindowManager);
};

// -----------------------------------------------------------------------------

// WindowTreeClient implementation that logs all calls to a TestChangeTracker.
class TestWindowTreeClient : public ui::mojom::WindowTreeClient {
 public:
  TestWindowTreeClient();
  ~TestWindowTreeClient() override;

  TestChangeTracker* tracker() { return &tracker_; }

  void Bind(mojo::InterfaceRequest<mojom::WindowTreeClient> request);

  void set_record_on_change_completed(bool value) {
    record_on_change_completed_ = value;
  }

 private:
  // WindowTreeClient:
  void OnEmbed(
      mojom::WindowDataPtr root,
      ui::mojom::WindowTreePtr tree,
      int64_t display_id,
      Id focused_window_id,
      bool drawn,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id) override;
  void OnEmbedFromToken(
      const base::UnguessableToken& token,
      mojom::WindowDataPtr root,
      int64_t display_id,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id) override;
  void OnEmbeddedAppDisconnected(Id window) override;
  void OnUnembed(Id window_id) override;
  void OnCaptureChanged(Id new_capture_window_id,
                        Id old_capture_window_id) override;
  void OnFrameSinkIdAllocated(Id window_id,
                              const viz::FrameSinkId& frame_sink_id) override;
  void OnTopLevelCreated(
      uint32_t change_id,
      mojom::WindowDataPtr data,
      int64_t display_id,
      bool drawn,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id) override;
  void OnWindowBoundsChanged(
      Id window,
      const gfx::Rect& old_bounds,
      const gfx::Rect& new_bounds,
      const base::Optional<viz::LocalSurfaceId>& local_surface_id) override;
  void OnWindowTransformChanged(Id window,
                                const gfx::Transform& old_transform,
                                const gfx::Transform& new_transform) override;
  void OnClientAreaChanged(
      Id window_id,
      const gfx::Insets& new_client_area,
      const std::vector<gfx::Rect>& new_additional_client_areas) override;
  void OnTransientWindowAdded(Id window_id, Id transient_window_id) override;
  void OnTransientWindowRemoved(Id window_id, Id transient_window_id) override;
  void OnWindowHierarchyChanged(
      Id window,
      Id old_parent,
      Id new_parent,
      std::vector<mojom::WindowDataPtr> windows) override;
  void OnWindowReordered(Id window_id,
                         Id relative_window_id,
                         mojom::OrderDirection direction) override;
  void OnWindowDeleted(Id window) override;
  void OnWindowVisibilityChanged(Id window, bool visible) override;
  void OnWindowOpacityChanged(Id window,
                              float old_opacity,
                              float new_opacity) override;
  void OnWindowParentDrawnStateChanged(Id window, bool drawn) override;
  void OnWindowSharedPropertyChanged(
      Id window,
      const std::string& name,
      const base::Optional<std::vector<uint8_t>>& new_data) override;
  void OnWindowInputEvent(
      uint32_t event_id,
      Id window,
      int64_t display_id,
      Id display_root_window,
      const gfx::PointF& event_location_in_screen_pixel_layout,
      std::unique_ptr<ui::Event> event,
      bool matches_pointer_watcher) override;
  void OnPointerEventObserved(std::unique_ptr<ui::Event> event,
                              Id window_id,
                              int64_t display_id) override;
  void OnWindowFocused(Id focused_window_id) override;
  void OnWindowCursorChanged(Id window_id, ui::CursorData cursor) override;
  void OnWindowSurfaceChanged(Id window_id,
                              const viz::SurfaceInfo& surface_info) override;
  void OnDragDropStart(const base::flat_map<std::string, std::vector<uint8_t>>&
                           mime_data) override;
  void OnDragEnter(Id window,
                   uint32_t key_state,
                   const gfx::Point& position,
                   uint32_t effect_bitmask,
                   OnDragEnterCallback callback) override;
  void OnDragOver(Id window,
                  uint32_t key_state,
                  const gfx::Point& position,
                  uint32_t effect_bitmask,
                  OnDragOverCallback callback) override;
  void OnDragLeave(Id window) override;
  void OnCompleteDrop(Id window,
                      uint32_t key_state,
                      const gfx::Point& position,
                      uint32_t effect_bitmask,
                      OnCompleteDropCallback callback) override;
  void OnPerformDragDropCompleted(uint32_t change_id,
                                  bool success,
                                  uint32_t action_taken) override;
  void OnDragDropDone() override;
  void OnChangeCompleted(uint32_t change_id, bool success) override;
  void RequestClose(Id window_id) override;
  void GetWindowManager(
      mojo::AssociatedInterfaceRequest<mojom::WindowManager> internal) override;

  TestChangeTracker tracker_;
  mojo::Binding<mojom::WindowTreeClient> binding_;
  bool record_on_change_completed_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestWindowTreeClient);
};

// -----------------------------------------------------------------------------

// WindowTreeBinding implementation that vends TestWindowTreeBinding.
class TestWindowTreeBinding : public WindowTreeBinding {
 public:
  TestWindowTreeBinding(WindowTree* tree,
                        std::unique_ptr<TestWindowTreeClient> client =
                            std::make_unique<TestWindowTreeClient>());
  ~TestWindowTreeBinding() override;

  std::unique_ptr<TestWindowTreeClient> ReleaseClient() {
    return std::move(client_);
  }

  WindowTree* tree() { return tree_; }
  TestWindowTreeClient* client() { return client_.get(); }
  TestWindowManager* window_manager() { return window_manager_.get(); }

  bool is_paused() const { return is_paused_; }

  // WindowTreeBinding:
  mojom::WindowManager* GetWindowManager() override;
  void SetIncomingMethodCallProcessingPaused(bool paused) override;

 protected:
  // WindowTreeBinding:
  mojom::WindowTreeClient* CreateClientForShutdown() override;

 private:
  WindowTree* tree_;
  std::unique_ptr<TestWindowTreeClient> client_;
  // This is the client created once ResetClientForShutdown() is called.
  std::unique_ptr<TestWindowTreeClient> client_after_reset_;
  bool is_paused_ = false;
  std::unique_ptr<TestWindowManager> window_manager_;

  DISALLOW_COPY_AND_ASSIGN(TestWindowTreeBinding);
};

// -----------------------------------------------------------------------------

// WindowServerDelegate that creates TestWindowTreeClients.
class TestWindowServerDelegate : public WindowServerDelegate {
 public:
  TestWindowServerDelegate();
  ~TestWindowServerDelegate() override;

  void set_window_server(WindowServer* window_server) {
    window_server_ = window_server;
  }

  TestWindowTreeClient* last_client() {
    return last_binding() ? last_binding()->client() : nullptr;
  }
  TestWindowTreeBinding* last_binding() {
    return bindings_.empty() ? nullptr : bindings_.back();
  }

  std::vector<TestWindowTreeBinding*>* bindings() { return &bindings_; }

  bool got_on_no_more_displays() const { return got_on_no_more_displays_; }

  // Does an Embed() in |tree| at |window| returning the TestWindowTreeBinding
  // that resulred (null on failure).
  TestWindowTreeBinding* Embed(WindowTree* tree,
                               ServerWindow* window,
                               int flags = 0);

  // WindowServerDelegate:
  void StartDisplayInit() override;
  void OnNoMoreDisplays() override;
  std::unique_ptr<WindowTreeBinding> CreateWindowTreeBinding(
      BindingType type,
      ws::WindowServer* window_server,
      ws::WindowTree* tree,
      mojom::WindowTreeRequest* tree_request,
      mojom::WindowTreeClientPtr* client) override;
  bool IsTestConfig() const override;
  void OnWillCreateTreeForWindowManager(
      bool automatically_create_display_roots) override;
  ThreadedImageCursorsFactory* GetThreadedImageCursorsFactory() override;

 private:
  WindowServer* window_server_ = nullptr;
  bool got_on_no_more_displays_ = false;
  // All TestWindowTreeBinding objects created via CreateWindowTreeBinding.
  // These are owned by the corresponding WindowTree.
  std::vector<TestWindowTreeBinding*> bindings_;
  std::unique_ptr<ThreadedImageCursorsFactory> threaded_image_cursors_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestWindowServerDelegate);
};

// -----------------------------------------------------------------------------

// Helper class which creates and sets up the necessary objects for tests that
// use the WindowServer.
class WindowServerTestHelper {
 public:
  WindowServerTestHelper();
  ~WindowServerTestHelper();

  WindowServer* window_server() { return window_server_.get(); }
  const ui::CursorData& cursor() const { return cursor_; }

  TestWindowServerDelegate* window_server_delegate() {
    return &window_server_delegate_;
  }

 private:
  ui::CursorData cursor_;
  TestPlatformDisplayFactory platform_display_factory_;
  TestWindowServerDelegate window_server_delegate_;
  std::unique_ptr<WindowServer> window_server_;
  std::unique_ptr<base::MessageLoop> message_loop_;

  DISALLOW_COPY_AND_ASSIGN(WindowServerTestHelper);
};

// -----------------------------------------------------------------------------

// Helper class which owns all of the necessary objects to test event targeting
// of ServerWindow objects.
class WindowEventTargetingHelper {
 public:
  explicit WindowEventTargetingHelper(
      bool automatically_create_display_roots = true);
  ~WindowEventTargetingHelper();

  // Creates |window| as an embeded window of the primary tree. This window is a
  // root window of its own tree, with bounds |window_bounds|. The bounds of the
  // root window of |display_| are defined by |root_window_bounds|.
  ServerWindow* CreatePrimaryTree(const gfx::Rect& root_window_bounds,
                                  const gfx::Rect& window_bounds);
  // Creates a secondary tree, embedded as a child of |embed_window|. The
  // resulting |window| is setup for event targeting, with bounds
  // |window_bounds|.
  // TODO(sky): rename and cleanup. This doesn't really create a new tree.
  void CreateSecondaryTree(ServerWindow* embed_window,
                           const gfx::Rect& window_bounds,
                           TestWindowTreeClient** out_client,
                           WindowTree** window_tree,
                           ServerWindow** window);
  // Sets the task runner for |message_loop_|
  void SetTaskRunner(scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  ui::CursorType cursor_type() const {
    return ws_test_helper_.cursor().cursor_type();
  }
  Display* display() { return display_; }
  TestWindowTreeBinding* last_binding() {
    return ws_test_helper_.window_server_delegate()->last_binding();
  }
  TestWindowTreeClient* last_window_tree_client() {
    return ws_test_helper_.window_server_delegate()->last_client();
  }
  TestWindowTreeClient* wm_client() { return wm_client_; }
  WindowServer* window_server() { return ws_test_helper_.window_server(); }

  TestWindowServerDelegate* test_window_server_delegate() {
    return ws_test_helper_.window_server_delegate();
  }

 private:
  WindowServerTestHelper ws_test_helper_;
  // TestWindowTreeClient that is used for the WM client. Owned by
  // |window_server_delegate_|
  TestWindowTreeClient* wm_client_ = nullptr;
  // Owned by WindowServer
  TestDisplayBinding* display_binding_ = nullptr;
  // Owned by WindowServer's DisplayManager.
  Display* display_ = nullptr;
  ClientSpecificId next_primary_tree_window_id_ = kEmbedTreeWindowId;

  DISALLOW_COPY_AND_ASSIGN(WindowEventTargetingHelper);
};

// -----------------------------------------------------------------------------

class TestScreenProviderObserver : public mojom::ScreenProviderObserver {
 public:
  TestScreenProviderObserver();
  ~TestScreenProviderObserver() override;

  mojom::ScreenProviderObserverPtr GetPtr();

  std::string GetAndClearObserverCalls();

 private:
  std::string DisplayIdsToString(
      const std::vector<mojom::WsDisplayPtr>& wm_displays);

  // mojom::ScreenProviderObserver:
  void OnDisplaysChanged(std::vector<mojom::WsDisplayPtr> displays,
                         int64_t primary_display_id,
                         int64_t internal_display_id) override;

  mojo::Binding<mojom::ScreenProviderObserver> binding_;
  std::string observer_calls_;

  DISALLOW_COPY_AND_ASSIGN(TestScreenProviderObserver);
};

// -----------------------------------------------------------------------------
// Empty implementation of PlatformDisplay.
class TestPlatformDisplay : public PlatformDisplay {
 public:
  TestPlatformDisplay(const display::ViewportMetrics& metrics,
                      ui::CursorData* cursor_storage);
  ~TestPlatformDisplay() override;

  display::Display::Rotation cursor_rotation() const {
    return cursor_rotation_;
  }
  float cursor_scale() const { return cursor_scale_; }

  gfx::Rect confine_cursor_bounds() const { return confine_cursor_bounds_; }

  const display::ViewportMetrics& metrics() const { return metrics_; }

  bool has_capture() const { return has_capture_; }

  // PlatformDisplay:
  void Init(PlatformDisplayDelegate* delegate) override;
  void SetViewportSize(const gfx::Size& size) override;
  void SetTitle(const base::string16& title) override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void SetCursor(const ui::CursorData& cursor) override;
  void SetCursorSize(const ui::CursorSize& cursor_size) override;
  void ConfineCursorToBounds(const gfx::Rect& pixel_bounds) override;
  void MoveCursorTo(const gfx::Point& window_pixel_location) override;
  void UpdateTextInputState(const ui::TextInputState& state) override;
  void SetImeVisibility(bool visible) override;
  void UpdateViewportMetrics(const display::ViewportMetrics& metrics) override;
  const display::ViewportMetrics& GetViewportMetrics() override;
  gfx::AcceleratedWidget GetAcceleratedWidget() const override;
  FrameGenerator* GetFrameGenerator() override;
  EventSink* GetEventSink() override;
  void SetCursorConfig(display::Display::Rotation rotation,
                       float scale) override;

 private:
  display::ViewportMetrics metrics_;
  ui::CursorData* cursor_storage_;
  display::Display::Rotation cursor_rotation_ =
      display::Display::Rotation::ROTATE_0;
  float cursor_scale_ = 1.0f;
  gfx::Rect confine_cursor_bounds_;
  bool has_capture_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestPlatformDisplay);
};

// -----------------------------------------------------------------------------

class CursorLocationManagerTestApi {
 public:
  CursorLocationManagerTestApi(CursorLocationManager* cursor_location_manager);
  ~CursorLocationManagerTestApi();

  base::subtle::Atomic32 current_cursor_location();

 private:
  CursorLocationManager* cursor_location_manager_;

  DISALLOW_COPY_AND_ASSIGN(CursorLocationManagerTestApi);
};

// -----------------------------------------------------------------------------

// Adds a WM to |window_server|. Creates WindowManagerWindowTreeFactory and
// associated WindowTree for the WM.
void AddWindowManager(WindowServer* window_server,
                      bool automatically_create_display_roots = true);

// Create a new Display object with specified origin, pixel size and device
// scale factor. The bounds size is computed based on the pixel size and device
// scale factor.
display::Display MakeDisplay(int origin_x,
                             int origin_y,
                             int width_pixels,
                             int height_pixels,
                             float scale_factor);

// Returns the first and only root of |tree|. If |tree| has zero or more than
// one root returns null.
ServerWindow* FirstRoot(WindowTree* tree);

// Returns the ClientWindowId of the first root of |tree|, or an empty
// ClientWindowId if |tree| has zero or more than one root.
ClientWindowId FirstRootId(WindowTree* tree);

// Returns |tree|s ClientWindowId for |window|.
ClientWindowId ClientWindowIdForWindow(WindowTree* tree,
                                       const ServerWindow* window);

// Creates a new visible window as a child of the single root of |tree|.
// |client_id| is set to the ClientWindowId of the new window.
ServerWindow* NewWindowInTree(WindowTree* tree,
                              ClientWindowId* client_id = nullptr);
ServerWindow* NewWindowInTreeWithParent(WindowTree* tree,
                                        ServerWindow* parent,
                                        ClientWindowId* client_id = nullptr,
                                        const gfx::Rect& bounds = gfx::Rect());

// Converts an atomic 32 to a point. The cursor location is represented as an
// atomic 32.
gfx::Point Atomic32ToPoint(base::subtle::Atomic32 atomic);

}  // namespace test
}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_TEST_UTILS_H_
