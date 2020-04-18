// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_server.h"

#include <set>
#include <string>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "components/viz/common/switches.h"
#include "services/ui/ws/display.h"
#include "services/ui/ws/display_creation_config.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/frame_generator.h"
#include "services/ui/ws/gpu_host.h"
#include "services/ui/ws/operation.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/user_activity_monitor.h"
#include "services/ui/ws/window_manager_access_policy.h"
#include "services/ui/ws/window_manager_display_root.h"
#include "services/ui/ws/window_manager_state.h"
#include "services/ui/ws/window_manager_window_tree_factory.h"
#include "services/ui/ws/window_server_delegate.h"
#include "services/ui/ws/window_tree.h"
#include "services/ui/ws/window_tree_binding.h"
#include "ui/gfx/geometry/size_conversions.h"

namespace ui {
namespace ws {

namespace {

// Returns true if |window| is considered the active window manager for
// |display|.
bool IsWindowConsideredWindowManagerRoot(const Display* display,
                                         const ServerWindow* window) {
  if (!display)
    return false;

  const WindowManagerDisplayRoot* display_root =
      display->window_manager_display_root();
  return display_root && display_root->GetClientVisibleRoot() == window;
}

class VizHostProxyImpl : public VizHostProxy {
 public:
  explicit VizHostProxyImpl(viz::HostFrameSinkManager* manager)
      : manager_(manager) {}

  ~VizHostProxyImpl() override = default;

  // VizHostProxy:
  void RegisterFrameSinkId(const viz::FrameSinkId& frame_sink_id,
                           viz::HostFrameSinkClient* client) override {
    if (manager_)
      manager_->RegisterFrameSinkId(frame_sink_id, client);
  }

  void SetFrameSinkDebugLabel(const viz::FrameSinkId& frame_sink_id,
                              const std::string& name) override {
    if (manager_)
      manager_->SetFrameSinkDebugLabel(frame_sink_id, name);
  }

  void InvalidateFrameSinkId(const viz::FrameSinkId& frame_sink_id) override {
    if (manager_)
      manager_->InvalidateFrameSinkId(frame_sink_id);
  }

  void RegisterFrameSinkHierarchy(const viz::FrameSinkId& new_parent,
                                  const viz::FrameSinkId& child) override {
    if (manager_)
      manager_->RegisterFrameSinkHierarchy(new_parent, child);
  }

  void UnregisterFrameSinkHierarchy(const viz::FrameSinkId& old_parent,
                                    const viz::FrameSinkId& child) override {
    if (manager_)
      manager_->UnregisterFrameSinkHierarchy(old_parent, child);
  }

  void CreateRootCompositorFrameSink(
      viz::mojom::RootCompositorFrameSinkParamsPtr params) override {
    // No software compositing on ChromeOS.
    params->gpu_compositing = true;

    if (manager_)
      manager_->CreateRootCompositorFrameSink(std::move(params));
  }

  void CreateCompositorFrameSink(
      const viz::FrameSinkId& frame_sink_id,
      viz::mojom::CompositorFrameSinkRequest request,
      viz::mojom::CompositorFrameSinkClientPtr client) override {
    if (manager_) {
      manager_->CreateCompositorFrameSink(frame_sink_id, std::move(request),
                                          std::move(client));
    }
  }

  viz::HitTestQuery* GetHitTestQuery(
      const viz::FrameSinkId& frame_sink_id) override {
    if (!manager_)
      return nullptr;
    const auto& display_hit_test_query_map = manager_->display_hit_test_query();
    const auto iter = display_hit_test_query_map.find(frame_sink_id);
    return (iter != display_hit_test_query_map.end()) ? iter->second.get()
                                                      : nullptr;
  }

 private:
  viz::HostFrameSinkManager* const manager_;

