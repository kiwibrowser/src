// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_tree.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/map.h"
#include "services/ui/common/util.h"
#include "services/ui/display/screen_manager.h"
#include "services/ui/ws/cursor_location_manager.h"
#include "services/ui/ws/debug_utils.h"
#include "services/ui/ws/default_access_policy.h"
#include "services/ui/ws/display.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/event_location.h"
#include "services/ui/ws/event_matcher.h"
#include "services/ui/ws/event_processor.h"
#include "services/ui/ws/focus_controller.h"
#include "services/ui/ws/frame_generator.h"
#include "services/ui/ws/modal_window_controller.h"
#include "services/ui/ws/operation.h"
#include "services/ui/ws/platform_display.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/server_window_observer.h"
#include "services/ui/ws/user_display_manager.h"
#include "services/ui/ws/window_manager_display_root.h"
#include "services/ui/ws/window_manager_state.h"
#include "services/ui/ws/window_server.h"
#include "services/ui/ws/window_tree_binding.h"
#include "ui/base/cursor/cursor.h"
#include "ui/display/display.h"
#include "ui/display/display_list.h"
#include "ui/display/screen_base.h"
#include "ui/display/types/display_constants.h"
#include "ui/platform_window/mojo/ime_type_converters.h"
#include "ui/platform_window/text_input_state.h"

using mojo::InterfaceRequest;

using EventProperties = base::flat_map<std::string, std::vector<uint8_t>>;

namespace ui {
namespace ws {
namespace {

bool HasPositiveInset(const gfx::Insets& insets) {
  return insets.width() > 0 || insets.height() > 0 || insets.left() > 0 ||
         insets.right() > 0;
}

FrameGenerator* GetFrameGenerator(WindowManagerDisplayRoot* display_root) {
  return display_root->display()->platform_display()
             ? display_root->display()->platform_display()->GetFrameGenerator()
             : nullptr;
}

display::ViewportMetrics TransportMetricsToDisplayMetrics(
    const ui::mojom::WmViewportMetrics& transport_metrics) {
  display::ViewportMetrics viewport_metrics;
  viewport_metrics.bounds_in_pixels = transport_metrics.bounds_in_pixels;
  viewport_metrics.device_scale_factor = transport_metrics.device_scale_factor;
  viewport_metrics.ui_scale_factor = transport_metrics.ui_scale_factor;
  return viewport_metrics;
}

}  // namespace

class TargetedEvent : public ServerWindowObserver {
 public:
  TargetedEvent(ServerWindow* target,
                const ui::Event& event,
                const EventLocation& event_location,
                WindowTree::DispatchEventCallback callback)
      : target_(target),
        event_(ui::Event::Clone(event)),
        event_location_(event_location),
        callback_(std::move(callback)) {
    target_->AddObserver(this);
  }
  ~TargetedEvent() override {
    if (target_)
      target_->RemoveObserver(this);
  }

  ServerWindow* target() { return target_; }
  std::unique_ptr<ui::Event> TakeEvent() { return std::move(event_); }
  const EventLocation& event_location() const { return event_location_; }
  WindowTree::DispatchEventCallback TakeCallback() {
    return std::move(callback_);
  }

 private:
  // ServerWindowObserver:
  void OnWindowDestroyed(ServerWindow* window) override {
    DCHECK_EQ(target_, window);
    target_->RemoveObserver(this);
    target_ = nullptr;
  }

  ServerWindow* target_;
  std::unique_ptr<ui::Event> event_;
  const EventLocation event_location_;
  WindowTree::DispatchEventCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(TargetedEvent);
};

struct WindowTree::DragMoveState {
  // Whether we've queued a move to |queued_cursor_location_| when we get an
  // ack from WmMoveDragImage.
  bool has_queued_drag_window_move = false;

