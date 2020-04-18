// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_manager_access_policy.h"

#include "services/ui/ws/access_policy_delegate.h"
#include "services/ui/ws/server_window.h"

namespace ui {
namespace ws {

WindowManagerAccessPolicy::WindowManagerAccessPolicy() {}

WindowManagerAccessPolicy::~WindowManagerAccessPolicy() {}

void WindowManagerAccessPolicy::Init(ClientSpecificId client_id,
                                     AccessPolicyDelegate* delegate) {
  client_id_ = client_id;
  delegate_ = delegate;
}

bool WindowManagerAccessPolicy::CanRemoveWindowFromParent(
    const ServerWindow* window) const {
  return true;
}

bool WindowManagerAccessPolicy::CanAddWindow(const ServerWindow* parent,
                                             const ServerWindow* child) const {
  return true;
}

bool WindowManagerAccessPolicy::CanAddTransientWindow(
    const ServerWindow* parent,
    const ServerWindow* child) const {
  return true;
}

bool WindowManagerAccessPolicy::CanRemoveTransientWindowFromParent(
    const ServerWindow* window) const {
  return true;
}

bool WindowManagerAccessPolicy::CanSetModal(
    const ServerWindow* window) const {
  return true;
}

bool WindowManagerAccessPolicy::CanSetChildModalParent(
    const ServerWindow* window,
    const ServerWindow* modal_parent) const {
  return true;
}

bool WindowManagerAccessPolicy::CanReorderWindow(
    const ServerWindow* window,
    const ServerWindow* relative_window,
    mojom::OrderDirection direction) const {
  return true;
}

bool WindowManagerAccessPolicy::CanDeleteWindow(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool WindowManagerAccessPolicy::CanGetWindowTree(
    const ServerWindow* window) const {
  return true;
}

bool WindowManagerAccessPolicy::CanDescendIntoWindowForWindowTree(
    const ServerWindow* window) const {
  return true;
}

bool WindowManagerAccessPolicy::CanEmbed(const ServerWindow* window) const {
  return !delegate_->HasRootForAccessPolicy(window);
}

bool WindowManagerAccessPolicy::CanChangeWindowVisibility(
    const ServerWindow* window) const {
  if (WasCreatedByThisClient(window))
    return true;
  // The WindowManager can change the visibility of the WindowManager root.
  const ServerWindow* root = window->GetRootForDrawn();
  return root && window->parent() == root;
}

bool WindowManagerAccessPolicy::CanChangeWindowOpacity(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool WindowManagerAccessPolicy::CanSetWindowCompositorFrameSink(
    const ServerWindow* window) const {
  if (delegate_->IsWindowRootOfAnotherTreeForAccessPolicy(window))
    return false;

  return WasCreatedByThisClient(window) ||
         (delegate_->HasRootForAccessPolicy(window));
}

bool WindowManagerAccessPolicy::CanSetWindowBounds(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool WindowManagerAccessPolicy::CanSetWindowTransform(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool WindowManagerAccessPolicy::CanSetWindowProperties(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool WindowManagerAccessPolicy::CanSetWindowTextInputState(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool WindowManagerAccessPolicy::CanSetCapture(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool WindowManagerAccessPolicy::CanSetFocus(const ServerWindow* window) const {
  return true;
}

bool WindowManagerAccessPolicy::CanSetClientArea(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool WindowManagerAccessPolicy::CanSetHitTestMask(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool WindowManagerAccessPolicy::CanSetAcceptDrops(
    const ServerWindow* window) const {
  return true;
}

bool WindowManagerAccessPolicy::CanSetEventTargetingPolicy(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool WindowManagerAccessPolicy::CanStackAbove(
    const ServerWindow* above,
    const ServerWindow* below) const {
  // This API is for clients. Window managers can perform any arbitrary
  // reordering of the windows and don't need to go through this constrained
  // API.
  return false;
}

bool WindowManagerAccessPolicy::CanStackAtTop(
    const ServerWindow* window) const {
  // This API is for clients. Window managers can perform any arbitrary
  // reordering of the windows and don't need to go through this constrained
  // API.
  return false;
}

bool WindowManagerAccessPolicy::CanPerformWmAction(
    const ServerWindow* window) const {
  // This API is for clients. Window managers don't need to tell themselves to
  // do things.
  return false;
}

bool WindowManagerAccessPolicy::CanSetCursorProperties(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool WindowManagerAccessPolicy::CanInitiateDragLoop(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool WindowManagerAccessPolicy::CanInitiateMoveLoop(
    const ServerWindow* window) const {
  return false;
}

bool WindowManagerAccessPolicy::ShouldNotifyOnHierarchyChange(
    const ServerWindow* window,
    const ServerWindow** new_parent,
    const ServerWindow** old_parent) const {
  // Notify if we've already told the window manager about the window, or if
  // we've
  // already told the window manager about the parent. The later handles the
  // case of a window that wasn't parented to the root getting added to the
  // root.
  return IsWindowKnown(window) || (*new_parent && IsWindowKnown(*new_parent));
}

bool WindowManagerAccessPolicy::CanSetWindowManager() const {
  return true;
}

const ServerWindow* WindowManagerAccessPolicy::GetWindowForFocusChange(
    const ServerWindow* focused) {
  return focused;
}

bool WindowManagerAccessPolicy::IsWindowKnown(
    const ServerWindow* window) const {
  return delegate_->IsWindowKnownForAccessPolicy(window);
}

bool WindowManagerAccessPolicy::IsValidIdForNewWindow(
    const ClientWindowId& id) const {
  // The WindowManager see windows created from other clients. If the WM doesn't
  // use the client id when creating windows the WM could end up with two
  // windows with the same id. Because of this the wm must use the same
  // client id for all windows it creates.
  return base::checked_cast<ClientSpecificId>(id.client_id()) == client_id_;
}

bool WindowManagerAccessPolicy::WasCreatedByThisClient(
    const ServerWindow* window) const {
  return window->owning_tree_id() == client_id_;
}

}  // namespace ws
}  // namespace ui