  DISALLOW_COPY_AND_ASSIGN(VizHostProxyImpl);
};

}  // namespace

struct WindowServer::CurrentMoveLoopState {
  uint32_t change_id;
  ServerWindow* window;
  WindowTree* initiator;
  gfx::Rect revert_bounds;
};

struct WindowServer::CurrentDragLoopState {
  uint32_t change_id;
  ServerWindow* window;
  WindowTree* initiator;
};

WindowServer::WindowServer(WindowServerDelegate* delegate, bool should_host_viz)
    : delegate_(delegate),
      next_client_id_(kWindowServerClientId + 1),
      display_manager_(std::make_unique<DisplayManager>(this, should_host_viz)),
      current_operation_(nullptr),
      in_destructor_(false),
      next_wm_change_id_(0),
      window_manager_window_tree_factory_(
          std::make_unique<WindowManagerWindowTreeFactory>(this)),
      host_frame_sink_manager_(
          should_host_viz ? std::make_unique<viz::HostFrameSinkManager>()
                          : nullptr),
      viz_host_proxy_(
          std::make_unique<VizHostProxyImpl>(host_frame_sink_manager_.get())),
      video_detector_(host_frame_sink_manager_.get()),
      display_creation_config_(DisplayCreationConfig::UNKNOWN) {
  if (host_frame_sink_manager_)
    host_frame_sink_manager_->WillAssignTemporaryReferencesExternally();
  user_activity_monitor_ = std::make_unique<UserActivityMonitor>(nullptr);
}

WindowServer::~WindowServer() {
  in_destructor_ = true;

  for (auto& pair : tree_map_)
    pair.second->PrepareForWindowServerShutdown();

  // Destroys the window trees results in querying for the display. Tear down
  // the displays first so that the trees are notified of the display going
  // away while the display is still valid.
  display_manager_->DestroyAllDisplays();

  while (!tree_map_.empty())
    DestroyTree(tree_map_.begin()->second.get());

  display_manager_.reset();
}

void WindowServer::SetDisplayCreationConfig(DisplayCreationConfig config) {
  DCHECK(tree_map_.empty());
  DCHECK_EQ(DisplayCreationConfig::UNKNOWN, display_creation_config_);
  display_creation_config_ = config;
  display_manager_->OnDisplayCreationConfigSet();
}

void WindowServer::SetGpuHost(std::unique_ptr<GpuHost> gpu_host) {
  DCHECK(host_frame_sink_manager_);
  gpu_host_ = std::move(gpu_host);
  CreateFrameSinkManager();
}

ThreadedImageCursorsFactory* WindowServer::GetThreadedImageCursorsFactory() {
  return delegate()->GetThreadedImageCursorsFactory();
}

ServerWindow* WindowServer::CreateServerWindow(
    const viz::FrameSinkId& frame_sink_id,
    const std::map<std::string, std::vector<uint8_t>>& properties) {
  ServerWindow* window = new ServerWindow(this, frame_sink_id, properties);
  window->AddObserver(this);
  return window;
}

ClientSpecificId WindowServer::GetAndAdvanceNextClientId() {
  const ClientSpecificId id = next_client_id_++;
  CHECK_NE(0u, next_client_id_);
  return id;
}

WindowTree* WindowServer::EmbedAtWindow(
    ServerWindow* root,
    mojom::WindowTreeClientPtr client,
    uint32_t flags,
    std::unique_ptr<AccessPolicy> access_policy) {
  // TODO(sky): I suspect this code needs to reset the FrameSinkId to the
  // ClientWindowId that was used at the time the window was created. As
  // currently if there is a reembed the FrameSinkId from the last embedding
  // is incorrectly used.
  const bool is_for_embedding = true;
  std::unique_ptr<WindowTree> tree_ptr = std::make_unique<WindowTree>(
      this, is_for_embedding, root, std::move(access_policy));
  WindowTree* tree = tree_ptr.get();
  if (flags & mojom::kEmbedFlagEmbedderInterceptsEvents)
    tree->set_embedder_intercepts_events();

  if (flags & mojom::kEmbedFlagEmbedderControlsVisibility)
    tree->set_can_change_root_window_visibility(false);

  mojom::WindowTreePtr window_tree_ptr;
  auto window_tree_request = mojo::MakeRequest(&window_tree_ptr);
  std::unique_ptr<WindowTreeBinding> binding =
      delegate_->CreateWindowTreeBinding(
          WindowServerDelegate::BindingType::EMBED, this, tree,
          &window_tree_request, &client);
  if (!binding) {
    binding = std::make_unique<ws::DefaultWindowTreeBinding>(
        tree, this, std::move(window_tree_request), std::move(client));
  }

  AddTree(std::move(tree_ptr), std::move(binding), std::move(window_tree_ptr));
  OnTreeMessagedClient(tree->id());
  root->UpdateFrameSinkId(viz::FrameSinkId(tree->id(), 0));
  return tree;
}

void WindowServer::AddTree(std::unique_ptr<WindowTree> tree_impl_ptr,
                           std::unique_ptr<WindowTreeBinding> binding,
                           mojom::WindowTreePtr tree_ptr) {
  CHECK_EQ(0u, tree_map_.count(tree_impl_ptr->id()));
  WindowTree* tree = tree_impl_ptr.get();
  tree_map_[tree->id()] = std::move(tree_impl_ptr);
  tree->Init(std::move(binding), std::move(tree_ptr));
}

WindowTree* WindowServer::CreateTreeForWindowManager(
    mojom::WindowTreeRequest window_tree_request,
    mojom::WindowTreeClientPtr window_tree_client,
    bool automatically_create_display_roots) {
  delegate_->OnWillCreateTreeForWindowManager(
      automatically_create_display_roots);

  const bool is_for_embedding = false;
  std::unique_ptr<WindowTree> window_tree = std::make_unique<WindowTree>(
      this, is_for_embedding, nullptr,
      base::WrapUnique(new WindowManagerAccessPolicy));
  std::unique_ptr<WindowTreeBinding> window_tree_binding =
      delegate_->CreateWindowTreeBinding(
          WindowServerDelegate::BindingType::WINDOW_MANAGER, this,
          window_tree.get(), &window_tree_request, &window_tree_client);
  if (!window_tree_binding) {
    window_tree_binding = std::make_unique<DefaultWindowTreeBinding>(
        window_tree.get(), this, std::move(window_tree_request),
        std::move(window_tree_client));
  }
  WindowTree* window_tree_ptr = window_tree.get();
  AddTree(std::move(window_tree), std::move(window_tree_binding), nullptr);
  window_tree_ptr->ConfigureWindowManager(automatically_create_display_roots);
  return window_tree_ptr;
}

void WindowServer::DestroyTree(WindowTree* tree) {
  std::unique_ptr<WindowTree> tree_ptr;
  {
    auto iter = tree_map_.find(tree->id());
    DCHECK(iter != tree_map_.end());
    tree_ptr = std::move(iter->second);
    tree_map_.erase(iter);
  }

  base::EraseIf(scheduled_embeds_,
                [&tree](const std::pair<base::UnguessableToken,
                                        const WindowTreeAndWindowId&>& pair) {
                  return (tree == pair.second.tree);
                });

  // Notify remaining connections so that they can cleanup.
  for (auto& pair : tree_map_)
    pair.second->OnWillDestroyTree(tree);

  if (window_manager_window_tree_factory_->window_tree() == tree)
    window_manager_window_tree_factory_->OnTreeDestroyed();

  // Remove any requests from the client that resulted in a call to the window
  // manager and we haven't gotten a response back yet.
  std::set<uint32_t> to_remove;
  for (auto& pair : in_flight_wm_change_map_) {
    if (pair.second.client_id == tree->id())
      to_remove.insert(pair.first);
  }
  for (uint32_t id : to_remove)
    in_flight_wm_change_map_.erase(id);
}

WindowTree* WindowServer::GetTreeWithId(ClientSpecificId client_id) {
  auto iter = tree_map_.find(client_id);
  return iter == tree_map_.end() ? nullptr : iter->second.get();
}

WindowTree* WindowServer::GetTreeWithClientName(
    const std::string& client_name) {
  for (const auto& entry : tree_map_) {
    if (entry.second->name() == client_name)
      return entry.second.get();
  }
  return nullptr;
}

base::UnguessableToken WindowServer::RegisterEmbedToken(
    WindowTree* tree,
    ClientSpecificId window_id) {
  const base::UnguessableToken token = base::UnguessableToken::Create();
  DCHECK(!scheduled_embeds_.count(token));
  scheduled_embeds_[token] = {tree, window_id};
  return token;
}

WindowTreeAndWindowId WindowServer::UnregisterEmbedToken(
    const base::UnguessableToken& token) {
  auto iter = scheduled_embeds_.find(token);
  if (iter == scheduled_embeds_.end())
    return {};
  WindowTreeAndWindowId result = iter->second;
  scheduled_embeds_.erase(iter);
  return result;
}

void WindowServer::OnTreeMessagedClient(ClientSpecificId id) {
  if (current_operation_)
    current_operation_->MarkTreeAsMessaged(id);
}

bool WindowServer::DidTreeMessageClient(ClientSpecificId id) const {
  return current_operation_ && current_operation_->DidMessageTree(id);
}

const WindowTree* WindowServer::GetTreeWithRoot(
    const ServerWindow* window) const {
  if (!window)
    return nullptr;
  for (auto& pair : tree_map_) {
    if (pair.second->HasRoot(window))
      return pair.second.get();
  }
  return nullptr;
}

void WindowServer::BindWindowManagerWindowTreeFactory(
    mojo::InterfaceRequest<mojom::WindowManagerWindowTreeFactory> request) {
  if (window_manager_window_tree_factory_->is_bound()) {
    DVLOG(1) << "Can only have one WindowManagerWindowTreeFactory";
    return;
  }
  window_manager_window_tree_factory_->Bind(std::move(request));
}

bool WindowServer::SetFocusedWindow(ServerWindow* window) {
  // TODO(sky): this should fail if there is modal dialog active and |window|
  // is outside that.
  ServerWindow* currently_focused = GetFocusedWindow();
  Display* focused_display =
      currently_focused
          ? display_manager_->GetDisplayContaining(currently_focused)
          : nullptr;
  if (!window)
    return focused_display ? focused_display->SetFocusedWindow(nullptr) : true;

  Display* display = display_manager_->GetDisplayContaining(window);
  DCHECK(display);  // It's assumed callers do validation before calling this.
  const bool result = display->SetFocusedWindow(window);
  // If the focus actually changed, and focus was in another display, then we
  // need to notify the previously focused display so that it cleans up state
  // and notifies appropriately.
  if (result && display->GetFocusedWindow() && display != focused_display &&
      focused_display) {
    const bool cleared_focus = focused_display->SetFocusedWindow(nullptr);
    DCHECK(cleared_focus);
  }
  return result;
}

ServerWindow* WindowServer::GetFocusedWindow() {
  for (Display* display : display_manager_->displays()) {
    ServerWindow* focused_window = display->GetFocusedWindow();
    if (focused_window)
      return focused_window;
  }
  return nullptr;
}

void WindowServer::SetHighContrastMode(bool enabled) {
  // TODO(fsamuel): This doesn't really seem like it's a window server concept?
  if (high_contrast_mode_ == enabled)
    return;
  high_contrast_mode_ = enabled;

  // Propagate the change to all Displays so that FrameGenerators start
  // requesting BeginFrames.
  display_manager_->SetHighContrastMode(enabled);
}

uint32_t WindowServer::GenerateWindowManagerChangeId(
    WindowTree* source,
    uint32_t client_change_id) {
  const uint32_t wm_change_id = next_wm_change_id_++;
  in_flight_wm_change_map_[wm_change_id] = {source->id(), client_change_id};
  return wm_change_id;
}

void WindowServer::WindowManagerChangeCompleted(
    uint32_t window_manager_change_id,
    bool success) {
  InFlightWindowManagerChange change;
  if (!GetAndClearInFlightWindowManagerChange(window_manager_change_id,
                                              &change)) {
    return;
  }

  WindowTree* tree = GetTreeWithId(change.client_id);
  tree->OnChangeCompleted(change.client_change_id, success);
}

void WindowServer::WindowManagerCreatedTopLevelWindow(
    WindowTree* wm_tree,
    uint32_t window_manager_change_id,
    ServerWindow* window) {
  InFlightWindowManagerChange change;
  if (!GetAndClearInFlightWindowManagerChange(window_manager_change_id,
                                              &change)) {
    DVLOG(1) << "WindowManager responded with invalid change id; most "
             << "likely bug in WindowManager processing WmCreateTopLevelWindow "
             << "change_id=" << window_manager_change_id;
    return;
  }

  WindowTree* tree = GetTreeWithId(change.client_id);
  // The window manager should have created the window already, and it should
  // be ready for embedding.
  if (!tree->IsWaitingForNewTopLevelWindow(window_manager_change_id)) {
    DVLOG(1) << "WindowManager responded with valid change id, but client "
             << "is not waiting for top-level; possible bug in mus, change_id="
             << window_manager_change_id;
    WindowManagerSentBogusMessage();
    return;
  }
  if (window && (window->owning_tree_id() != wm_tree->id() ||
                 !window->children().empty() || GetTreeWithRoot(window))) {
    DVLOG(1)
        << "WindowManager responded with invalid window; window should "
        << "not have any children, not be the root of a client and should be "
        << "created by the WindowManager, window_manager_change_id="
        << window_manager_change_id;
    WindowManagerSentBogusMessage();
    return;
  }

  viz::FrameSinkId updated_frame_sink_id =
      tree->OnWindowManagerCreatedTopLevelWindow(
          window_manager_change_id, change.client_change_id, window);
  if (window)
    window->UpdateFrameSinkId(updated_frame_sink_id);
}

void WindowServer::ProcessWindowBoundsChanged(
    const ServerWindow* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowBoundsChanged(window, old_bounds, new_bounds,
                                            IsOperationSource(pair.first),
                                            local_surface_id);
  }
}

void WindowServer::ProcessWindowTransformChanged(
    const ServerWindow* window,
    const gfx::Transform& old_transform,
    const gfx::Transform& new_transform) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowTransformChanged(
        window, old_transform, new_transform, IsOperationSource(pair.first));
  }
}