  // When |has_queued_drag_window_move_| is true, this is a location which
  // should be sent to the window manager as soon as it acked the last one.
  gfx::Point queued_cursor_location;
};

WindowTree::WindowTree(WindowServer* window_server,
                       bool is_for_embedding,
                       ServerWindow* root,
                       std::unique_ptr<AccessPolicy> access_policy)
    : window_server_(window_server),
      is_for_embedding_(is_for_embedding),
      id_(window_server_->GetAndAdvanceNextClientId()),
      access_policy_(std::move(access_policy)),
      event_ack_id_(0),
      window_manager_internal_(nullptr),
      drag_weak_factory_(this) {
  if (root)
    roots_.insert(root);
  access_policy_->Init(id_, this);
}

WindowTree::~WindowTree() {
  DestroyWindows();

  // We alert the WindowManagerState that we're destroying this state here
  // because WindowManagerState would attempt to use things that wouldn't have
  // been cleaned up by OnWindowDestroyingTreeImpl().
  if (window_manager_state_) {
    window_manager_state_->OnWillDestroyTree(this);
    window_manager_state_.reset();
  }
}

void WindowTree::Init(std::unique_ptr<WindowTreeBinding> binding,
                      mojom::WindowTreePtr tree) {
  DCHECK(!binding_);
  binding_ = std::move(binding);

  if (roots_.empty())
    return;

  std::vector<const ServerWindow*> to_send;
  CHECK_EQ(1u, roots_.size());
  const ServerWindow* root = *roots_.begin();
  GetUnknownWindowsFrom(root, &to_send, nullptr);

  Display* display = GetDisplay(root);
  int64_t display_id = display ? display->GetId() : display::kInvalidDisplayId;
  const ServerWindow* focused_window =
      display ? display->GetFocusedWindow() : nullptr;
  if (focused_window)
    focused_window = access_policy_->GetWindowForFocusChange(focused_window);
  ClientWindowId focused_window_id;
  if (focused_window)
    IsWindowKnown(focused_window, &focused_window_id);

  const bool drawn = root->parent() && root->parent()->IsDrawn();
  client()->OnEmbed(WindowToWindowData(to_send.front()), std::move(tree),
                    display_id, ClientWindowIdToTransportId(focused_window_id),
                    drawn, root->current_local_surface_id());
}

void WindowTree::OnAcceleratedWidgetAvailableForDisplay(Display* display) {
  DCHECK(window_manager_internal_);
  // TODO(sad): Use GpuSurfaceTracker on platforms where a gpu::SurfaceHandle is
  // not the same as a gfx::AcceleratedWidget.
  window_manager_internal_->WmOnAcceleratedWidgetForDisplay(
      display->GetId(), display->platform_display()->GetAcceleratedWidget());
}

void WindowTree::ConfigureWindowManager(
    bool automatically_create_display_roots) {
  // ConfigureWindowManager() should be called early on, before anything
  // else. |waiting_for_top_level_window_info_| must be null as if
  // |waiting_for_top_level_window_info_| is non-null it means we're about to
  // create an associated interface, which doesn't work with pause/resume.
  // TODO(sky): DCHECK temporary until 626869 is sorted out.
  DCHECK(!waiting_for_top_level_window_info_);
  DCHECK(!window_manager_internal_);
  automatically_create_display_roots_ = automatically_create_display_roots;
  window_manager_internal_ = binding_->GetWindowManager();
  window_manager_internal_->OnConnect();
  window_manager_state_ = std::make_unique<WindowManagerState>(this);
}

bool WindowTree::IsWindowKnown(const ServerWindow* window,
                               ClientWindowId* id) const {
  if (!window)
    return false;
  auto iter = window_to_client_id_map_.find(window);
  if (iter == window_to_client_id_map_.end())
    return false;
  if (id)
    *id = iter->second;
  return true;
}

bool WindowTree::HasRoot(const ServerWindow* window) const {
  return roots_.count(window) > 0;
}

const ServerWindow* WindowTree::GetWindowByClientId(
    const ClientWindowId& id) const {
  auto iter = client_id_to_window_map_.find(id);
  return iter == client_id_to_window_map_.end() ? nullptr : iter->second;
}

const Display* WindowTree::GetDisplay(const ServerWindow* window) const {
  return window ? display_manager()->GetDisplayContaining(window) : nullptr;
}

const WindowManagerDisplayRoot* WindowTree::GetWindowManagerDisplayRoot(
    const ServerWindow* window) const {
  return window ? display_manager()->GetWindowManagerDisplayRoot(window)
                : nullptr;
}

DisplayManager* WindowTree::display_manager() {
  return window_server_->display_manager();
}

const DisplayManager* WindowTree::display_manager() const {
  return window_server_->display_manager();
}

void WindowTree::PrepareForWindowServerShutdown() {
  window_manager_internal_client_binding_.reset();
  binding_->ResetClientForShutdown();
  if (window_manager_internal_)
    window_manager_internal_ = binding_->GetWindowManager();
}

void WindowTree::AddRootForWindowManager(const ServerWindow* root) {
  if (!automatically_create_display_roots_)
    return;

  DCHECK(automatically_create_display_roots_);
  DCHECK(window_manager_internal_);
  const ClientWindowId client_window_id = root->frame_sink_id();
  AddToMaps(root, client_window_id);
  roots_.insert(root);

  Display* ws_display = GetDisplay(root);
  DCHECK(ws_display);

  window_manager_internal_->WmNewDisplayAdded(
      ws_display->GetDisplay(), WindowToWindowData(root),
      root->parent()->IsDrawn(), root->current_local_surface_id());
}

void WindowTree::OnWillDestroyTree(WindowTree* tree) {
  DCHECK_NE(tree, this);  // This function is not called for |this|.

  if (event_source_wms_ && event_source_wms_->window_tree() == tree)
    event_source_wms_ = nullptr;

  // Notify our client if |tree| was embedded in any of our windows.
  for (const auto* tree_root : tree->roots_) {
    const bool owns_tree_root = tree_root->owning_tree_id() == id_;
    if (owns_tree_root)
      client()->OnEmbeddedAppDisconnected(TransportIdForWindow(tree_root));
  }

  if (window_manager_state_)
    window_manager_state_->OnWillDestroyTree(tree);
}

void WindowTree::OnWmDisplayModified(const display::Display& display) {
  window_manager_internal_->WmDisplayModified(display);
}

void WindowTree::NotifyChangeCompleted(
    uint32_t change_id,
    mojom::WindowManagerErrorCode error_code) {
  client()->OnChangeCompleted(
      change_id, error_code == mojom::WindowManagerErrorCode::SUCCESS);
}

void WindowTree::OnWmMoveDragImageAck() {
  if (drag_move_state_->has_queued_drag_window_move) {
    gfx::Point queued_location = drag_move_state_->queued_cursor_location;
    drag_move_state_.reset();
    OnDragMoved(queued_location);
  } else {
    drag_move_state_.reset();
  }
}

ServerWindow* WindowTree::ProcessSetDisplayRoot(
    const display::Display& display_to_create,
    const display::ViewportMetrics& viewport_metrics,
    bool is_primary_display,
    const ClientWindowId& client_window_id,
    const std::vector<display::Display>& mirrors) {
  DCHECK(window_manager_state_);  // Only called for window manager.
  DVLOG(3) << "SetDisplayRoot client=" << id_
           << " global window_id=" << client_window_id.ToString();

  if (automatically_create_display_roots_) {
    DVLOG(1) << "SetDisplayRoot is only applicable when "
             << "automatically_create_display_roots is false";
    return nullptr;
  }

  ServerWindow* window = GetWindowByClientId(client_window_id);
  const bool is_moving_to_new_display =
      window && window->parent() && base::ContainsKey(roots_, window);
  if (!window || (window->parent() && !is_moving_to_new_display)) {
    DVLOG(1) << "SetDisplayRoot called with invalid window id "
             << client_window_id.ToString();
    return nullptr;
  }

  if (base::ContainsKey(roots_, window) && !is_moving_to_new_display) {
    DVLOG(1) << "SetDisplayRoot called with existing root";
    return nullptr;
  }

  Display* display = display_manager()->GetDisplayById(display_to_create.id());
  const bool display_already_existed = display != nullptr;
  if (!display) {
    // Create a display if the window manager is extending onto a new display.
    display = display_manager()->AddDisplayForWindowManager(
        is_primary_display, display_to_create, viewport_metrics);
  } else if (!display->window_manager_display_root()) {
    // Init the root if the display already existed as a mirroring destination.
    display->InitWindowManagerDisplayRoots();
  }

  if (!mirrors.empty())
    NOTIMPLEMENTED() << "TODO(crbug.com/806318): Mus+Viz mirroring/unified";

  DCHECK(display);
  WindowManagerDisplayRoot* display_root =
      display->window_manager_display_root();
  DCHECK(display_root);
  display_root->root()->RemoveAllChildren();

  // NOTE: this doesn't resize the window in any way. We assume the client takes
  // care of any modifications it needs to do.
  roots_.insert(window);
  Operation op(this, window_server_, OperationType::ADD_WINDOW);
  ServerWindow* old_parent =
      is_moving_to_new_display ? window->parent() : nullptr;
  display_root->root()->Add(window);
  if (is_moving_to_new_display) {
    DCHECK(old_parent);
    window_manager_state_->DeleteWindowManagerDisplayRoot(old_parent);
  }
  if (display_already_existed &&
      display->platform_display()->GetAcceleratedWidget()) {
    // Notify the window manager that the dispay's accelerated widget is already
    // available, if the display is being reused for a new window tree host.
    window_manager_internal_->WmOnAcceleratedWidgetForDisplay(
        display->GetId(), display->platform_display()->GetAcceleratedWidget());
  }
  return window;
}

bool WindowTree::ProcessSwapDisplayRoots(int64_t display_id1,
                                         int64_t display_id2) {
  DCHECK(window_manager_state_);  // Can only be called by the window manager.
  DVLOG(3) << "SwapDisplayRoots display_id1=" << display_id2
           << " display_id2=" << display_id2;
  if (automatically_create_display_roots_) {
    DVLOG(1) << "SwapDisplayRoots only applicable when window-manager creates "
             << "display roots";
    return false;
  }
  Display* display1 = display_manager()->GetDisplayById(display_id1);
  Display* display2 = display_manager()->GetDisplayById(display_id2);
  if (!display1 || !display2) {
    DVLOG(1) << "SwapDisplayRoots called with unknown display ids";
    return false;
  }

  WindowManagerDisplayRoot* display_root1 =
      display1->window_manager_display_root();
  WindowManagerDisplayRoot* display_root2 =
      display2->window_manager_display_root();

  if (!display_root1->GetClientVisibleRoot() ||
      !display_root2->GetClientVisibleRoot()) {
    DVLOG(1) << "SetDisplayRoot called with displays that have not been "
             << "configured";
    return false;
  }

  display_root1->root()->Add(display_root2->GetClientVisibleRoot());
  display_root2->root()->Add(display_root1->GetClientVisibleRoot());

  // TODO(sky): this is race condition here if one is valid and one null.
  FrameGenerator* frame_generator1 = GetFrameGenerator(display_root1);
  FrameGenerator* frame_generator2 = GetFrameGenerator(display_root2);
  if (frame_generator1 && frame_generator2)
    frame_generator1->SwapSurfaceWith(frame_generator2);

  return true;
}

bool WindowTree::ProcessSetBlockingContainers(
    std::vector<mojom::BlockingContainersPtr>
        transport_all_blocking_containers) {
  DCHECK(window_manager_state_);  // Can only be called by the window manager.

  std::vector<BlockingContainers> all_containers;
  for (auto& transport_container : transport_all_blocking_containers) {
    BlockingContainers blocking_containers;
    blocking_containers.system_modal_container = GetWindowByClientId(
        MakeClientWindowId(transport_container->system_modal_container_id));
    if (!blocking_containers.system_modal_container) {
      DVLOG(1) << "SetBlockingContainers called with unknown modal container";
      return false;
    }
    blocking_containers.min_container = GetWindowByClientId(
        MakeClientWindowId(transport_container->min_container_id));
    all_containers.push_back(blocking_containers);
  }
  window_manager_state_->event_processor()
      ->modal_window_controller()
      ->SetBlockingContainers(all_containers);
  return true;
}

bool WindowTree::SetCapture(const ClientWindowId& client_window_id) {
  ServerWindow* window = GetWindowByClientId(client_window_id);
  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  ServerWindow* current_capture_window =
      display_root ? display_root->window_manager_state()->capture_window()
                   : nullptr;
  if (window && window->IsDrawn() && display_root &&
      access_policy_->CanSetCapture(window) &&
      (!current_capture_window ||
       access_policy_->CanSetCapture(current_capture_window))) {
    Operation op(this, window_server_, OperationType::SET_CAPTURE);
    return display_root->window_manager_state()->SetCapture(window, id_);
  }
  return false;
}

bool WindowTree::ReleaseCapture(const ClientWindowId& client_window_id) {
  ServerWindow* window = GetWindowByClientId(client_window_id);
  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  ServerWindow* current_capture_window =
      display_root ? display_root->window_manager_state()->capture_window()
                   : nullptr;
  if (!window || !display_root ||
      (current_capture_window &&
       !access_policy_->CanSetCapture(current_capture_window)) ||
      window != current_capture_window) {
    return false;
  }
  Operation op(this, window_server_, OperationType::RELEASE_CAPTURE);
  return display_root->window_manager_state()->SetCapture(nullptr,
                                                          kInvalidClientId);
}

void WindowTree::DispatchEvent(ServerWindow* target,
                               const ui::Event& event,
                               const EventLocation& event_location,
                               DispatchEventCallback callback) {
  if (event_ack_id_ || !event_queue_.empty()) {
    // Either awaiting an ack, or there are events in the queue. Store the event
    // for processing when the ack is received.
    event_queue_.push(std::make_unique<TargetedEvent>(
        target, event, event_location, std::move(callback)));
    // TODO(sad): If the |event_queue_| grows too large, then this should notify
    // Display, so that it can stop sending events.
    return;
  }

  DispatchEventImpl(target, event, event_location, std::move(callback));
}

void WindowTree::DispatchAccelerator(uint32_t accelerator_id,
                                     const ui::Event& event,
                                     AcceleratorCallback callback) {
  DVLOG(3) << "OnAccelerator client=" << id_;
  DCHECK(window_manager_internal_);  // Only valid for the window manager.
  if (callback) {
    GenerateEventAckId();
    accelerator_ack_callback_ = std::move(callback);
  } else {
    DCHECK_EQ(0u, event_ack_id_);
    DCHECK(!accelerator_ack_callback_);
  }
  // TODO: https://crbug.com/617167. Don't clone event once we map mojom::Event
  // directly to ui::Event.
  window_manager_internal_->OnAccelerator(event_ack_id_, accelerator_id,
                                          ui::Event::Clone(event));
}

bool WindowTree::NewWindow(
    const ClientWindowId& client_window_id,
    const std::map<std::string, std::vector<uint8_t>>& properties) {
  DVLOG(3) << "new window client=" << id_
           << " window_id=" << client_window_id.ToString();
  if (!IsValidIdForNewWindow(client_window_id)) {
    DVLOG(1) << "NewWindow failed (id is not valid for client)";
    return false;
  }
  DCHECK(!GetWindowByClientId(client_window_id));
  DCHECK_EQ(id_, client_window_id.client_id());
  ServerWindow* window =
      window_server_->CreateServerWindow(client_window_id, properties);
  created_windows_.insert(window);
  AddToMaps(window, client_window_id);
  return true;
}

bool WindowTree::AddWindow(const ClientWindowId& parent_id,
                           const ClientWindowId& child_id) {
  ServerWindow* parent = GetWindowByClientId(parent_id);
  ServerWindow* child = GetWindowByClientId(child_id);
  DVLOG(3) << "add window client=" << id_
           << " client parent window_id=" << parent_id.ToString()
           << " global window_id=" << DebugWindowId(parent)
           << " client child window_id= " << child_id.ToString()
           << " global window_id=" << DebugWindowId(child);
  if (!parent) {
    DVLOG(1) << "AddWindow failed (no parent)";
    return false;
  }
  if (!child) {
    DVLOG(1) << "AddWindow failed (no child)";
    return false;
  }
  if (child->parent() == parent) {
    DVLOG(1) << "AddWindow failed (already has parent)";
    return false;
  }
  if (child->Contains(parent)) {
    DVLOG(1) << "AddWindow failed (child contains parent)";
    return false;
  }
  if (!access_policy_->CanAddWindow(parent, child)) {
    DVLOG(1) << "AddWindow failed (access denied)";
    return false;
  }
  Operation op(this, window_server_, OperationType::ADD_WINDOW);
  parent->Add(child);
  return true;
}

bool WindowTree::AddTransientWindow(const ClientWindowId& window_id,
                                    const ClientWindowId& transient_window_id) {
  ServerWindow* window = GetWindowByClientId(window_id);
  ServerWindow* transient_window = GetWindowByClientId(transient_window_id);
  if (window && transient_window && !transient_window->Contains(window) &&
      access_policy_->CanAddTransientWindow(window, transient_window)) {
    Operation op(this, window_server_, OperationType::ADD_TRANSIENT_WINDOW);
    return window->AddTransientWindow(transient_window);
  }
  return false;
}

bool WindowTree::DeleteWindow(const ClientWindowId& window_id) {
  ServerWindow* window = GetWindowByClientId(window_id);
  DVLOG(3) << "removing window from parent client=" << id_
           << " client window_id= " << window_id.ToString()
           << " global window_id=" << DebugWindowId(window);
  if (!window)
    return false;

  if (roots_.count(window) > 0) {
    // Deleting a root behaves as an unembed.
    window_server_->OnTreeMessagedClient(id_);
    RemoveRoot(window, RemoveRootReason::UNEMBED);
    return true;
  }

  if (!access_policy_->CanDeleteWindow(window) &&
      !ShouldRouteToWindowManager(window)) {
    return false;
  }

  // Have the owner of the tree service the actual delete.
  WindowTree* tree = window_server_->GetTreeWithId(window->owning_tree_id());
  return tree && tree->DeleteWindowImpl(this, window);
}

bool WindowTree::SetModalType(const ClientWindowId& window_id,
                              ModalType modal_type) {
  ServerWindow* window = GetWindowByClientId(window_id);
  if (!window) {
    DVLOG(1) << "SetModalType failed (invalid id)";
    return false;
  }

  if (is_for_embedding_ && modal_type == MODAL_TYPE_SYSTEM) {
    DVLOG(1) << "SetModalType failed (not allowed for embedded clients)";
    return false;
  }

  if (ShouldRouteToWindowManager(window)) {
    WindowTree* wm_tree = GetWindowManagerDisplayRoot(window)
                              ->window_manager_state()
                              ->window_tree();
    wm_tree->window_manager_internal_->WmSetModalType(
        wm_tree->TransportIdForWindow(window), modal_type);
    return true;
  }

  if (!access_policy_->CanSetModal(window)) {
    DVLOG(1) << "SetModalType failed (access denied)";
    return false;
  }

  if (window->modal_type() == modal_type)
    return true;

  window->SetModalType(modal_type);
  return true;
}

bool WindowTree::SetChildModalParent(
    const ClientWindowId& window_id,
    const ClientWindowId& modal_parent_window_id) {
  ServerWindow* window = GetWindowByClientId(window_id);
  ServerWindow* modal_parent_window =
      GetWindowByClientId(modal_parent_window_id);
  // A value of null for |modal_parent_window| resets the modal parent.
  if (!window) {
    DVLOG(1) << "SetChildModalParent failed (invalid id)";
    return false;
  }

  if (!access_policy_->CanSetChildModalParent(window, modal_parent_window)) {
    DVLOG(1) << "SetChildModalParent failed (access denied)";
    return false;
  }

  window->SetChildModalParent(modal_parent_window);
  return true;
}

std::vector<const ServerWindow*> WindowTree::GetWindowTree(
    const ClientWindowId& window_id) const {
  const ServerWindow* window = GetWindowByClientId(window_id);
  std::vector<const ServerWindow*> windows;
  if (window)
    GetWindowTreeImpl(window, &windows);
  return windows;
}

bool WindowTree::SetWindowVisibility(const ClientWindowId& window_id,
                                     bool visible) {
  ServerWindow* window = GetWindowByClientId(window_id);
  DVLOG(3) << "SetWindowVisibility client=" << id_
           << " client window_id= " << window_id.ToString()
           << " global window_id=" << DebugWindowId(window);
  if (!window) {
    DVLOG(1) << "SetWindowVisibility failed (no window)";
    return false;
  }
  if (!access_policy_->CanChangeWindowVisibility(window) ||
      (!can_change_root_window_visibility_ && HasRoot(window))) {
    DVLOG(1) << "SetWindowVisibility failed (access policy denied change)";
    return false;
  }
  if (window->visible() == visible)
    return true;
  Operation op(this, window_server_, OperationType::SET_WINDOW_VISIBILITY);
  window->SetVisible(visible);
  return true;
}

bool WindowTree::SetWindowOpacity(const ClientWindowId& window_id,
                                  float opacity) {
  ServerWindow* window = GetWindowByClientId(window_id);
  if (!window || !access_policy_->CanChangeWindowOpacity(window))
    return false;
  if (window->opacity() == opacity)
    return true;
  Operation op(this, window_server_, OperationType::SET_WINDOW_OPACITY);
  window->SetOpacity(opacity);
  return true;
}

bool WindowTree::SetFocus(const ClientWindowId& window_id) {
  ServerWindow* window = GetWindowByClientId(window_id);
  ServerWindow* currently_focused = window_server_->GetFocusedWindow();
  DVLOG(3) << "SetFocusedWindow client=" << id_
           << " client window_id=" << window_id.ToString()
           << " window=" << DebugWindowId(window);
  if (currently_focused == window)
    return true;

  Display* display = GetDisplay(window);
  if (window && (!display || !window->can_focus() || !window->IsDrawn())) {
    DVLOG(1) << "SetFocus failed (window cannot be focused)";
    return false;
  }

  if (!access_policy_->CanSetFocus(window)) {
    DVLOG(1) << "SetFocus failed (blocked by access policy)";
    return false;
  }

  Operation op(this, window_server_, OperationType::SET_FOCUS);
  bool success = window_server_->SetFocusedWindow(window);
  if (!success)
    DVLOG(1) << "SetFocus failed (could not SetFocusedWindow)";
  return success;
}

bool WindowTree::Embed(const ClientWindowId& window_id,
                       mojom::WindowTreeClientPtr window_tree_client,
                       uint32_t flags) {
  if (!window_tree_client || !CanEmbed(window_id))
    return false;
  ServerWindow* window = GetWindowByClientId(window_id);
  DCHECK(window);  // CanEmbed() returns false if no window.
  PrepareForEmbed(window);
  // mojom::kEmbedFlagEmbedderInterceptsEvents is inherited, otherwise an
  // embedder could effectively circumvent it by embedding itself.
  if (embedder_intercepts_events_)
    flags = mojom::kEmbedFlagEmbedderInterceptsEvents;
  window_server_->EmbedAtWindow(window, std::move(window_tree_client), flags,
                                base::WrapUnique(new DefaultAccessPolicy));
  client()->OnFrameSinkIdAllocated(ClientWindowIdToTransportId(window_id),
                                   window->frame_sink_id());
  return true;
}

bool WindowTree::EmbedExistingTree(
    const ClientWindowId& window_id,
    const WindowTreeAndWindowId& tree_and_window_id,
    const base::UnguessableToken& token) {
  const ClientWindowId window_id_in_embedded(tree_and_window_id.tree->id(),
                                             tree_and_window_id.window_id);
  if (!CanEmbed(window_id) ||
      !tree_and_window_id.tree->IsValidIdForNewWindow(window_id_in_embedded)) {
    return false;
  }
  ServerWindow* window = GetWindowByClientId(window_id);
  DCHECK(window);  // CanEmbed() returns false if no window.
  PrepareForEmbed(window);
  tree_and_window_id.tree->AddRootForToken(token, window,
                                           window_id_in_embedded);
  window->UpdateFrameSinkId(window_id_in_embedded);
  client()->OnFrameSinkIdAllocated(ClientWindowIdToTransportId(window_id),
                                   window->frame_sink_id());
  return true;
}

bool WindowTree::IsWaitingForNewTopLevelWindow(uint32_t wm_change_id) {
  return waiting_for_top_level_window_info_ &&
         waiting_for_top_level_window_info_->wm_change_id == wm_change_id;
}

viz::FrameSinkId WindowTree::OnWindowManagerCreatedTopLevelWindow(
    uint32_t wm_change_id,
    uint32_t client_change_id,
    const ServerWindow* window) {
  DCHECK(IsWaitingForNewTopLevelWindow(wm_change_id));
  std::unique_ptr<WaitingForTopLevelWindowInfo>
      waiting_for_top_level_window_info(
          std::move(waiting_for_top_level_window_info_));
  binding_->SetIncomingMethodCallProcessingPaused(false);
  // We were paused, so the id should still be valid.
  DCHECK(IsValidIdForNewWindow(
      waiting_for_top_level_window_info->client_window_id));
  if (!window) {
    client()->OnChangeCompleted(client_change_id, false);
    return viz::FrameSinkId();
  }
  AddToMaps(window, waiting_for_top_level_window_info->client_window_id);
  roots_.insert(window);
  Display* display = GetDisplay(window);
  int64_t display_id = display ? display->GetId() : display::kInvalidDisplayId;
  const bool drawn = window->parent() && window->parent()->IsDrawn();
  client()->OnTopLevelCreated(client_change_id, WindowToWindowData(window),
                              display_id, drawn,
                              window->current_local_surface_id());
  return waiting_for_top_level_window_info->client_window_id;
}

void WindowTree::AddActivationParent(const ClientWindowId& window_id) {
  ServerWindow* window = GetWindowByClientId(window_id);
  if (window)
    window->set_is_activation_parent(true);
  else
    DVLOG(1) << "AddActivationParent failed (invalid window id)";
}

void WindowTree::OnChangeCompleted(uint32_t change_id, bool success) {
  client()->OnChangeCompleted(change_id, success);
}

void WindowTree::OnEventOccurredOutsideOfModalWindow(
    const ServerWindow* modal_window) {
  DCHECK(window_manager_internal_);
  // Only tell the window manager about windows it created.
  if (modal_window->owning_tree_id() != id_)
    return;

  ClientWindowId client_window_id;
  const bool is_known = IsWindowKnown(modal_window, &client_window_id);
  // The window manager knows all windows.
  DCHECK(is_known);
  window_manager_internal_->OnEventBlockedByModalWindow(
      ClientWindowIdToTransportId(client_window_id));
}

void WindowTree::OnCursorTouchVisibleChanged(bool enabled) {
  DCHECK(window_manager_internal_);
  window_manager_internal_->OnCursorTouchVisibleChanged(enabled);
}

void WindowTree::OnDisplayDestroying(int64_t display_id) {
  DCHECK(window_manager_internal_);
  if (automatically_create_display_roots_)
    window_manager_internal_->WmDisplayRemoved(display_id);
  // For the else case the client should detect removal directly.
}

void WindowTree::ClientJankinessChanged(WindowTree* tree) {
  tree->janky_ = !tree->janky_;
  // Don't inform the client if it is the source of jank (which generally only
  // happens while debugging).
  if (window_manager_internal_ && tree != this) {
    window_manager_internal_->WmClientJankinessChanged(
        tree->id(), tree->janky());
  }
}

void WindowTree::ProcessWindowBoundsChanged(
    const ServerWindow* window,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds,
    bool originated_change,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  ClientWindowId client_window_id;
  if (originated_change || !IsWindowKnown(window, &client_window_id))
    return;
  client()->OnWindowBoundsChanged(ClientWindowIdToTransportId(client_window_id),
                                  old_bounds, new_bounds, local_surface_id);
}

void WindowTree::ProcessWindowTransformChanged(
    const ServerWindow* window,
    const gfx::Transform& old_transform,
    const gfx::Transform& new_transform,
    bool originated_change) {
  ClientWindowId client_window_id;
  if (originated_change || !IsWindowKnown(window, &client_window_id))
    return;
  client()->OnWindowTransformChanged(
      ClientWindowIdToTransportId(client_window_id), old_transform,
      new_transform);
}

void WindowTree::ProcessClientAreaChanged(
    const ServerWindow* window,
    const gfx::Insets& new_client_area,
    const std::vector<gfx::Rect>& new_additional_client_areas,
    bool originated_change) {
  ClientWindowId client_window_id;
  if (originated_change || !IsWindowKnown(window, &client_window_id))
    return;
  client()->OnClientAreaChanged(
      ClientWindowIdToTransportId(client_window_id), new_client_area,
      std::vector<gfx::Rect>(new_additional_client_areas));
}

void WindowTree::ProcessWillChangeWindowHierarchy(
    const ServerWindow* window,
    const ServerWindow* new_parent,
    const ServerWindow* old_parent,
    bool originated_change) {
  if (originated_change)
    return;

  const bool old_drawn = window->IsDrawn();
  const bool new_drawn =
      window->visible() && new_parent && new_parent->IsDrawn();
  if (old_drawn == new_drawn)
    return;

  NotifyDrawnStateChanged(window, new_drawn);
}

void WindowTree::ProcessWindowPropertyChanged(
    const ServerWindow* window,
    const std::string& name,
    const std::vector<uint8_t>* new_data,
    bool originated_change) {
  if (originated_change)
    return;

  ClientWindowId client_window_id;
  if (!IsWindowKnown(window, &client_window_id))
    return;

  base::Optional<std::vector<uint8_t>> data;
  if (new_data)
    data.emplace(*new_data);

  client()->OnWindowSharedPropertyChanged(
      ClientWindowIdToTransportId(client_window_id), name, data);
}

void WindowTree::ProcessWindowHierarchyChanged(const ServerWindow* window,
                                               const ServerWindow* new_parent,
                                               const ServerWindow* old_parent,
                                               bool originated_change) {
  const bool knows_new = new_parent && IsWindowKnown(new_parent);
  if (originated_change || (window_server_->current_operation_type() ==
                            OperationType::DELETE_WINDOW) ||
      (window_server_->current_operation_type() == OperationType::EMBED) ||
      window_server_->DidTreeMessageClient(id_)) {
    return;
  }

  if (!access_policy_->ShouldNotifyOnHierarchyChange(window, &new_parent,
                                                     &old_parent)) {
    return;
  }
  // Inform the client of any new windows and update the set of windows we know
  // about.
  std::vector<const ServerWindow*> to_send;
  if (!IsWindowKnown(window))
    GetUnknownWindowsFrom(window, &to_send, nullptr);
  const bool knows_old = old_parent && IsWindowKnown(old_parent);
  if (!knows_old && !knows_new)
    return;

  const Id new_parent_client_window_id =
      knows_new ? TransportIdForWindow(new_parent) : kInvalidTransportId;
  const Id old_parent_client_window_id =
      knows_old ? TransportIdForWindow(old_parent) : kInvalidTransportId;
  const Id client_window_id =
      window ? TransportIdForWindow(window) : kInvalidTransportId;
  client()->OnWindowHierarchyChanged(
      client_window_id, old_parent_client_window_id,
      new_parent_client_window_id, WindowsToWindowDatas(to_send));
  window_server_->OnTreeMessagedClient(id_);
}

void WindowTree::ProcessWindowReorder(const ServerWindow* window,
                                      const ServerWindow* relative_window,
                                      mojom::OrderDirection direction,
                                      bool originated_change) {
  DCHECK_EQ(window->parent(), relative_window->parent());
  ClientWindowId client_window_id, relative_client_window_id;
  if (originated_change || !IsWindowKnown(window, &client_window_id) ||
      !IsWindowKnown(relative_window, &relative_client_window_id) ||
      window_server_->DidTreeMessageClient(id_))
    return;

  // Do not notify ordering changes of the root windows, since the client
  // doesn't know about the ancestors of the roots, and so can't do anything
  // about this ordering change of the root.
  if (HasRoot(window) || HasRoot(relative_window))
    return;

  client()->OnWindowReordered(
      ClientWindowIdToTransportId(client_window_id),
      ClientWindowIdToTransportId(relative_client_window_id), direction);
  window_server_->OnTreeMessagedClient(id_);
}

void WindowTree::ProcessWindowDeleted(ServerWindow* window,
                                      bool originated_change) {
  created_windows_.erase(window);

  ClientWindowId client_window_id;
  if (!IsWindowKnown(window, &client_window_id))
    return;

  if (HasRoot(window))
    RemoveRoot(window, RemoveRootReason::DELETED);
  else
    RemoveFromMaps(window);

  if (originated_change)
    return;

  client()->OnWindowDeleted(ClientWindowIdToTransportId(client_window_id));
  window_server_->OnTreeMessagedClient(id_);
}

void WindowTree::ProcessWillChangeWindowVisibility(const ServerWindow* window,
                                                   bool originated_change) {
  if (originated_change)
    return;

  ClientWindowId client_window_id;
  if (IsWindowKnown(window, &client_window_id)) {
    client()->OnWindowVisibilityChanged(
        ClientWindowIdToTransportId(client_window_id), !window->visible());
    return;
  }

  bool window_target_drawn_state;
  if (window->visible()) {
    // Window is being hidden, won't be drawn.
    window_target_drawn_state = false;
  } else {
    // Window is being shown. Window will be drawn if its parent is drawn.
    window_target_drawn_state = window->parent() && window->parent()->IsDrawn();
  }

  NotifyDrawnStateChanged(window, window_target_drawn_state);
}

void WindowTree::ProcessWindowOpacityChanged(const ServerWindow* window,
                                             float old_opacity,
                                             float new_opacity,
                                             bool originated_change) {
  if (originated_change)
    return;

  ClientWindowId client_window_id;
  if (IsWindowKnown(window, &client_window_id)) {
    client()->OnWindowOpacityChanged(
        ClientWindowIdToTransportId(client_window_id), old_opacity,
        new_opacity);
  }
}

void WindowTree::ProcessCursorChanged(const ServerWindow* window,
                                      const ui::CursorData& cursor,
                                      bool originated_change) {
  if (originated_change)
    return;
  ClientWindowId client_window_id;
  if (!IsWindowKnown(window, &client_window_id))
    return;

  client()->OnWindowCursorChanged(ClientWindowIdToTransportId(client_window_id),
                                  cursor);
}

void WindowTree::ProcessFocusChanged(const ServerWindow* old_focused_window,
                                     const ServerWindow* new_focused_window) {
  if (window_server_->current_operation_type() == OperationType::SET_FOCUS &&
      window_server_->IsOperationSource(id_)) {
    return;
  }
  const ServerWindow* window =
      new_focused_window
          ? access_policy_->GetWindowForFocusChange(new_focused_window)
          : nullptr;
  ClientWindowId client_window_id;
  // If the window isn't known we'll supply null, which is ok.
  IsWindowKnown(window, &client_window_id);
  client()->OnWindowFocused(ClientWindowIdToTransportId(client_window_id));
}

void WindowTree::ProcessTransientWindowAdded(
    const ServerWindow* window,
    const ServerWindow* transient_window,
    bool originated_change) {
  if (originated_change)
    return;

  ClientWindowId client_window_id, transient_client_window_id;
  if (!IsWindowKnown(window, &client_window_id) ||
      !IsWindowKnown(transient_window, &transient_client_window_id)) {
    return;
  }
  client()->OnTransientWindowAdded(
      ClientWindowIdToTransportId(client_window_id),
      ClientWindowIdToTransportId(transient_client_window_id));
}

void WindowTree::ProcessTransientWindowRemoved(
    const ServerWindow* window,
    const ServerWindow* transient_window,
    bool originated_change) {
  if (originated_change)
    return;
  ClientWindowId client_window_id, transient_client_window_id;
  if (!IsWindowKnown(window, &client_window_id) ||
      !IsWindowKnown(transient_window, &transient_client_window_id)) {
    return;
  }
  client()->OnTransientWindowRemoved(
      ClientWindowIdToTransportId(client_window_id),
      ClientWindowIdToTransportId(transient_client_window_id));
}

void WindowTree::ProcessWindowSurfaceChanged(
    ServerWindow* window,
    const viz::SurfaceInfo& surface_info) {
  ClientWindowId client_window_id;
  if (!IsWindowKnown(window, &client_window_id))
    return;
  client()->OnWindowSurfaceChanged(
      ClientWindowIdToTransportId(client_window_id), surface_info);
}

void WindowTree::SendToPointerWatcher(const ui::Event& event,
                                      ServerWindow* target_window,
                                      int64_t display_id) {
  if (!EventMatchesPointerWatcher(event))
    return;

  ClientWindowId client_window_id;
  // Ignore the return value from IsWindowKnown() as in the case of the client
  // not knowing the window we'll send 0, which corresponds to no window.
  IsWindowKnown(target_window, &client_window_id);
  client()->OnPointerEventObserved(
      ui::Event::Clone(event), ClientWindowIdToTransportId(client_window_id),
      display_id);
}

Id WindowTree::ClientWindowIdToTransportId(
    const ClientWindowId& client_window_id) const {
  if (client_window_id.client_id() == id_)
    return client_window_id.sink_id();
  const Id client_id = client_window_id.client_id();
  return (client_id << 32) | client_window_id.sink_id();
}

bool WindowTree::ShouldRouteToWindowManager(const ServerWindow* window) const {
  if (window_manager_state_)
    return false;  // We are the window manager, don't route to ourself.

  // If the client created this window, then do not route it through the WM.
  if (window->owning_tree_id() == id_)
    return false;

  // If the client did not create the window, then it must be the root of the
  // client. If not, that means the client should not know about this window,
  // and so do not route the request to the WM.
  if (roots_.count(window) == 0)
    return false;

  return IsWindowCreatedByWindowManager(window);
}

void WindowTree::ProcessCaptureChanged(const ServerWindow* new_capture,
                                       const ServerWindow* old_capture,
                                       bool originated_change) {
  ClientWindowId new_capture_window_client_id;
  ClientWindowId old_capture_window_client_id;
  const bool new_capture_window_known =
      IsWindowKnown(new_capture, &new_capture_window_client_id);
  const bool old_capture_window_known =
      IsWindowKnown(old_capture, &old_capture_window_client_id);
  if (!new_capture_window_known && !old_capture_window_known)
    return;

  if (originated_change && ((window_server_->current_operation_type() ==
                             OperationType::RELEASE_CAPTURE) ||
                            (window_server_->current_operation_type() ==
                             OperationType::SET_CAPTURE))) {
    return;
  }

  client()->OnCaptureChanged(
      ClientWindowIdToTransportId(new_capture_window_client_id),
      ClientWindowIdToTransportId(old_capture_window_client_id));
}

Id WindowTree::TransportIdForWindow(const ServerWindow* window) const {
  auto iter = window_to_client_id_map_.find(window);
  DCHECK(iter != window_to_client_id_map_.end());
  return ClientWindowIdToTransportId(iter->second);
}

bool WindowTree::IsValidIdForNewWindow(const ClientWindowId& id) const {
  // Reserve 0 (ClientWindowId() and sink_id) to indicate a null window.
  return client_id_to_window_map_.count(id) == 0u &&
         access_policy_->IsValidIdForNewWindow(id) && id != ClientWindowId() &&
         id.sink_id() != 0;
}

bool WindowTree::CanReorderWindow(const ServerWindow* window,
                                  const ServerWindow* relative_window,
                                  mojom::OrderDirection direction) const {
  if (!window) {
    DVLOG(1) << "CanReorderWindow failed (invalid window)";
    return false;
  }
  if (!relative_window) {
    DVLOG(1) << "CanReorderWindow failed (invalid relative window)";
    return false;
  }

  if (!window->parent()) {
    DVLOG(1) << "CanReorderWindow failed (no parent)";
    return false;
  }

  if (window->parent() != relative_window->parent()) {
    DVLOG(1) << "CanReorderWindow failed (parents differ)";
    return false;
  }

  if (!access_policy_->CanReorderWindow(window, relative_window, direction)) {
    DVLOG(1) << "CanReorderWindow failed (access policy denied)";
    return false;
  }

  const ServerWindow::Windows& children = window->parent()->children();
  const size_t child_i =
      std::find(children.begin(), children.end(), window) - children.begin();
  const size_t target_i =
      std::find(children.begin(), children.end(), relative_window) -
      children.begin();
  if ((direction == mojom::OrderDirection::ABOVE && child_i == target_i + 1) ||
      (direction == mojom::OrderDirection::BELOW && child_i + 1 == target_i)) {
    DVLOG(1) << "CanReorderWindow failed (already in position)";
    return false;
  }

  return true;
}

bool WindowTree::RemoveWindowFromParent(
    const ClientWindowId& client_window_id) {
  ServerWindow* window = GetWindowByClientId(client_window_id);
  DVLOG(3) << "removing window from parent client=" << id_
           << " client window_id= " << client_window_id
           << " global window_id=" << DebugWindowId(window);
  if (!window) {
    DVLOG(1) << "RemoveWindowFromParent failed (invalid window id="
             << client_window_id.ToString() << ")";
    return false;
  }
  if (!window->parent()) {
    DVLOG(1) << "RemoveWindowFromParent failed (no parent id="
             << client_window_id.ToString() << ")";
    return false;
  }
  if (!access_policy_->CanRemoveWindowFromParent(window)) {
    DVLOG(1) << "RemoveWindowFromParent failed (access policy disallowed id="
             << client_window_id.ToString() << ")";
    return false;
  }
  Operation op(this, window_server_, OperationType::REMOVE_WINDOW_FROM_PARENT);
  window->parent()->Remove(window);
  return true;
}

bool WindowTree::DeleteWindowImpl(WindowTree* source, ServerWindow* window) {
  DCHECK(window);
  DCHECK_EQ(window->owning_tree_id(), id_);
  Operation op(source, window_server_, OperationType::DELETE_WINDOW);
  delete window;
  return true;
}

void WindowTree::GetUnknownWindowsFrom(
    const ServerWindow* window,
    std::vector<const ServerWindow*>* windows,
    const ClientWindowId* id_for_window) {
  if (!access_policy_->CanGetWindowTree(window))
    return;

  // This function is called in the context of a hierarchy change when the
  // parent wasn't known. We need to tell the client about the window so that
  // it can set the parent correctly.
  if (windows)
    windows->push_back(window);
  if (IsWindowKnown(window))
    return;

  const ClientWindowId client_window_id =
      id_for_window ? *id_for_window : window->frame_sink_id();
  AddToMaps(window, client_window_id);
  if (!access_policy_->CanDescendIntoWindowForWindowTree(window))
    return;
  const ServerWindow::Windows& children = window->children();
  for (ServerWindow* child : children)
    GetUnknownWindowsFrom(child, windows, nullptr);
}

void WindowTree::AddToMaps(const ServerWindow* window,
                           const ClientWindowId& client_window_id) {
  DCHECK_EQ(0u, client_id_to_window_map_.count(client_window_id));
  client_id_to_window_map_[client_window_id] = window;
  window_to_client_id_map_[window] = client_window_id;
}

void WindowTree::AddRootForToken(const base::UnguessableToken& token,
                                 ServerWindow* window,
                                 const ClientWindowId& client_window_id) {
  roots_.insert(window);
  Display* display = GetDisplay(window);
  int64_t display_id = display ? display->GetId() : display::kInvalidDisplayId;
  // Caller should have verified there is no window already registered for
  // |client_window_id|.
  DCHECK(!GetWindowByClientId(client_window_id));
  GetUnknownWindowsFrom(window, nullptr, &client_window_id);
  client()->OnEmbedFromToken(token, WindowToWindowData(window), display_id,
                             window->current_local_surface_id());
}

bool WindowTree::RemoveFromMaps(const ServerWindow* window) {
  auto iter = window_to_client_id_map_.find(window);
  if (iter == window_to_client_id_map_.end())
    return false;

  client_id_to_window_map_.erase(iter->second);
  window_to_client_id_map_.erase(iter);
  return true;
}

void WindowTree::RemoveFromKnown(const ServerWindow* window,
                                 std::vector<ServerWindow*>* created_windows) {
  // TODO(sky): const_cast here is a bit ick.
  if (created_windows_.count(const_cast<ServerWindow*>(window))) {
    if (created_windows)
      created_windows->push_back(const_cast<ServerWindow*>(window));
    return;
  }

  RemoveFromMaps(window);

  for (ServerWindow* child : window->children())
    RemoveFromKnown(child, created_windows);
}

void WindowTree::RemoveRoot(ServerWindow* window, RemoveRootReason reason) {
  DCHECK(roots_.count(window) > 0);
  roots_.erase(window);

  if (window->owning_tree_id() == id_) {
    // This client created the window. If this client is the window manager and
    // display roots are manually created, then |window| is a display root and
    // needs be cleaned.
    if (window_manager_state_ && !automatically_create_display_roots_) {
      // The window manager is asking to delete the root it created.
      window_manager_state_->DeleteWindowManagerDisplayRoot(window->parent());
      DeleteWindowImpl(this, window);
    }
    return;
  }

  const Id client_window_id = TransportIdForWindow(window);

  if (reason == RemoveRootReason::EMBED) {
    client()->OnUnembed(client_window_id);
    client()->OnWindowDeleted(client_window_id);
    window_server_->OnTreeMessagedClient(id_);
  }

  // This client no longer knows about |window|. Unparent any windows created
  // by this client that were parented to descendants of |window|.
  std::vector<ServerWindow*> created_windows;
  RemoveFromKnown(window, &created_windows);
  for (ServerWindow* created_window : created_windows)
    created_window->parent()->Remove(created_window);

  if (reason == RemoveRootReason::UNEMBED) {
    // Notify the owner of the window it no longer has a client embedded in it.
    // Owner is null in the case of the windowmanager unembedding itself from
    // a root.
    WindowTree* owning_tree =
        window_server_->GetTreeWithId(window->owning_tree_id());
    if (owning_tree) {
      DCHECK(owning_tree && owning_tree != this);
      owning_tree->client()->OnEmbeddedAppDisconnected(
          owning_tree->TransportIdForWindow(window));
    }

    window->OnEmbeddedAppDisconnected();
  }
}

std::vector<mojom::WindowDataPtr> WindowTree::WindowsToWindowDatas(
    const std::vector<const ServerWindow*>& windows) {
  std::vector<mojom::WindowDataPtr> array(windows.size());
  for (size_t i = 0; i < windows.size(); ++i)
    array[i] = WindowToWindowData(windows[i]);
  return array;
}

mojom::WindowDataPtr WindowTree::WindowToWindowData(
    const ServerWindow* window) {
  DCHECK(IsWindowKnown(window));
  const ServerWindow* parent = window->parent();
  const ServerWindow* transient_parent = window->transient_parent();
  // If the parent or transient parent isn't known, it means it is not visible
  // to the client and should not be sent over.
  if (!IsWindowKnown(parent))
    parent = nullptr;
  if (!IsWindowKnown(transient_parent))
    transient_parent = nullptr;
  mojom::WindowDataPtr window_data(mojom::WindowData::New());
  window_data->parent_id =
      parent ? TransportIdForWindow(parent) : kInvalidTransportId;
  window_data->window_id =
      window ? TransportIdForWindow(window) : kInvalidTransportId;
  window_data->transient_parent_id =
      transient_parent ? TransportIdForWindow(transient_parent)
                       : kInvalidTransportId;
  window_data->bounds = window->bounds();
  window_data->properties = mojo::MapToFlatMap(window->properties());
  window_data->visible = window->visible();
  return window_data;
}

void WindowTree::GetWindowTreeImpl(
    const ServerWindow* window,
    std::vector<const ServerWindow*>* windows) const {
  DCHECK(window);

  if (!access_policy_->CanGetWindowTree(window))
    return;

  windows->push_back(window);

  if (!access_policy_->CanDescendIntoWindowForWindowTree(window))
    return;

  const ServerWindow::Windows& children = window->children();
  for (ServerWindow* child : children)
    GetWindowTreeImpl(child, windows);
}

void WindowTree::NotifyDrawnStateChanged(const ServerWindow* window,
                                         bool new_drawn_value) {
  // Even though we don't know about window, it may be an ancestor of our root,
  // in which case the change may effect our roots drawn state.
  if (roots_.empty())
    return;

  for (auto* root : roots_) {
    if (window->Contains(root) && (new_drawn_value != root->IsDrawn())) {
      client()->OnWindowParentDrawnStateChanged(TransportIdForWindow(root),
                                                new_drawn_value);
    }
  }
}

void WindowTree::DestroyWindows() {
  if (created_windows_.empty())
    return;

  Operation op(this, window_server_, OperationType::DELETE_WINDOW);
  // If we get here from the destructor we're not going to get
  // ProcessWindowDeleted(). Copy the map and delete from the copy so that we
  // don't have to worry about whether |created_windows_| changes or not.
  std::set<ServerWindow*> created_windows_copy;
  created_windows_.swap(created_windows_copy);
  // A sibling can be a transient parent of another window so we detach windows
  // from their transient parents to avoid double deletes.
  for (ServerWindow* window : created_windows_copy) {
    ServerWindow* transient_parent = window->transient_parent();
    if (transient_parent)
      transient_parent->RemoveTransientWindow(window);
  }

  for (ServerWindow* window : created_windows_copy)
    delete window;
}

bool WindowTree::CanEmbed(const ClientWindowId& window_id) const {
  const ServerWindow* window = GetWindowByClientId(window_id);
  return window && access_policy_->CanEmbed(window);
}

void WindowTree::PrepareForEmbed(ServerWindow* window) {
  DCHECK(window);

  // Only allow a node to be the root for one client.
  WindowTree* existing_owner = window_server_->GetTreeWithRoot(window);

  Operation op(this, window_server_, OperationType::EMBED);
  RemoveChildrenAsPartOfEmbed(window);
  if (existing_owner) {
    // Never message the originating client.
    window_server_->OnTreeMessagedClient(id_);
    existing_owner->RemoveRoot(window, RemoveRootReason::EMBED);
  }
}

void WindowTree::RemoveChildrenAsPartOfEmbed(ServerWindow* window) {
  CHECK(window);
  while (!window->children().empty())
    window->Remove(window->children().front());
}

uint32_t WindowTree::GenerateEventAckId() {
  DCHECK(!event_ack_id_);
  // We do not want to create a sequential id for each event, because that can
  // leak some information to the client. So instead, manufacture the id
  // randomly.
  event_ack_id_ = 0x1000000 | (rand() & 0xffffff);
  return event_ack_id_;
}

void WindowTree::DispatchEventImpl(ServerWindow* target,
                                   const ui::Event& event,
                                   const EventLocation& event_location,
                                   DispatchEventCallback callback) {
  // DispatchEventImpl() is called so often that log level 4 is used.
  DVLOG(4) << "DispatchEventImpl client=" << id_;
  GenerateEventAckId();
  event_ack_callback_ = std::move(callback);
  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(target);
  DCHECK(display_root);
  event_source_wms_ = display_root->window_manager_state();
  // Should only get events from windows attached to a host.
  DCHECK(event_source_wms_);
  bool matched_pointer_watcher = EventMatchesPointerWatcher(event);

  // Pass the root window of the display supplying the event. This is necessary
  // for Ash to determine the event position in the unified desktop mode, where
  // each physical display mirrors part of a single virtual display.
  Display* display = window_server_->display_manager()->GetDisplayById(
      event_location.display_id);
  WindowManagerDisplayRoot* event_display_root = nullptr;
  if (display && window_manager_state_) {
    event_display_root = display->window_manager_display_root();
  }
  ServerWindow* display_root_window =
      event_display_root ? event_display_root->GetClientVisibleRoot() : nullptr;

  client()->OnWindowInputEvent(
      event_ack_id_, TransportIdForWindow(target), event_location.display_id,
      display_root_window ? TransportIdForWindow(display_root_window) : Id(),
      event_location.raw_location, ui::Event::Clone(event),
      matched_pointer_watcher);
}

bool WindowTree::EventMatchesPointerWatcher(const ui::Event& event) const {
  if (!has_pointer_watcher_)
    return false;
  if (!event.IsPointerEvent())
    return false;
  if (pointer_watcher_want_moves_ && event.type() == ui::ET_POINTER_MOVED)
    return true;
  return event.type() == ui::ET_POINTER_DOWN ||
         event.type() == ui::ET_POINTER_UP ||
         event.type() == ui::ET_POINTER_WHEEL_CHANGED;
}

ClientWindowId WindowTree::MakeClientWindowId(Id transport_window_id) const {
  // If the client didn't specify the id portion of the window_id use the id of
  // the client.
  if (!ClientIdFromTransportId(transport_window_id))
    return ClientWindowId(id_, transport_window_id);
  return ClientWindowId(ClientIdFromTransportId(transport_window_id),
                        ClientWindowIdFromTransportId(transport_window_id));
}

mojom::WindowTreeClientPtr
WindowTree::GetAndRemoveScheduledEmbedWindowTreeClient(
    const base::UnguessableToken& token) {
  auto iter = scheduled_embeds_.find(token);
  if (iter != scheduled_embeds_.end()) {
    mojom::WindowTreeClientPtr client = std::move(iter->second);
    scheduled_embeds_.erase(iter);
    return client;
  }

  // There are no clients above the window manager.
  if (window_manager_internal_)
    return nullptr;

  // Use the root to find the client that embedded this. For non-window manager
  // connections there should be only one root (a WindowTreeClient can only be
  // embedded once). During shutdown there may be no roots.
  if (roots_.size() != 1)
    return nullptr;
  const ServerWindow* root = *roots_.begin();
  WindowTree* owning_tree =
      window_server_->GetTreeWithId(root->owning_tree_id());
  if (!owning_tree)
    return nullptr;
  DCHECK_NE(this, owning_tree);
  return owning_tree->GetAndRemoveScheduledEmbedWindowTreeClient(token);
}

void WindowTree::NewWindow(
    uint32_t change_id,
    Id transport_window_id,
    const base::Optional<base::flat_map<std::string, std::vector<uint8_t>>>&
        transport_properties) {
  std::map<std::string, std::vector<uint8_t>> properties;
  if (transport_properties.has_value())
    properties = mojo::FlatMapToMap(transport_properties.value());

  client()->OnChangeCompleted(
      change_id,
      NewWindow(MakeClientWindowId(transport_window_id), properties));
}

void WindowTree::NewTopLevelWindow(
    uint32_t change_id,
    Id transport_window_id,
    const base::flat_map<std::string, std::vector<uint8_t>>&
        transport_properties) {
  // TODO(sky): rather than DCHECK, have this kill connection.
  DCHECK(!window_manager_internal_);  // Not valid for the windowmanager.
  DCHECK(!waiting_for_top_level_window_info_);
  const ClientWindowId client_window_id =
      MakeClientWindowId(transport_window_id);
  // TODO(sky): need a way for client to provide context to figure out display.
  Display* display = display_manager()->displays().empty()
                         ? nullptr
                         : *(display_manager()->displays().begin());
  // TODO(sky): move checks to accesspolicy.
  WindowManagerDisplayRoot* display_root =
      display && !is_for_embedding_ ? display->window_manager_display_root()
                                    : nullptr;
  if (!display_root ||
      display_root->window_manager_state()->window_tree() == this ||
      !IsValidIdForNewWindow(client_window_id)) {
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  // The server creates the real window. Any further messages from the client
  // may try to alter the window. Pause incoming messages so that we know we
  // can't get a message for a window before the window is created. Once the
  // window is created we'll resume processing.
  binding_->SetIncomingMethodCallProcessingPaused(true);

  const uint32_t wm_change_id =
      window_server_->GenerateWindowManagerChangeId(this, change_id);

  waiting_for_top_level_window_info_.reset(
      new WaitingForTopLevelWindowInfo(client_window_id, wm_change_id));

  display_root->window_manager_state()
      ->window_tree()
      ->window_manager_internal_->WmCreateTopLevelWindow(
          wm_change_id, client_window_id, transport_properties);
}

void WindowTree::DeleteWindow(uint32_t change_id, Id transport_window_id) {
  client()->OnChangeCompleted(
      change_id, DeleteWindow(MakeClientWindowId(transport_window_id)));
}

void WindowTree::AddWindow(uint32_t change_id, Id parent_id, Id child_id) {
  client()->OnChangeCompleted(
      change_id,
      AddWindow(MakeClientWindowId(parent_id), MakeClientWindowId(child_id)));
}

void WindowTree::RemoveWindowFromParent(uint32_t change_id, Id window_id) {
  client()->OnChangeCompleted(
      change_id, RemoveWindowFromParent(MakeClientWindowId(window_id)));
}

void WindowTree::AddTransientWindow(uint32_t change_id,
                                    Id window,
                                    Id transient_window) {
  client()->OnChangeCompleted(
      change_id, AddTransientWindow(MakeClientWindowId(window),
                                    MakeClientWindowId(transient_window)));
}

void WindowTree::RemoveTransientWindowFromParent(uint32_t change_id,
                                                 Id transient_window_id) {
  bool success = false;
  ServerWindow* transient_window =
      GetWindowByClientId(MakeClientWindowId(transient_window_id));
  if (transient_window && transient_window->transient_parent() &&
      access_policy_->CanRemoveTransientWindowFromParent(transient_window)) {
    success = true;
    Operation op(this, window_server_,
                 OperationType::REMOVE_TRANSIENT_WINDOW_FROM_PARENT);
    transient_window->transient_parent()->RemoveTransientWindow(
        transient_window);
  }
  client()->OnChangeCompleted(change_id, success);
}

void WindowTree::SetModalType(uint32_t change_id,
                              Id window_id,
                              ModalType modal_type) {
  client()->OnChangeCompleted(
      change_id, SetModalType(MakeClientWindowId(window_id), modal_type));
}

void WindowTree::SetChildModalParent(uint32_t change_id,
                                     Id window_id,
                                     Id parent_window_id) {
  client()->OnChangeCompleted(
      change_id, SetChildModalParent(MakeClientWindowId(window_id),
                                     MakeClientWindowId(parent_window_id)));
}

void WindowTree::ReorderWindow(uint32_t change_id,
                               Id window_id,
                               Id relative_window_id,
                               mojom::OrderDirection direction) {
  // TODO(erg): This implementation allows reordering two windows that are
  // children of a parent window which the two implementations can't see. There
  // should be a security check to prevent this.
  bool success = false;
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  ServerWindow* relative_window =
      GetWindowByClientId(MakeClientWindowId(relative_window_id));
  DVLOG(3) << "reorder client=" << id_ << " client window_id=" << window_id
           << " global window_id=" << DebugWindowId(window)
           << " relative client window_id=" << relative_window_id
           << " relative global window_id=" << DebugWindowId(relative_window);
  if (CanReorderWindow(window, relative_window, direction)) {
    success = true;
    Operation op(this, window_server_, OperationType::REORDER_WINDOW);
    window->Reorder(relative_window, direction);
    window_server_->ProcessWindowReorder(window, relative_window, direction);
  }
  client()->OnChangeCompleted(change_id, success);
}

void WindowTree::GetWindowTree(Id window_id, GetWindowTreeCallback callback) {
  std::vector<const ServerWindow*> windows(
      GetWindowTree(MakeClientWindowId(window_id)));
  std::move(callback).Run(WindowsToWindowDatas(windows));
}

void WindowTree::SetCapture(uint32_t change_id, Id window_id) {
  client()->OnChangeCompleted(change_id,
                              SetCapture(MakeClientWindowId(window_id)));
}

void WindowTree::ReleaseCapture(uint32_t change_id, Id window_id) {
  client()->OnChangeCompleted(change_id,
                              ReleaseCapture(MakeClientWindowId(window_id)));
}

void WindowTree::StartPointerWatcher(bool want_moves) {
  has_pointer_watcher_ = true;
  pointer_watcher_want_moves_ = want_moves;
}

void WindowTree::StopPointerWatcher() {
  has_pointer_watcher_ = false;
  pointer_watcher_want_moves_ = false;
}

void WindowTree::SetWindowBounds(
    uint32_t change_id,
    Id window_id,
    const gfx::Rect& bounds,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  if (window && ShouldRouteToWindowManager(window)) {
    DVLOG(3) << "Redirecting request to change bounds for "
             << DebugWindowId(window) << " to window manager...";
    const uint32_t wm_change_id =
        window_server_->GenerateWindowManagerChangeId(this, change_id);
    // |window_id| may be a client id, use the id from the window to ensure
    // the windowmanager doesn't get an id it doesn't know about.
    WindowManagerDisplayRoot* display_root =
        GetWindowManagerDisplayRoot(window);
    WindowTree* wm_tree = display_root->window_manager_state()->window_tree();
    wm_tree->window_manager_internal_->WmSetBounds(
        wm_change_id, wm_tree->TransportIdForWindow(window), std::move(bounds));
    return;
  }

  DVLOG(3) << "SetWindowBounds window_id=" << window_id
           << " global window_id=" << DebugWindowId(window)
           << " bounds=" << bounds.ToString() << " local_surface_id="
           << (local_surface_id ? local_surface_id->ToString() : "null");

  if (!window) {
    DVLOG(1) << "SetWindowBounds failed (invalid window id)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  // Only the owner of the window can change the bounds.
  bool success = access_policy_->CanSetWindowBounds(window);
  if (success) {
    Operation op(this, window_server_, OperationType::SET_WINDOW_BOUNDS);
    window->SetBounds(bounds, local_surface_id);
  } else {
    DVLOG(1) << "SetWindowBounds failed (access denied)";
  }
  client()->OnChangeCompleted(change_id, success);
}

void WindowTree::SetWindowTransform(uint32_t change_id,
                                    Id window_id,
                                    const gfx::Transform& transform) {
  // Clients shouldn't have a need to set the transform of the embed root, so
  // we don't bother routing it to the window-manager.

  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  DVLOG(3) << "SetWindowTransform client window_id=" << window_id
           << " global window_id=" << DebugWindowId(window)
           << " transform=" << transform.ToString();

  if (!window) {
    DVLOG(1) << "SetWindowTransform failed (invalid window id)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  // Only the owner of the window can change the bounds.
  const bool success = access_policy_->CanSetWindowTransform(window);
  if (success) {
    Operation op(this, window_server_, OperationType::SET_WINDOW_TRANSFORM);
    window->SetTransform(transform);
  } else {
    DVLOG(1) << "SetWindowTransform failed (access denied)";
  }
  client()->OnChangeCompleted(change_id, success);
}

void WindowTree::SetWindowVisibility(uint32_t change_id,
                                     Id transport_window_id,
                                     bool visible) {
  client()->OnChangeCompleted(
      change_id,
      SetWindowVisibility(MakeClientWindowId(transport_window_id), visible));
}

void WindowTree::SetWindowProperty(
    uint32_t change_id,
    Id transport_window_id,
    const std::string& name,
    const base::Optional<std::vector<uint8_t>>& value) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  if (window && ShouldRouteToWindowManager(window)) {
    const uint32_t wm_change_id =
        window_server_->GenerateWindowManagerChangeId(this, change_id);
    WindowManagerDisplayRoot* display_root =
        GetWindowManagerDisplayRoot(window);
    WindowTree* wm_tree = display_root->window_manager_state()->window_tree();
    wm_tree->window_manager_internal_->WmSetProperty(
        wm_change_id, wm_tree->TransportIdForWindow(window), name, value);
    return;
  }
  const bool success = window && access_policy_->CanSetWindowProperties(window);
  if (success) {
    Operation op(this, window_server_, OperationType::SET_WINDOW_PROPERTY);
    if (!value.has_value()) {
      window->SetProperty(name, nullptr);
    } else {
      window->SetProperty(name, &value.value());
    }
  }
  client()->OnChangeCompleted(change_id, success);
}

void WindowTree::SetWindowOpacity(uint32_t change_id,
                                  Id window_id,
                                  float opacity) {
  client()->OnChangeCompleted(
      change_id, SetWindowOpacity(MakeClientWindowId(window_id), opacity));
}

void WindowTree::AttachCompositorFrameSink(
    Id transport_window_id,
    viz::mojom::CompositorFrameSinkRequest compositor_frame_sink,
    viz::mojom::CompositorFrameSinkClientPtr client) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  if (!window) {
    DVLOG(1) << "AttachCompositorFrameSink failed (invalid window id)";
    return;
  }

  const bool success = access_policy_->CanSetWindowCompositorFrameSink(window);
  if (!success) {
    DVLOG(1) << "AttachCompositorFrameSink failed (access denied)";
    return;
  }
  window->CreateCompositorFrameSink(std::move(compositor_frame_sink),
                                    std::move(client));
}

void WindowTree::SetWindowTextInputState(Id transport_window_id,
                                         ui::mojom::TextInputStatePtr state) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  bool success = window && access_policy_->CanSetWindowTextInputState(window);
  if (success)
    window->SetTextInputState(state.To<ui::TextInputState>());
}

void WindowTree::SetImeVisibility(Id transport_window_id,
                                  bool visible,
                                  ui::mojom::TextInputStatePtr state) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  bool success = window && access_policy_->CanSetWindowTextInputState(window);
  if (success) {
    if (!state.is_null())
      window->SetTextInputState(state.To<ui::TextInputState>());

    Display* display = GetDisplay(window);
    if (display)
      display->SetImeVisibility(window, visible);
  }
}

void WindowTree::OnWindowInputEventAck(uint32_t event_id,
                                       mojom::EventResult result) {
  // DispatchEventImpl() is called so often that log level 4 is used.
  DVLOG(4) << "OnWindowInputEventAck client=" << id_;
  if (event_ack_id_ == 0 || event_id != event_ack_id_ || !event_ack_callback_) {
    // TODO(sad): Something bad happened. Kill the client?
    NOTIMPLEMENTED() << ": Wrong event acked. event_id=" << event_id
                     << ", event_ack_id_=" << event_ack_id_;
    DVLOG(1) << "OnWindowInputEventAck supplied unexpected event_id";
    return;
  }

  event_ack_id_ = 0;

  if (janky_)
    event_source_wms_->window_tree()->ClientJankinessChanged(this);

  event_source_wms_ = nullptr;
  base::ResetAndReturn(&event_ack_callback_).Run(result);

  if (!event_queue_.empty()) {
    DCHECK(!event_ack_id_);
    ServerWindow* target = nullptr;
    std::unique_ptr<ui::Event> event;
    DispatchEventCallback callback;
    EventLocation event_location;
    do {
      std::unique_ptr<TargetedEvent> targeted_event =
          std::move(event_queue_.front());
      event_queue_.pop();
      target = targeted_event->target();
      event = targeted_event->TakeEvent();
      event_location = targeted_event->event_location();
      callback = targeted_event->TakeCallback();
    } while (!event_queue_.empty() && !GetDisplay(target));
    if (GetDisplay(target)) {
      DispatchEventImpl(target, *event, event_location, std::move(callback));
    } else {
      // If the window is no longer valid (or not in a display), then there is
      // no point in dispatching to the client, but we need to run the callback
      // so that WindowManagerState isn't still waiting for an ack. We only
      // need run the last callback as they should all target the same
      // WindowManagerState and WindowManagerState is at most waiting on one
      // ack.
      std::move(callback).Run(mojom::EventResult::UNHANDLED);
    }
  }
}

void WindowTree::SetClientArea(Id transport_window_id,
                               const gfx::Insets& insets,
                               const base::Optional<std::vector<gfx::Rect>>&
                                   transport_additional_client_areas) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  DVLOG(3) << "SetClientArea client window_id=" << transport_window_id
           << " global window_id=" << DebugWindowId(window)
           << " insets=" << insets.top() << " " << insets.left() << " "
           << insets.bottom() << " " << insets.right();
  if (!window) {
    DVLOG(1) << "SetClientArea failed (invalid window id)";
    return;
  }
  if (!access_policy_->CanSetClientArea(window)) {
    DVLOG(1) << "SetClientArea failed (access denied)";
    return;
  }

