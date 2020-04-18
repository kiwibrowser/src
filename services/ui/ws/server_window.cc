// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/server_window.h"

#include <inttypes.h>
#include <stddef.h>

#include "base/strings/stringprintf.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/host/renderer_settings_creation.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "services/ui/ws/server_window_delegate.h"
#include "services/ui/ws/server_window_observer.h"
#include "services/ui/ws/server_window_tracker.h"
#include "ui/base/cursor/cursor.h"

namespace ui {
namespace ws {

ServerWindow::ServerWindow(ServerWindowDelegate* delegate,
                           const viz::FrameSinkId& frame_sink_id,
                           const Properties& properties)
    : delegate_(delegate),
      owning_tree_id_(frame_sink_id.client_id()),
      // Default to kPointer as kNull doesn't change the cursor, it leaves
      // the last non-null cursor.
      cursor_(ui::CursorType::kPointer),
      non_client_cursor_(ui::CursorType::kPointer),
      properties_(properties),
      // Don't notify newly added observers during notification. This causes
      // problems for code that adds an observer as part of an observer
      // notification (such as ServerWindowDrawTracker).
      observers_(base::ObserverListPolicy::EXISTING_ONLY) {
  DCHECK(delegate);  // Must provide a delegate.
  UpdateFrameSinkId(frame_sink_id);
}

ServerWindow::~ServerWindow() {
  for (auto& observer : observers_)
    observer.OnWindowDestroying(this);

  if (transient_parent_)
    transient_parent_->RemoveTransientWindow(this);

  // Destroy transient children, only after we've removed ourselves from our
  // parent, as destroying an active transient child may otherwise attempt to
  // refocus us.
  Windows transient_children(transient_children_);
  for (auto* window : transient_children)
    delete window;
  DCHECK(transient_children_.empty());

  while (!children_.empty())
    children_.front()->parent()->Remove(children_.front());

  if (parent_)
    parent_->Remove(this);

  for (auto& observer : observers_)
    observer.OnWindowDestroyed(this);

  auto* host_frame_sink_manager = delegate_->GetVizHostProxy();
  if (host_frame_sink_manager)
    host_frame_sink_manager->InvalidateFrameSinkId(frame_sink_id_);
}

void ServerWindow::AddObserver(ServerWindowObserver* observer) {
  observers_.AddObserver(observer);
}

void ServerWindow::RemoveObserver(ServerWindowObserver* observer) {
  DCHECK(observers_.HasObserver(observer));
  observers_.RemoveObserver(observer);
}

bool ServerWindow::HasObserver(ServerWindowObserver* observer) {
  return observers_.HasObserver(observer);
}

void ServerWindow::CreateRootCompositorFrameSink(
    gfx::AcceleratedWidget widget,
    viz::mojom::CompositorFrameSinkAssociatedRequest sink_request,
    viz::mojom::CompositorFrameSinkClientPtr client,
    viz::mojom::DisplayPrivateAssociatedRequest display_request,
    viz::mojom::DisplayClientPtr display_client) {
  has_created_compositor_frame_sink_ = true;
  // TODO(fsamuel): AcceleratedWidget cannot be transported over IPC for Mac
  // or Android. We should instead use GpuSurfaceTracker here on those
  // platforms.

  auto params = viz::mojom::RootCompositorFrameSinkParams::New();
  params->frame_sink_id = frame_sink_id_;
  params->widget = widget;
  params->renderer_settings = viz::CreateRendererSettings();
  params->compositor_frame_sink = std::move(sink_request);
  params->compositor_frame_sink_client = client.PassInterface();
  params->display_private = std::move(display_request);
  params->display_client = display_client.PassInterface();

  delegate_->GetVizHostProxy()->CreateRootCompositorFrameSink(
      std::move(params));
}

void ServerWindow::CreateCompositorFrameSink(
    viz::mojom::CompositorFrameSinkRequest request,
    viz::mojom::CompositorFrameSinkClientPtr client) {
  has_created_compositor_frame_sink_ = true;
  delegate_->GetVizHostProxy()->CreateCompositorFrameSink(
      frame_sink_id_, std::move(request), std::move(client));
}

void ServerWindow::UpdateFrameSinkId(const viz::FrameSinkId& frame_sink_id) {
  DCHECK(frame_sink_id.is_valid());
  auto* host_frame_sink_manager = delegate_->GetVizHostProxy();
  DCHECK(host_frame_sink_manager);
  host_frame_sink_manager->RegisterFrameSinkId(frame_sink_id, this);
  host_frame_sink_manager->SetFrameSinkDebugLabel(frame_sink_id, GetName());
  if (frame_sink_id_.is_valid()) {
    if (parent()) {
      host_frame_sink_manager->UnregisterFrameSinkHierarchy(
          parent()->frame_sink_id(), frame_sink_id_);
      host_frame_sink_manager->RegisterFrameSinkHierarchy(
          parent()->frame_sink_id(), frame_sink_id);
    }
    host_frame_sink_manager->InvalidateFrameSinkId(frame_sink_id_);
  }
  frame_sink_id_ = frame_sink_id;
}

void ServerWindow::Add(ServerWindow* child) {
  // We assume validation checks happened already.
  DCHECK(child);
  DCHECK(child != this);
  DCHECK(!child->Contains(this));
  if (child->parent() == this) {
    if (children_.size() == 1)
      return;  // Already in the right position.
    child->Reorder(children_.back(), mojom::OrderDirection::ABOVE);
    return;
  }

  ServerWindow* old_parent = child->parent();
  for (auto& observer : child->observers_)
    observer.OnWillChangeWindowHierarchy(child, this, old_parent);

  if (child->parent())
    child->parent()->RemoveImpl(child);

  child->parent_ = this;
  children_.push_back(child);

  for (auto& observer : child->observers_)
    observer.OnWindowHierarchyChanged(child, this, old_parent);
}

void ServerWindow::Remove(ServerWindow* child) {
  // We assume validation checks happened else where.
  DCHECK(child);
  DCHECK(child != this);
  DCHECK(child->parent() == this);

  for (auto& observer : child->observers_)
    observer.OnWillChangeWindowHierarchy(child, nullptr, this);

  RemoveImpl(child);

  for (auto& observer : child->observers_)
    observer.OnWindowHierarchyChanged(child, nullptr, this);
}

void ServerWindow::RemoveAllChildren() {
  while (!children_.empty())
    Remove(children_[0]);
}

void ServerWindow::Reorder(ServerWindow* relative,
                           mojom::OrderDirection direction) {
  parent_->children_.erase(
      std::find(parent_->children_.begin(), parent_->children_.end(), this));
  Windows::iterator i =
      std::find(parent_->children_.begin(), parent_->children_.end(), relative);
  if (direction == mojom::OrderDirection::ABOVE) {
    DCHECK(i != parent_->children_.end());
    parent_->children_.insert(++i, this);
  } else if (direction == mojom::OrderDirection::BELOW) {
    DCHECK(i != parent_->children_.end());
    parent_->children_.insert(i, this);
  }
  for (auto& observer : observers_)
    observer.OnWindowReordered(this, relative, direction);
  OnStackingChanged();
}

void ServerWindow::StackChildAtBottom(ServerWindow* child) {
  // There's nothing to do if the child is already at the bottom.
  if (children_.size() <= 1 || child == children_.front())
    return;
  child->Reorder(children_.front(), mojom::OrderDirection::BELOW);
}

void ServerWindow::StackChildAtTop(ServerWindow* child) {
  // There's nothing to do if the child is already at the top.
  if (children_.size() <= 1 || child == children_.back())
    return;
  child->Reorder(children_.back(), mojom::OrderDirection::ABOVE);
}

void ServerWindow::SetBounds(
    const gfx::Rect& bounds,
    const base::Optional<viz::LocalSurfaceId>& local_surface_id) {
  if (bounds_ == bounds && current_local_surface_id_ == local_surface_id)
    return;

  const gfx::Rect old_bounds = bounds_;

  current_local_surface_id_ = local_surface_id;

  bounds_ = bounds;
  for (auto& observer : observers_)
    observer.OnWindowBoundsChanged(this, old_bounds, bounds);
}

void ServerWindow::SetClientArea(
    const gfx::Insets& insets,
    const std::vector<gfx::Rect>& additional_client_areas) {
  if (client_area_ == insets &&
      additional_client_areas == additional_client_areas_) {
    return;
  }

  additional_client_areas_ = additional_client_areas;
  client_area_ = insets;
  for (auto& observer : observers_)
    observer.OnWindowClientAreaChanged(this, insets, additional_client_areas);
}

void ServerWindow::SetHitTestMask(const gfx::Rect& mask) {
  hit_test_mask_ = std::make_unique<gfx::Rect>(mask);
}

void ServerWindow::ClearHitTestMask() {
  hit_test_mask_.reset();
}

void ServerWindow::SetCanAcceptDrops(bool accepts_drops) {
  accepts_drops_ = accepts_drops;
}

const ServerWindow* ServerWindow::GetRootForDrawn() const {
  return delegate_->GetRootWindowForDrawn(this);
}

bool ServerWindow::AddTransientWindow(ServerWindow* child) {
  if (child->transient_parent())
    child->transient_parent()->RemoveTransientWindow(child);

  DCHECK(std::find(transient_children_.begin(), transient_children_.end(),
                   child) == transient_children_.end());
  transient_children_.push_back(child);
  child->transient_parent_ = this;

  for (auto& observer : observers_)
    observer.OnTransientWindowAdded(this, child);
  return true;
}

void ServerWindow::RemoveTransientWindow(ServerWindow* child) {
  Windows::iterator i =
      std::find(transient_children_.begin(), transient_children_.end(), child);
  DCHECK(i != transient_children_.end());
  transient_children_.erase(i);
  DCHECK_EQ(this, child->transient_parent());
  child->transient_parent_ = nullptr;

  for (auto& observer : observers_)
    observer.OnTransientWindowRemoved(this, child);
}

bool ServerWindow::HasTransientAncestor(const ServerWindow* window) const {
  const ServerWindow* transient_ancestor = this;
  while (transient_ancestor && transient_ancestor != window)
    transient_ancestor = transient_ancestor->transient_parent_;
  return transient_ancestor == window;
}

void ServerWindow::SetModalType(ModalType modal_type) {
  if (modal_type_ == modal_type)
    return;

  const ModalType old_modal_type = modal_type_;
  modal_type_ = modal_type;
  for (auto& observer : observers_)
    observer.OnWindowModalTypeChanged(this, old_modal_type);
}

void ServerWindow::SetChildModalParent(ServerWindow* modal_parent) {
  if (modal_parent) {
    child_modal_parent_tracker_ = std::make_unique<ServerWindowTracker>();
    child_modal_parent_tracker_->Add(modal_parent);
  } else {
    child_modal_parent_tracker_.reset();
  }
}

const ServerWindow* ServerWindow::GetChildModalParent() const {
  return child_modal_parent_tracker_ &&
                 !child_modal_parent_tracker_->windows().empty()
             ? *child_modal_parent_tracker_->windows().begin()
             : nullptr;
}

bool ServerWindow::Contains(const ServerWindow* window) const {
  for (const ServerWindow* parent = window; parent; parent = parent->parent_) {
    if (parent == this)
      return true;
  }
  return false;
}

void ServerWindow::SetVisible(bool value) {
  if (visible_ == value)
    return;

  for (auto& observer : observers_)
    observer.OnWillChangeWindowVisibility(this);
  visible_ = value;
  for (auto& observer : observers_)
    observer.OnWindowVisibilityChanged(this);
}

void ServerWindow::SetOpacity(float value) {
  if (value == opacity_)
    return;
  float old_opacity = opacity_;
  opacity_ = value;
  for (auto& observer : observers_)
    observer.OnWindowOpacityChanged(this, old_opacity, opacity_);
}

void ServerWindow::SetCursor(ui::CursorData value) {
  if (cursor_.IsSameAs(value))
    return;
  cursor_ = std::move(value);
  for (auto& observer : observers_)
    observer.OnWindowCursorChanged(this, cursor_);
}

void ServerWindow::SetNonClientCursor(ui::CursorData value) {
  if (non_client_cursor_.IsSameAs(value))
    return;
  non_client_cursor_ = std::move(value);
  for (auto& observer : observers_)
    observer.OnWindowNonClientCursorChanged(this, non_client_cursor_);
}

void ServerWindow::SetTransform(const gfx::Transform& transform) {
  if (transform_ == transform)
    return;

  const gfx::Transform old_transform = transform_;
  transform_ = transform;
  for (auto& observer : observers_)
    observer.OnWindowTransformChanged(this, old_transform, transform);
}

void ServerWindow::SetProperty(const std::string& name,
                               const std::vector<uint8_t>* value) {
  auto it = properties_.find(name);
  if (it != properties_.end()) {
    if (value && it->second == *value)
      return;
  } else if (!value) {
    // This property isn't set in |properties_| and |value| is nullptr, so
    // there's
    // no change.
    return;
  }

  if (value) {
    properties_[name] = *value;
  } else if (it != properties_.end()) {
    properties_.erase(it);
  }
  auto* host_frame_sink_manager = delegate_->GetVizHostProxy();
  if (host_frame_sink_manager && name == mojom::WindowManager::kName_Property)
    host_frame_sink_manager->SetFrameSinkDebugLabel(frame_sink_id_, GetName());

  for (auto& observer : observers_)
    observer.OnWindowSharedPropertyChanged(this, name, value);
}

std::string ServerWindow::GetName() const {
  auto it = properties_.find(mojom::WindowManager::kName_Property);
  if (it == properties_.end())
    return std::string();
  return std::string(it->second.begin(), it->second.end());
}

void ServerWindow::SetTextInputState(const ui::TextInputState& state) {
  const bool changed = !(text_input_state_ == state);
  if (changed) {
    text_input_state_ = state;
    // keyboard even if the state is not changed. So we have to notify
    // |observers_|.
    for (auto& observer : observers_)
      observer.OnWindowTextInputStateChanged(this, state);
  }
}

bool ServerWindow::IsDrawn() const {
  const ServerWindow* root = delegate_->GetRootWindowForDrawn(this);
  if (!root || !root->visible())
    return false;
  const ServerWindow* window = this;
  while (window && window != root && window->visible())
    window = window->parent();
  return root == window;
}

mojom::ShowState ServerWindow::GetShowState() const {
  auto iter = properties_.find(mojom::WindowManager::kShowState_Property);
  if (iter == properties_.end() || iter->second.empty())
    return mojom::ShowState::DEFAULT;

  return static_cast<mojom::ShowState>(iter->second[0]);
}

void ServerWindow::SetUnderlayOffset(const gfx::Vector2d& offset) {
  if (offset == underlay_offset_)
    return;

  underlay_offset_ = offset;
}

void ServerWindow::OnEmbeddedAppDisconnected() {
  for (auto& observer : observers_)
    observer.OnWindowEmbeddedAppDisconnected(this);
}

#if DCHECK_IS_ON()
std::string ServerWindow::GetDebugWindowHierarchy() const {
  std::string result;
  BuildDebugInfo(std::string(), &result);
  return result;
}

std::string ServerWindow::GetDebugWindowInfo() const {
  std::string name = GetName();
  if (name.empty())
    name = "(no name)";

  std::string frame_sink;
  if (has_created_compositor_frame_sink_)
    frame_sink = " [" + frame_sink_id_.ToString() + "]";

  return base::StringPrintf(
      "visible=%s bounds=%s name=%s%s", visible_ ? "true" : "false",
      bounds_.ToString().c_str(), name.c_str(), frame_sink.c_str());
}

void ServerWindow::BuildDebugInfo(const std::string& depth,
                                  std::string* result) const {
  *result +=
      base::StringPrintf("%s%s\n", depth.c_str(), GetDebugWindowInfo().c_str());
  for (const ServerWindow* child : children_)
    child->BuildDebugInfo(depth + "  ", result);
}
#endif  // DCHECK_IS_ON()

void ServerWindow::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  delegate_->OnFirstSurfaceActivation(surface_info, this);
}

void ServerWindow::OnFrameTokenChanged(uint32_t frame_token) {
  // TODO(yiyix, fsamuel): Implement frame token propagation for Mus. See
  // crbug.com/771331
}

void ServerWindow::RemoveImpl(ServerWindow* window) {
  window->parent_ = nullptr;
  children_.erase(std::find(children_.begin(), children_.end(), window));
}

void ServerWindow::OnStackingChanged() {
  if (stacking_target_) {
    Windows::const_iterator window_i = std::find(
        parent()->children().begin(), parent()->children().end(), this);
    DCHECK(window_i != parent()->children().end());
    if (window_i != parent()->children().begin() &&
        (*(window_i - 1) == stacking_target_)) {
      return;
    }
  }
}

}  // namespace ws
}  // namespace ui