void WindowServer::ProcessClientAreaChanged(
    const ServerWindow* window,
    const gfx::Insets& new_client_area,
    const std::vector<gfx::Rect>& new_additional_client_areas) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessClientAreaChanged(window, new_client_area,
                                          new_additional_client_areas,
                                          IsOperationSource(pair.first));
  }
}

void WindowServer::ProcessCaptureChanged(const ServerWindow* new_capture,
                                         const ServerWindow* old_capture) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessCaptureChanged(new_capture, old_capture,
                                       IsOperationSource(pair.first));
  }
}

void WindowServer::ProcessWillChangeWindowHierarchy(
    const ServerWindow* window,
    const ServerWindow* new_parent,
    const ServerWindow* old_parent) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWillChangeWindowHierarchy(
        window, new_parent, old_parent, IsOperationSource(pair.first));
  }
}

void WindowServer::ProcessWindowHierarchyChanged(
    const ServerWindow* window,
    const ServerWindow* new_parent,
    const ServerWindow* old_parent) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowHierarchyChanged(window, new_parent, old_parent,
                                               IsOperationSource(pair.first));
  }
}

void WindowServer::ProcessWindowReorder(const ServerWindow* window,
                                        const ServerWindow* relative_window,
                                        const mojom::OrderDirection direction) {
  // We'll probably do a bit of reshuffling when we add a transient window.
  if ((current_operation_type() == OperationType::ADD_TRANSIENT_WINDOW) ||
      (current_operation_type() ==
       OperationType::REMOVE_TRANSIENT_WINDOW_FROM_PARENT)) {
    return;
  }
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowReorder(window, relative_window, direction,
                                      IsOperationSource(pair.first));
  }
}