  Operation op(this, window_server_, OperationType::SET_CLIENT_AREA);
  window->SetClientArea(insets, transport_additional_client_areas.value_or(
                                    std::vector<gfx::Rect>()));
}

void WindowTree::SetHitTestMask(Id transport_window_id,
                                const base::Optional<gfx::Rect>& mask) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  if (!window) {
    DVLOG(1) << "SetHitTestMask failed (invalid window id)";
    return;
  }

  if (!access_policy_->CanSetHitTestMask(window)) {
    DVLOG(1) << "SetHitTestMask failed (access denied)";
    return;
  }

  if (mask)
    window->SetHitTestMask(*mask);
  else
    window->ClearHitTestMask();
}

void WindowTree::SetCanAcceptDrops(Id window_id, bool accepts_drops) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  if (!window) {
    DVLOG(1) << "SetCanAcceptDrops failed (invalid window id)";
    return;
  }

  if (!access_policy_->CanSetAcceptDrops(window)) {
    DVLOG(1) << "SetAcceptsDrops failed (access denied)";
    return;
  }

  window->SetCanAcceptDrops(accepts_drops);
}

void WindowTree::Embed(Id transport_window_id,
                       mojom::WindowTreeClientPtr client,
                       uint32_t flags,
                       EmbedCallback callback) {
  std::move(callback).Run(
      Embed(MakeClientWindowId(transport_window_id), std::move(client), flags));
}

void WindowTree::ScheduleEmbed(mojom::WindowTreeClientPtr client,
                               ScheduleEmbedCallback callback) {
  const base::UnguessableToken token = base::UnguessableToken::Create();
  scheduled_embeds_[token] = std::move(client);
  std::move(callback).Run(token);
}

void WindowTree::EmbedUsingToken(Id transport_window_id,
                                 const base::UnguessableToken& token,
                                 uint32_t flags,
                                 EmbedUsingTokenCallback callback) {
  mojom::WindowTreeClientPtr client =
      GetAndRemoveScheduledEmbedWindowTreeClient(token);
  if (client) {
    Embed(transport_window_id, std::move(client), flags, std::move(callback));
    return;
  }

  WindowTreeAndWindowId tree_and_id =
      window_server()->UnregisterEmbedToken(token);
  if (!tree_and_id.tree) {
    DVLOG(1) << "EmbedUsingToken failed, no ScheduleEmbed(), token="
             << token.ToString();
    std::move(callback).Run(false);
    return;
  }
  if (tree_and_id.tree == this) {
    DVLOG(1) << "EmbedUsingToken failed, attempt to embed self, token="
             << token.ToString();
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(EmbedExistingTree(
      MakeClientWindowId(transport_window_id), tree_and_id, token));
}

void WindowTree::ScheduleEmbedForExistingClient(
    uint32_t window_id,
    ScheduleEmbedForExistingClientCallback callback) {
  std::move(callback).Run(window_server()->RegisterEmbedToken(this, window_id));
}

void WindowTree::SetFocus(uint32_t change_id, Id transport_window_id) {
  client()->OnChangeCompleted(
      change_id, SetFocus(MakeClientWindowId(transport_window_id)));
}

void WindowTree::SetCanFocus(Id transport_window_id, bool can_focus) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  if (!window) {
    DVLOG(1) << "SetCanFocus failed (invalid window id)";
    return;
  }

  if (ShouldRouteToWindowManager(window)) {
    WindowManagerDisplayRoot* display_root =
        GetWindowManagerDisplayRoot(window);
    WindowTree* wm_tree = display_root->window_manager_state()->window_tree();
    wm_tree->window_manager_internal_->WmSetCanFocus(transport_window_id,
                                                     can_focus);
  } else if (access_policy_->CanSetFocus(window)) {
    window->set_can_focus(can_focus);
  } else {
    DVLOG(1) << "SetCanFocus failed (access denied)";
  }
}