void WindowServer::ProcessWindowDeleted(ServerWindow* window) {
  for (auto& pair : tree_map_)
    pair.second->ProcessWindowDeleted(window, IsOperationSource(pair.first));
}

void WindowServer::ProcessWillChangeWindowCursor(ServerWindow* window,
                                                 const ui::CursorData& cursor) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessCursorChanged(window, cursor,
                                      IsOperationSource(pair.first));
  }
}

void WindowServer::SendToPointerWatchers(const ui::Event& event,
                                         ServerWindow* target_window,
                                         WindowTree* ignore_tree,
                                         int64_t display_id) {
  for (auto& pair : tree_map_) {
    WindowTree* tree = pair.second.get();
    if (tree != ignore_tree)
      tree->SendToPointerWatcher(event, target_window, display_id);
  }
}

void WindowServer::SetSurfaceActivationCallback(
    base::OnceCallback<void(ServerWindow*)> callback) {
  DCHECK(delegate_->IsTestConfig()) << "Surface activation callbacks are "
                                    << "expensive, and allowed only in tests.";
  DCHECK(surface_activation_callback_.is_null() || callback.is_null());
  surface_activation_callback_ = std::move(callback);
}

void WindowServer::StartMoveLoop(uint32_t change_id,
                                 ServerWindow* window,
                                 WindowTree* initiator,
                                 const gfx::Rect& revert_bounds) {
  current_move_loop_.reset(
      new CurrentMoveLoopState{change_id, window, initiator, revert_bounds});
}