void WindowTree::SetEventTargetingPolicy(Id transport_window_id,
                                         mojom::EventTargetingPolicy policy) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  // TODO(riajiang): check |event_queue_| is empty for |window|.
  if (window && access_policy_->CanSetEventTargetingPolicy(window))
    window->set_event_targeting_policy(policy);
}

void WindowTree::SetCursor(uint32_t change_id,
                           Id transport_window_id,
                           ui::CursorData cursor) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  if (!window) {
    DVLOG(1) << "SetCursor failed (invalid id)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  // Only the owner of the window can change the bounds.
  bool success = access_policy_->CanSetCursorProperties(window);
  if (!success) {
    DVLOG(1) << "SetCursor failed (access denied)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  // If the cursor is custom, it must have valid frames.
  if (cursor.cursor_type() == ui::CursorType::kCustom) {
    if (cursor.cursor_frames().empty()) {
      DVLOG(1) << "SetCursor failed (no frames with custom cursor)";
      client()->OnChangeCompleted(change_id, false);
      return;
    }

    for (const SkBitmap& bitmap : cursor.cursor_frames()) {
      if (bitmap.drawsNothing()) {
        DVLOG(1) << "SetCursor failed (cursor frame draws nothing)";
        client()->OnChangeCompleted(change_id, false);
        return;
      }
    }
  }

  Operation op(this, window_server_,
               OperationType::SET_WINDOW_PREDEFINED_CURSOR);
  window->SetCursor(std::move(cursor));
  client()->OnChangeCompleted(change_id, success);
}

void WindowTree::DeactivateWindow(Id window_id) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  if (!window) {
    DVLOG(1) << "DeactivateWindow failed (invalid id)";
    return;
  }

  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root) {
    // The window isn't parented. There's nothing to do.
    DVLOG(1) << "DeactivateWindow failed (window unparented)";
    return;
  }

  WindowTree* wm_tree = display_root->window_manager_state()->window_tree();
  wm_tree->window_manager_internal_->WmDeactivateWindow(
      wm_tree->TransportIdForWindow(window));
}

void WindowTree::StackAbove(uint32_t change_id, Id above_id, Id below_id) {
  ServerWindow* above = GetWindowByClientId(MakeClientWindowId(above_id));
  if (!above) {
    DVLOG(1) << "StackAbove failed (invalid above id)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  ServerWindow* below = GetWindowByClientId(MakeClientWindowId(below_id));
  if (!below) {
    DVLOG(1) << "StackAbove failed (invalid below id)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  if (!access_policy_->CanStackAbove(above, below)) {
    DVLOG(1) << "StackAbove failed (access denied)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  ServerWindow* parent = above->parent();
  ServerWindow* below_parent = below->parent();
  if (!parent) {
    DVLOG(1) << "StackAbove failed (above unparented)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }
  if (!below_parent) {
    DVLOG(1) << "StackAbove failed (below unparented)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }
  if (parent != below_parent) {
    DVLOG(1) << "StackAbove failed (windows have different parents)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(above);
  if (!display_root) {
    DVLOG(1) << "StackAbove (no display root)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  // Window reordering assumes that it is the owner of parent who is sending
  // the message, and does not deal gracefully with other clients reordering
  // their windows. So tell the window manager to send us a reorder message.
  WindowTree* wm_tree = display_root->window_manager_state()->window_tree();
  const uint32_t wm_change_id =
      window_server_->GenerateWindowManagerChangeId(this, change_id);
  wm_tree->window_manager_internal_->WmStackAbove(
      wm_change_id, wm_tree->TransportIdForWindow(above),
      wm_tree->TransportIdForWindow(below));
}

void WindowTree::StackAtTop(uint32_t change_id, Id window_id) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  if (!window) {
    DVLOG(1) << "StackAtTop failed (invalid id)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  if (!access_policy_->CanStackAtTop(window)) {
    DVLOG(1) << "StackAtTop failed (access denied)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  ServerWindow* parent = window->parent();
  if (!parent) {
    DVLOG(1) << "StackAtTop failed (window unparented)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  DCHECK(!parent->children().empty());
  if (parent->children().back() == window) {
    // Ignore this call; the client didn't know they were already at the top.
    DVLOG(3) << "StackAtTop ignored (already at top)";
    client()->OnChangeCompleted(change_id, true);
    return;
  }

  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root) {
    DVLOG(1) << "StackAtTop (no display root)";
    client()->OnChangeCompleted(change_id, false);
    return;
  }

  // Window reordering assumes that it is the owner of parent who is sending
  // the message, and does not deal gracefully with other clients reordering
  // their windows. So tell the window manager to send us a reorder message.
  WindowTree* wm_tree = display_root->window_manager_state()->window_tree();
  const uint32_t wm_change_id =
      window_server_->GenerateWindowManagerChangeId(this, change_id);
  wm_tree->window_manager_internal_->WmStackAtTop(
      wm_change_id, wm_tree->TransportIdForWindow(window));
}

void WindowTree::PerformWmAction(Id window_id, const std::string& action) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  if (!window) {
    DVLOG(1) << "PerformWmAction(" << action << ") failed (invalid id)";
    return;
  }

  if (!access_policy_->CanPerformWmAction(window)) {
    DVLOG(1) << "PerformWmAction(" << action << ") failed (access denied)";
    return;
  }

  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root) {
    DVLOG(1) << "PerformWmAction(" << action << ") failed (no display root)";
    return;
  }

  WindowTree* wm_tree = display_root->window_manager_state()->window_tree();
  wm_tree->window_manager_internal_->WmPerformWmAction(
      wm_tree->TransportIdForWindow(window), action);
}

void WindowTree::GetWindowManagerClient(
    mojo::AssociatedInterfaceRequest<mojom::WindowManagerClient> internal) {
  if (!access_policy_->CanSetWindowManager() || !window_manager_internal_ ||
      window_manager_internal_client_binding_) {
    return;
  }
  window_manager_internal_client_binding_.reset(
      new mojo::AssociatedBinding<mojom::WindowManagerClient>(
          this, std::move(internal)));
}

void WindowTree::GetCursorLocationMemory(
    GetCursorLocationMemoryCallback callback) {
  std::move(callback).Run(
      display_manager()->cursor_location_manager()->GetCursorLocationMemory());
}

void WindowTree::PerformDragDrop(
    uint32_t change_id,
    Id source_window_id,
    const gfx::Point& screen_location,
    const base::flat_map<std::string, std::vector<uint8_t>>& drag_data,
    const SkBitmap& drag_image,
    const gfx::Vector2d& drag_image_offset,
    uint32_t drag_operation,
    ui::mojom::PointerKind source) {
  // TODO(erg): SkBitmap is the wrong data type for the drag image; we should
  // be passing ImageSkias once http://crbug.com/655874 is implemented.

  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(source_window_id));
  bool success = window && access_policy_->CanInitiateDragLoop(window);
  if (!success || !ShouldRouteToWindowManager(window)) {
    // We need to fail this move loop change, otherwise the client will just be
    // waiting for |change_id|.
    DVLOG(1) << "PerformDragDrop failed (access denied)";
    client()->OnPerformDragDropCompleted(change_id, false,
                                         mojom::kDropEffectNone);
    return;
  }

  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root) {
    // The window isn't parented. There's nothing to do.
    DVLOG(1) << "PerformDragDrop failed (window unparented)";
    client()->OnPerformDragDropCompleted(change_id, false,
                                         mojom::kDropEffectNone);
    return;
  }

  if (window_server_->in_move_loop() || window_server_->in_drag_loop()) {
    // Either the window manager is servicing a window drag or we're servicing
    // a drag and drop operation. We can't start a second drag.
    DVLOG(1) << "PerformDragDrop failed (already performing a drag)";
    client()->OnPerformDragDropCompleted(change_id, false,
                                         mojom::kDropEffectNone);
    return;
  }

  WindowManagerState* wms = display_root->window_manager_state();

  // Send the drag representation to the window manager.
  wms->window_tree()->window_manager_internal_->WmBuildDragImage(
      screen_location, drag_image, drag_image_offset, source);

  // Here, we need to dramatically change how the mouse pointer works. Once
  // we've started a drag drop operation, cursor events don't go to windows as
  // normal.
  window_server_->StartDragLoop(change_id, window, this);
  wms->SetDragDropSourceWindow(this, window, this, drag_data, drag_operation);
}

void WindowTree::CancelDragDrop(Id window_id) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  if (!window) {
    DVLOG(1) << "CancelDragDrop failed (no window)";
    return;
  }

  if (window != window_server_->GetCurrentDragLoopWindow()) {
    DVLOG(1) << "CancelDragDrop failed (not the drag loop window)";
    return;
  }

  if (window_server_->GetCurrentDragLoopInitiator() != this) {
    DVLOG(1) << "CancelDragDrop failed (not the drag initiator)";
    return;
  }

  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root) {
    DVLOG(1) << "CancelDragDrop failed (no such window manager display root)";
    return;
  }

  WindowManagerState* wms = display_root->window_manager_state();
  wms->CancelDragDrop();
}