void WindowServer::EndMoveLoop() {
  current_move_loop_.reset();
}

uint32_t WindowServer::GetCurrentMoveLoopChangeId() {
  if (current_move_loop_)
    return current_move_loop_->change_id;
  return 0;
}

ServerWindow* WindowServer::GetCurrentMoveLoopWindow() {
  if (current_move_loop_)
    return current_move_loop_->window;
  return nullptr;
}

WindowTree* WindowServer::GetCurrentMoveLoopInitiator() {
  if (current_move_loop_)
    return current_move_loop_->initiator;
  return nullptr;
}

gfx::Rect WindowServer::GetCurrentMoveLoopRevertBounds() {
  if (current_move_loop_)
    return current_move_loop_->revert_bounds;
  return gfx::Rect();
}

void WindowServer::StartDragLoop(uint32_t change_id,
                                 ServerWindow* window,
                                 WindowTree* initiator) {
  current_drag_loop_.reset(
      new CurrentDragLoopState{change_id, window, initiator});
}

void WindowServer::EndDragLoop() {
  current_drag_loop_.reset();
}

uint32_t WindowServer::GetCurrentDragLoopChangeId() {
  if (current_drag_loop_)
    return current_drag_loop_->change_id;
  return 0u;
}

ServerWindow* WindowServer::GetCurrentDragLoopWindow() {
  if (current_drag_loop_)
    return current_drag_loop_->window;
  return nullptr;
}

WindowTree* WindowServer::GetCurrentDragLoopInitiator() {
  if (current_drag_loop_)
    return current_drag_loop_->initiator;
  return nullptr;
}

void WindowServer::OnDisplayReady(Display* display, bool is_first) {
  if (is_first)
    delegate_->OnFirstDisplayReady();
  bool wm_is_hosting_viz = !gpu_host_;
  if (wm_is_hosting_viz) {
    // Notify WM about the AcceleratedWidget if it is hosting viz.
    for (auto& pair : tree_map_) {
      if (pair.second->window_manager_state()) {
        auto& wm_tree = pair.second;
        wm_tree->OnAcceleratedWidgetAvailableForDisplay(display);
      }
    }
  } else {
    gpu_host_->OnAcceleratedWidgetAvailable(
        display->platform_display()->GetAcceleratedWidget());
  }
}

void WindowServer::OnDisplayDestroyed(Display* display) {
  if (gpu_host_) {
    gpu_host_->OnAcceleratedWidgetDestroyed(
        display->platform_display()->GetAcceleratedWidget());
  }
}

void WindowServer::OnNoMoreDisplays() {
  delegate_->OnNoMoreDisplays();
}