void WindowTree::PerformWindowMove(uint32_t change_id,
                                   Id window_id,
                                   ui::mojom::MoveLoopSource source,
                                   const gfx::Point& cursor) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  bool success = window && access_policy_->CanInitiateMoveLoop(window);
  if (!success || !ShouldRouteToWindowManager(window)) {
    // We need to fail this move loop change, otherwise the client will just be
    // waiting for |change_id|.
    DVLOG(1) << "PerformWindowMove failed (access denied)";
    OnChangeCompleted(change_id, false);
    return;
  }

  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root) {
    // The window isn't parented. There's nothing to do.
    DVLOG(1) << "PerformWindowMove failed (window unparented)";
    OnChangeCompleted(change_id, false);
    return;
  }

  if (window_server_->in_move_loop() || window_server_->in_drag_loop()) {
    // Either the window manager is servicing a window drag or we're servicing
    // a drag and drop operation. We can't start a second drag.
    DVLOG(1) << "PerformWindowMove failed (already performing a drag)";
    OnChangeCompleted(change_id, false);
    return;
  }

  // When we perform a window move loop, we give the window manager non client
  // capture. Because of how the capture public interface currently works,
  // SetCapture() will check whether the mouse cursor is currently in the
  // non-client area and if so, will redirect messages to the window
  // manager. (And normal window movement relies on this behaviour.)
  WindowManagerState* wms = display_root->window_manager_state();
  wms->SetCapture(window, wms->window_tree()->id());

  const uint32_t wm_change_id =
      window_server_->GenerateWindowManagerChangeId(this, change_id);
  window_server_->StartMoveLoop(wm_change_id, window, this, window->bounds());
  wms->window_tree()->window_manager_internal_->WmPerformMoveLoop(
      wm_change_id, wms->window_tree()->TransportIdForWindow(window), source,
      cursor);
}

void WindowTree::CancelWindowMove(Id window_id) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  if (!window) {
    DVLOG(1) << "CancelWindowMove failed (invalid window id)";
    return;
  }

  bool success = access_policy_->CanInitiateMoveLoop(window);
  if (!success) {
    DVLOG(1) << "CancelWindowMove failed (access denied)";
    return;
  }

  if (window != window_server_->GetCurrentMoveLoopWindow()) {
    DVLOG(1) << "CancelWindowMove failed (not the move loop window)";
    return;
  }

  if (window_server_->GetCurrentMoveLoopInitiator() != this) {
    DVLOG(1) << "CancelWindowMove failed (not the move loop initiator)";
    return;
  }

  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root) {
    DVLOG(1) << "CancelWindowMove failed (no such window manager display root)";
    return;
  }

  WindowManagerState* wms = display_root->window_manager_state();
  wms->window_tree()->window_manager_internal_->WmCancelMoveLoop(
      window_server_->GetCurrentMoveLoopChangeId());
}

void WindowTree::AddAccelerators(
    std::vector<mojom::WmAcceleratorPtr> accelerators,
    AddAcceleratorsCallback callback) {
  DCHECK(window_manager_state_);

  bool success = true;
  for (auto iter = accelerators.begin(); iter != accelerators.end(); ++iter) {
    if (!window_manager_state_->event_processor()->AddAccelerator(
            iter->get()->id, std::move(iter->get()->event_matcher)))
      success = false;
  }
  std::move(callback).Run(success);
}

void WindowTree::RemoveAccelerator(uint32_t id) {
  window_manager_state_->event_processor()->RemoveAccelerator(id);
}

void WindowTree::AddActivationParent(Id transport_window_id) {
  AddActivationParent(MakeClientWindowId(transport_window_id));
}

void WindowTree::RemoveActivationParent(Id transport_window_id) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  if (!window) {
    DVLOG(1) << "RemoveActivationParent failed (invalid window id)";
    return;
  }
  window->set_is_activation_parent(false);
}

void WindowTree::SetExtendedHitRegionForChildren(
    Id window_id,
    const gfx::Insets& mouse_insets,
    const gfx::Insets& touch_insets) {
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  // Extended hit test region should only be set by the owner of the window.
  if (!window) {
    DVLOG(1) << "SetExtendedHitRegionForChildren failed (invalid window id)";
    return;
  }
  if (window->owning_tree_id() != id_) {
    DVLOG(1) << "SetExtendedHitRegionForChildren failed (supplied window that "
             << "client does not own)";
    return;
  }
  if (HasPositiveInset(mouse_insets) || HasPositiveInset(touch_insets)) {
    DVLOG(1) << "SetExtendedHitRegionForChildren failed (insets must be "
             << "negative)";
    return;
  }
  window->set_extended_hit_test_regions_for_children(mouse_insets,
                                                     touch_insets);
}

void WindowTree::SetKeyEventsThatDontHideCursor(
    std::vector<::ui::mojom::EventMatcherPtr> dont_hide_cursor_list) {
  DCHECK(window_manager_state_);
  window_manager_state_->SetKeyEventsThatDontHideCursor(
      std::move(dont_hide_cursor_list));
}