WindowManagerState* WindowServer::GetWindowManagerState() {
  return window_manager_window_tree_factory_->window_tree()
             ? window_manager_window_tree_factory_->window_tree()
                   ->window_manager_state()
             : nullptr;
}

VizHostProxy* WindowServer::GetVizHostProxy() {
  return viz_host_proxy_.get();
}

void WindowServer::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info,
    ServerWindow* window) {
  DCHECK(host_frame_sink_manager_);
  // This is only used for testing to observe that a window has a
  // CompositorFrame.
  if (surface_activation_callback_)
    std::move(surface_activation_callback_).Run(window);

  Display* display = display_manager_->GetDisplayContaining(window);
  if (IsWindowConsideredWindowManagerRoot(display, window)) {
    // A new surface for a WindowManager root has been created. This is a
    // special case because ServerWindows created by the WindowServer are not
    // part of a WindowTree. Send the SurfaceId directly to FrameGenerator and
    // claim the temporary reference for the display root.
    display_manager_->OnWindowManagerSurfaceActivation(display, surface_info);
    host_frame_sink_manager_->AssignTemporaryReference(
        surface_info.id(), display->root_window()->frame_sink_id());
    return;
  }

  HandleTemporaryReferenceForNewSurface(surface_info.id(), window);

  // We always use the owner of the window's id (even for an embedded window),
  // because an embedded window's id is allocated by the parent's window tree.
  WindowTree* window_tree = GetTreeWithId(window->owning_tree_id());
  if (window_tree)
    window_tree->ProcessWindowSurfaceChanged(window, surface_info);
}

bool WindowServer::GetFrameDecorations(
    mojom::FrameDecorationValuesPtr* values) {
  WindowManagerState* window_manager_state = GetWindowManagerState();
  if (!window_manager_state)
    return false;
  if (values && window_manager_state->got_frame_decoration_values())
    *values = window_manager_state->frame_decoration_values().Clone();
  return window_manager_state->got_frame_decoration_values();
}

int64_t WindowServer::GetInternalDisplayId() {
  return display_manager_->GetInternalDisplayId();
}

bool WindowServer::GetAndClearInFlightWindowManagerChange(
    uint32_t window_manager_change_id,
    InFlightWindowManagerChange* change) {
  // There are valid reasons as to why we wouldn't know about the id. The
  // most likely is the client disconnected before the response from the window
  // manager came back.
  auto iter = in_flight_wm_change_map_.find(window_manager_change_id);
  if (iter == in_flight_wm_change_map_.end())
    return false;

  *change = iter->second;
  in_flight_wm_change_map_.erase(iter);
  return true;
}

void WindowServer::PrepareForOperation(Operation* op) {
  // Should only ever have one change in flight.
  CHECK(!current_operation_);
  current_operation_ = op;
}

void WindowServer::FinishOperation() {
  // PrepareForOperation/FinishOperation should be balanced.
  CHECK(current_operation_);
  current_operation_ = nullptr;
}

void WindowServer::UpdateNativeCursorFromMouseLocation(ServerWindow* window) {
  UpdateNativeCursorFromMouseLocation(
      display_manager_->GetWindowManagerDisplayRoot(window));
}

void WindowServer::UpdateNativeCursorFromMouseLocation(
    WindowManagerDisplayRoot* display_root) {
  if (!display_root)
    return;

  EventProcessor* event_processor =
      display_root->window_manager_state()->event_processor();
  event_processor->UpdateCursorProviderByLastKnownLocation();
}

void WindowServer::UpdateNativeCursorIfOver(ServerWindow* window) {
  WindowManagerDisplayRoot* display_root =
      display_manager_->GetWindowManagerDisplayRoot(window);
  if (!display_root)
    return;

  EventProcessor* event_processor =
      display_root->window_manager_state()->event_processor();
  if (window != event_processor->GetWindowForMouseCursor())
    return;

  event_processor->UpdateNonClientAreaForCurrentWindow();
}

void WindowServer::HandleTemporaryReferenceForNewSurface(
    const viz::SurfaceId& surface_id,
    ServerWindow* window) {
  DCHECK(host_frame_sink_manager_);
  // TODO(kylechar): Investigate adding tests for this.
  const ClientSpecificId window_client_id = window->owning_tree_id();

  // Find the root ServerWindow for the client that embeds |window|, which is
  // the root of the client that embeds |surface_id|. The client that embeds
  // |surface_id| created |window|, so |window| will have the client id of the
  // embedder. The root window of the embedder will have been created by it's
  // embedder, so the first ServerWindow with a different client id will be the
  // root of the embedder.
  ServerWindow* current = window->parent();
  while (current && current->owning_tree_id() == window_client_id)
    current = current->parent();

  // The client that embeds |window| is expected to submit a CompositorFrame
  // that references |surface_id|. Have the parent claim ownership of the
  // temporary reference to |surface_id|. If the parent client crashes before it
  // adds a surface reference then the GPU can cleanup temporary references. If
  // no parent client embeds |window| then tell the GPU to drop the temporary
  // reference immediately.
  if (current) {
    host_frame_sink_manager_->AssignTemporaryReference(
        surface_id, current->frame_sink_id());
  } else {
    host_frame_sink_manager_->DropTemporaryReference(surface_id);
  }
}

void WindowServer::CreateFrameSinkManager() {
  DCHECK(host_frame_sink_manager_);
  viz::mojom::FrameSinkManagerPtr frame_sink_manager;
  viz::mojom::FrameSinkManagerRequest frame_sink_manager_request =
      mojo::MakeRequest(&frame_sink_manager);
  viz::mojom::FrameSinkManagerClientPtr frame_sink_manager_client;
  viz::mojom::FrameSinkManagerClientRequest frame_sink_manager_client_request =
      mojo::MakeRequest(&frame_sink_manager_client);

  viz::mojom::FrameSinkManagerParamsPtr params =
      viz::mojom::FrameSinkManagerParams::New();
  params->restart_id = viz_restart_id_++;
  base::Optional<uint32_t> activation_deadline_in_frames =
      switches::GetDeadlineToSynchronizeSurfaces();
  params->use_activation_deadline = activation_deadline_in_frames.has_value();
  params->activation_deadline_in_frames =
      activation_deadline_in_frames.value_or(0u);
  params->frame_sink_manager = std::move(frame_sink_manager_request);
  params->frame_sink_manager_client = frame_sink_manager_client.PassInterface();
  gpu_host_->CreateFrameSinkManager(std::move(params));

  host_frame_sink_manager_->BindAndSetManager(
      std::move(frame_sink_manager_client_request), nullptr /* task_runner */,
      std::move(frame_sink_manager));
}

AsyncEventDispatcher* WindowServer::GetAsyncEventDispatcherById(
    ClientSpecificId id) {
  return GetTreeWithId(id);
}

ServerWindow* WindowServer::GetRootWindowForDrawn(const ServerWindow* window) {
  Display* display = display_manager_->GetDisplayContaining(window);
  return display ? display->root_window() : nullptr;
}

void WindowServer::OnWindowDestroyed(ServerWindow* window) {
  ProcessWindowDeleted(window);
}

void WindowServer::OnWillChangeWindowHierarchy(ServerWindow* window,
                                               ServerWindow* new_parent,
                                               ServerWindow* old_parent) {
  if (in_destructor_)
    return;

  ProcessWillChangeWindowHierarchy(window, new_parent, old_parent);
}

void WindowServer::OnWindowHierarchyChanged(ServerWindow* window,
                                            ServerWindow* new_parent,
                                            ServerWindow* old_parent) {
  if (in_destructor_)
    return;

  WindowManagerDisplayRoot* display_root =
      display_manager_->GetWindowManagerDisplayRoot(window);
  if (display_root) {
    display_root->window_manager_state()
        ->ReleaseCaptureBlockedByAnyModalWindow();
  }

  ProcessWindowHierarchyChanged(window, new_parent, old_parent);

  if (old_parent) {
    viz_host_proxy_->UnregisterFrameSinkHierarchy(old_parent->frame_sink_id(),
                                                  window->frame_sink_id());
  }
  if (new_parent) {
    viz_host_proxy_->RegisterFrameSinkHierarchy(new_parent->frame_sink_id(),
                                                window->frame_sink_id());
  }

  if (!pending_system_modal_windows_.windows().empty()) {
    // Windows that are now in a display are put here, then removed. We do this
    // in two passes to avoid removing from a list we're iterating over.
    std::set<ServerWindow*> no_longer_pending;
    for (ServerWindow* system_modal_window :
         pending_system_modal_windows_.windows()) {
      DCHECK_EQ(MODAL_TYPE_SYSTEM, system_modal_window->modal_type());
      WindowManagerDisplayRoot* display_root =
          display_manager_->GetWindowManagerDisplayRoot(system_modal_window);
      if (display_root) {
        no_longer_pending.insert(system_modal_window);
        display_root->window_manager_state()->AddSystemModalWindow(window);
      }
    }

    for (ServerWindow* system_modal_window : no_longer_pending)
      pending_system_modal_windows_.Remove(system_modal_window);
  }

  WindowManagerDisplayRoot* old_display_root =
      old_parent ? display_manager_->GetWindowManagerDisplayRoot(old_parent)
                 : nullptr;
  WindowManagerDisplayRoot* new_display_root =
      new_parent ? display_manager_->GetWindowManagerDisplayRoot(new_parent)
                 : nullptr;
  UpdateNativeCursorFromMouseLocation(new_display_root);
  if (old_display_root != new_display_root)
    UpdateNativeCursorFromMouseLocation(old_display_root);
}