void WindowTree::SetDisplayRoot(const display::Display& display,
                                mojom::WmViewportMetricsPtr viewport_metrics,
                                bool is_primary_display,
                                Id window_id,
                                const std::vector<display::Display>& mirrors,
                                SetDisplayRootCallback callback) {
  ServerWindow* display_root = ProcessSetDisplayRoot(
      display, TransportMetricsToDisplayMetrics(*viewport_metrics),
      is_primary_display, MakeClientWindowId(window_id), mirrors);
  if (!display_root) {
    std::move(callback).Run(false);
    return;
  }
  display_root->parent()->SetVisible(true);
  std::move(callback).Run(true);
}

void WindowTree::SetDisplayConfiguration(
    const std::vector<display::Display>& displays,
    std::vector<ui::mojom::WmViewportMetricsPtr> transport_metrics,
    int64_t primary_display_id,
    int64_t internal_display_id,
    const std::vector<display::Display>& mirrors,
    SetDisplayConfigurationCallback callback) {
  std::vector<display::ViewportMetrics> metrics;
  for (auto& transport_ptr : transport_metrics)
    metrics.push_back(TransportMetricsToDisplayMetrics(*transport_ptr));
  std::move(callback).Run(display_manager()->SetDisplayConfiguration(
      displays, metrics, primary_display_id, internal_display_id, mirrors));
}

void WindowTree::SwapDisplayRoots(int64_t display_id1,
                                  int64_t display_id2,
                                  SwapDisplayRootsCallback callback) {
  DCHECK(window_manager_state_);  // Only applicable to the window manager.
  std::move(callback).Run(ProcessSwapDisplayRoots(display_id1, display_id2));
}

void WindowTree::SetBlockingContainers(
    std::vector<::ui::mojom::BlockingContainersPtr> blocking_containers,
    SetBlockingContainersCallback callback) {
  DCHECK(window_manager_state_);  // Only applicable to the window manager.
  std::move(callback).Run(
      ProcessSetBlockingContainers(std::move(blocking_containers)));
}

void WindowTree::WmResponse(uint32_t change_id, bool response) {
  if (window_server_->in_move_loop() &&
      window_server_->GetCurrentMoveLoopChangeId() == change_id) {
    ServerWindow* window = window_server_->GetCurrentMoveLoopWindow();

    if (window->owning_tree_id() != id_) {
      window_server_->WindowManagerSentBogusMessage();
      window = nullptr;
    } else {
      WindowManagerDisplayRoot* display_root =
          GetWindowManagerDisplayRoot(window);
      if (display_root) {
        WindowManagerState* wms = display_root->window_manager_state();
        // Clear the implicit capture.
        wms->SetCapture(nullptr, false);
      }
    }

    if (!response && window) {
      // Our move loop didn't succeed, which means that we must restore the
      // original bounds of the window.
      window->SetBounds(window_server_->GetCurrentMoveLoopRevertBounds(),
                        base::nullopt);
    }

    window_server_->EndMoveLoop();
  }

  window_server_->WindowManagerChangeCompleted(change_id, response);
}

void WindowTree::WmSetBoundsResponse(uint32_t change_id) {
  // The window manager will always give a response of false to the client
  // because it will always update the bounds of the top level window itself
  // which will overwrite the request made by the client.
  WmResponse(change_id, false);
}

void WindowTree::WmRequestClose(Id transport_window_id) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  WindowTree* tree = window_server_->GetTreeWithRoot(window);
  if (tree && tree != this) {
    tree->client()->RequestClose(tree->TransportIdForWindow(window));
  }
  // TODO(sky): think about what else case means.
}

void WindowTree::WmSetFrameDecorationValues(
    mojom::FrameDecorationValuesPtr values) {
  DCHECK(window_manager_state_);
  window_manager_state_->SetFrameDecorationValues(std::move(values));
}

void WindowTree::WmSetNonClientCursor(Id window_id, ui::CursorData cursor) {
  DCHECK(window_manager_state_);
  ServerWindow* window = GetWindowByClientId(MakeClientWindowId(window_id));
  if (!window) {
    DVLOG(1) << "WmSetNonClientCursor failed (invalid window id)";
    return;
  }

  window->SetNonClientCursor(std::move(cursor));
}

void WindowTree::WmLockCursor() {
  DCHECK(window_manager_state_);
  window_manager_state_->cursor_state().LockCursor();
}

void WindowTree::WmUnlockCursor() {
  DCHECK(window_manager_state_);
  window_manager_state_->cursor_state().UnlockCursor();
}

void WindowTree::WmSetCursorVisible(bool visible) {
  DCHECK(window_manager_state_);
  window_manager_state_->cursor_state().SetCursorVisible(visible);
}

void WindowTree::WmSetCursorSize(ui::CursorSize cursor_size) {
  DCHECK(window_manager_state_);
  window_manager_state_->cursor_state().SetCursorSize(cursor_size);
}

void WindowTree::WmSetGlobalOverrideCursor(
    base::Optional<ui::CursorData> cursor) {
  DCHECK(window_manager_state_);
  window_manager_state_->cursor_state().SetGlobalOverrideCursor(cursor);
}

void WindowTree::WmMoveCursorToDisplayLocation(const gfx::Point& display_pixels,
                                               int64_t display_id) {
  DCHECK(window_manager_state_);
  window_manager_state_->SetCursorLocation(display_pixels, display_id);
}

void WindowTree::WmConfineCursorToBounds(const gfx::Rect& bounds_in_pixels,
                                         int64_t display_id) {
  DCHECK(window_manager_state_);
  Display* display = display_manager()->GetDisplayById(display_id);
  if (!display) {
    DVLOG(1) << "WmConfineCursorToBounds failed (invalid display id)";
    return;
  }

  PlatformDisplay* platform_display = display->platform_display();
  if (!platform_display) {
    DVLOG(1) << "WmConfineCursorToBounds failed (no platform display)";
    return;
  }

  platform_display->ConfineCursorToBounds(bounds_in_pixels);
}

void WindowTree::WmSetCursorTouchVisible(bool enabled) {
  DCHECK(window_manager_state_);
  window_manager_state_->SetCursorTouchVisible(enabled);
}

void WindowTree::OnWmCreatedTopLevelWindow(uint32_t change_id,
                                           Id transport_window_id) {
  ServerWindow* window =
      GetWindowByClientId(MakeClientWindowId(transport_window_id));
  if (window && window->owning_tree_id() != id_) {
    DVLOG(1) << "OnWmCreatedTopLevelWindow failed (invalid window id)";
    window_server_->WindowManagerSentBogusMessage();
    window = nullptr;
  }
  window_server_->WindowManagerCreatedTopLevelWindow(this, change_id, window);
}

void WindowTree::OnAcceleratorAck(uint32_t event_id,
                                  mojom::EventResult result,
                                  const EventProperties& properties) {
  DVLOG(3) << "OnAcceleratorAck client=" << id_;
  if (event_ack_id_ == 0 || event_id != event_ack_id_ ||
      !accelerator_ack_callback_) {
    DVLOG(1) << "OnAcceleratorAck failed (invalid event id)";
    window_server_->WindowManagerSentBogusMessage();
    return;
  }
  event_ack_id_ = 0;
  DCHECK(window_manager_state_);
  base::ResetAndReturn(&accelerator_ack_callback_).Run(result, properties);
}

bool WindowTree::HasRootForAccessPolicy(const ServerWindow* window) const {
  return HasRoot(window);
}

bool WindowTree::IsWindowKnownForAccessPolicy(
    const ServerWindow* window) const {
  return IsWindowKnown(window);
}

bool WindowTree::IsWindowRootOfAnotherTreeForAccessPolicy(
    const ServerWindow* window) const {
  WindowTree* tree = window_server_->GetTreeWithRoot(window);
  return tree && tree != this;
}

bool WindowTree::IsWindowCreatedByWindowManager(
    const ServerWindow* window) const {
  // The WindowManager is attached to the root of the Display, if there isn't a
  // WindowManager attached, the window manager didn't create this window.
  const WindowManagerDisplayRoot* display_root =
      GetWindowManagerDisplayRoot(window);
  if (!display_root)
    return false;

  return display_root->window_manager_state()->window_tree()->id() ==
         window->owning_tree_id();
}

bool WindowTree::ShouldInterceptEventsForAccessPolicy(
    const ServerWindow* window) const {
  if (is_for_embedding_)
    return false;

  while (window) {
    // Find the first window created by this client. If there is an embedding,
    // it'll be here.
    if (window->owning_tree_id() == id_) {
      // embedded_tree->embedder_intercepts_events() indicates Embed() was
      // called with kEmbedFlagEmbedderInterceptsEvents. In this case the
      // embedder needs to see the window so that it knows the event is
      // targetted at it.
      WindowTree* embedded_tree = window_server_->GetTreeWithRoot(window);
      if (embedded_tree && embedded_tree->embedder_intercepts_events())
        return true;
      return false;
    }
    window = window->parent();
  }
  return false;
}

void WindowTree::OnDragMoved(const gfx::Point& location) {
  DCHECK(window_server_->in_drag_loop());
  DCHECK_EQ(this, window_server_->GetCurrentDragLoopInitiator());

  ServerWindow* window = window_server_->GetCurrentDragLoopWindow();
  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root)
    return;

  if (drag_move_state_) {
    drag_move_state_->has_queued_drag_window_move = true;
    drag_move_state_->queued_cursor_location = location;
  } else {
    WindowManagerState* wms = display_root->window_manager_state();
    drag_move_state_ = std::make_unique<DragMoveState>();
    wms->window_tree()->window_manager_internal_->WmMoveDragImage(
        location, base::Bind(&WindowTree::OnWmMoveDragImageAck,
                             drag_weak_factory_.GetWeakPtr()));
  }
}

void WindowTree::OnDragCompleted(bool success, uint32_t action_taken) {
  DCHECK(window_server_->in_drag_loop());

  if (window_server_->GetCurrentDragLoopInitiator() != this)
    return;

  uint32_t change_id = window_server_->GetCurrentDragLoopChangeId();
  ServerWindow* window = window_server_->GetCurrentDragLoopWindow();
  WindowManagerDisplayRoot* display_root = GetWindowManagerDisplayRoot(window);
  if (!display_root)
    return;

  window_server_->EndDragLoop();
  WindowManagerState* wms = display_root->window_manager_state();
  wms->EndDragDrop();
  wms->window_tree()->window_manager_internal_->WmDestroyDragImage();
  drag_weak_factory_.InvalidateWeakPtrs();

  client()->OnPerformDragDropCompleted(change_id, success, action_taken);
}

DragTargetConnection* WindowTree::GetDragTargetForWindow(
    const ServerWindow* window) {
  if (!window)
    return nullptr;
  DragTargetConnection* connection = window_server_->GetTreeWithRoot(window);
  if (connection)
    return connection;
  return window_server_->GetTreeWithId(window->owning_tree_id());
}

void WindowTree::PerformOnDragDropStart(
    const base::flat_map<std::string, std::vector<uint8_t>>& mime_data) {
  client()->OnDragDropStart(mime_data);
}

void WindowTree::PerformOnDragEnter(
    const ServerWindow* window,
    uint32_t event_flags,
    const gfx::Point& cursor_offset,
    uint32_t effect_bitmask,
    const base::Callback<void(uint32_t)>& callback) {
  ClientWindowId client_window_id;
  if (!IsWindowKnown(window, &client_window_id)) {
    NOTREACHED();
    callback.Run(0);
    return;
  }
  client()->OnDragEnter(ClientWindowIdToTransportId(client_window_id),
                        event_flags, cursor_offset, effect_bitmask, callback);
}

void WindowTree::PerformOnDragOver(
    const ServerWindow* window,
    uint32_t event_flags,
    const gfx::Point& cursor_offset,
    uint32_t effect_bitmask,
    const base::Callback<void(uint32_t)>& callback) {
  ClientWindowId client_window_id;
  if (!IsWindowKnown(window, &client_window_id)) {
    NOTREACHED();
    callback.Run(0);
    return;
  }
  client()->OnDragOver(ClientWindowIdToTransportId(client_window_id),
                       event_flags, cursor_offset, effect_bitmask, callback);
}

void WindowTree::PerformOnDragLeave(const ServerWindow* window) {
  ClientWindowId client_window_id;
  if (!IsWindowKnown(window, &client_window_id)) {
    NOTREACHED();
    return;
  }
  client()->OnDragLeave(ClientWindowIdToTransportId(client_window_id));
}

void WindowTree::PerformOnCompleteDrop(
    const ServerWindow* window,
    uint32_t event_flags,
    const gfx::Point& cursor_offset,
    uint32_t effect_bitmask,
    const base::Callback<void(uint32_t)>& callback) {
  ClientWindowId client_window_id;
  if (!IsWindowKnown(window, &client_window_id)) {
    NOTREACHED();
    callback.Run(0);
    return;
  }
  client()->OnCompleteDrop(ClientWindowIdToTransportId(client_window_id),
                           event_flags, cursor_offset, effect_bitmask,
                           callback);
}

void WindowTree::PerformOnDragDropDone() {
  client()->OnDragDropDone();
}

}  // namespace ws
}  // namespace ui