void WindowServer::OnWindowBoundsChanged(ServerWindow* window,
                                         const gfx::Rect& old_bounds,
                                         const gfx::Rect& new_bounds) {
  if (in_destructor_)
    return;

  ProcessWindowBoundsChanged(window, old_bounds, new_bounds,
                             window->current_local_surface_id());
  UpdateNativeCursorFromMouseLocation(window);
}

void WindowServer::OnWindowTransformChanged(
    ServerWindow* window,
    const gfx::Transform& old_transform,
    const gfx::Transform& new_transform) {
  if (in_destructor_)
    return;

  ProcessWindowTransformChanged(window, old_transform, new_transform);
  UpdateNativeCursorFromMouseLocation(window);
}

void WindowServer::OnWindowClientAreaChanged(
    ServerWindow* window,
    const gfx::Insets& new_client_area,
    const std::vector<gfx::Rect>& new_additional_client_areas) {
  if (in_destructor_)
    return;

  ProcessClientAreaChanged(window, new_client_area,
                           new_additional_client_areas);

  UpdateNativeCursorIfOver(window);
}

void WindowServer::OnWindowReordered(ServerWindow* window,
                                     ServerWindow* relative,
                                     mojom::OrderDirection direction) {
  ProcessWindowReorder(window, relative, direction);
  UpdateNativeCursorFromMouseLocation(window);
}

void WindowServer::OnWillChangeWindowVisibility(ServerWindow* window) {
  if (in_destructor_)
    return;

  for (auto& pair : tree_map_) {
    pair.second->ProcessWillChangeWindowVisibility(
        window, IsOperationSource(pair.first));
  }
}

void WindowServer::OnWindowOpacityChanged(ServerWindow* window,
                                          float old_opacity,
                                          float new_opacity) {
  DCHECK(!in_destructor_);

  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowOpacityChanged(window, old_opacity, new_opacity,
                                             IsOperationSource(pair.first));
  }
}

void WindowServer::OnWindowVisibilityChanged(ServerWindow* window) {
  if (in_destructor_)
    return;

  WindowManagerDisplayRoot* display_root =
      display_manager_->GetWindowManagerDisplayRoot(window);
  if (display_root) {
    display_root->window_manager_state()
        ->ReleaseCaptureBlockedByAnyModalWindow();
  }
}

void WindowServer::OnWindowCursorChanged(ServerWindow* window,
                                         const ui::CursorData& cursor) {
  if (in_destructor_)
    return;

  ProcessWillChangeWindowCursor(window, cursor);

  UpdateNativeCursorIfOver(window);
}

void WindowServer::OnWindowNonClientCursorChanged(
    ServerWindow* window,
    const ui::CursorData& cursor) {
  if (in_destructor_)
    return;

  UpdateNativeCursorIfOver(window);
}

void WindowServer::OnWindowSharedPropertyChanged(
    ServerWindow* window,
    const std::string& name,
    const std::vector<uint8_t>* new_data) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessWindowPropertyChanged(window, name, new_data,
                                              IsOperationSource(pair.first));
  }
}

void WindowServer::OnWindowTextInputStateChanged(
    ServerWindow* window,
    const ui::TextInputState& state) {
  Display* display = display_manager_->GetDisplayContaining(window);
  display->UpdateTextInputState(window, state);
}

void WindowServer::OnTransientWindowAdded(ServerWindow* window,
                                          ServerWindow* transient_child) {
  for (auto& pair : tree_map_) {
    pair.second->ProcessTransientWindowAdded(window, transient_child,
                                             IsOperationSource(pair.first));
  }
}

void WindowServer::OnTransientWindowRemoved(ServerWindow* window,
                                            ServerWindow* transient_child) {
  // If we're deleting a window, then this is a superfluous message.
  if (current_operation_type() == OperationType::DELETE_WINDOW)
    return;
  for (auto& pair : tree_map_) {
    pair.second->ProcessTransientWindowRemoved(window, transient_child,
                                               IsOperationSource(pair.first));
  }
}

void WindowServer::OnWindowModalTypeChanged(ServerWindow* window,
                                            ModalType old_modal_type) {
  WindowManagerDisplayRoot* display_root =
      display_manager_->GetWindowManagerDisplayRoot(window);
  if (window->modal_type() == MODAL_TYPE_SYSTEM) {
    if (display_root)
      display_root->window_manager_state()->AddSystemModalWindow(window);
    else
      pending_system_modal_windows_.Add(window);
  } else {
    pending_system_modal_windows_.Remove(window);
  }

  if (display_root && window->modal_type() != MODAL_TYPE_NONE) {
    display_root->window_manager_state()
        ->ReleaseCaptureBlockedByAnyModalWindow();
  }
}

void WindowServer::OnGpuServiceInitialized() {
  delegate_->StartDisplayInit();
}

}  // namespace ws
}  // namespace ui
