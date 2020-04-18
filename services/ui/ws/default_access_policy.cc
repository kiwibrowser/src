// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/default_access_policy.h"

#include "services/ui/ws/access_policy_delegate.h"
#include "services/ui/ws/server_window.h"

namespace ui {
namespace ws {

DefaultAccessPolicy::DefaultAccessPolicy() {}

DefaultAccessPolicy::~DefaultAccessPolicy() {}

void DefaultAccessPolicy::Init(ClientSpecificId client_id,
                               AccessPolicyDelegate* delegate) {
  client_id_ = client_id;
  delegate_ = delegate;
}

bool DefaultAccessPolicy::CanRemoveWindowFromParent(
    const ServerWindow* window) const {
  if (!WasCreatedByThisClient(window))
    return false;  // Can only unparent windows we created.

  return delegate_->HasRootForAccessPolicy(window->parent()) ||
         WasCreatedByThisClient(window->parent());
}

bool DefaultAccessPolicy::CanAddWindow(const ServerWindow* parent,
                                       const ServerWindow* child) const {
  return WasCreatedByThisClient(child) &&
         (delegate_->HasRootForAccessPolicy(parent) ||
          (WasCreatedByThisClient(parent) &&
           !delegate_->IsWindowRootOfAnotherTreeForAccessPolicy(parent)));
}

bool DefaultAccessPolicy::CanAddTransientWindow(
    const ServerWindow* parent,
    const ServerWindow* child) const {
  return (delegate_->HasRootForAccessPolicy(child) ||
          WasCreatedByThisClient(child)) &&
         (delegate_->HasRootForAccessPolicy(parent) ||
          WasCreatedByThisClient(parent));
}

bool DefaultAccessPolicy::CanRemoveTransientWindowFromParent(
    const ServerWindow* window) const {
  return (delegate_->HasRootForAccessPolicy(window) ||
          WasCreatedByThisClient(window)) &&
         (delegate_->HasRootForAccessPolicy(window->transient_parent()) ||
          WasCreatedByThisClient(window->transient_parent()));
}

bool DefaultAccessPolicy::CanSetModal(const ServerWindow* window) const {
  return delegate_->HasRootForAccessPolicy(window) ||
         WasCreatedByThisClient(window);
}

bool DefaultAccessPolicy::CanSetChildModalParent(
    const ServerWindow* window,
    const ServerWindow* modal_parent) const {
  return (delegate_->HasRootForAccessPolicy(window) ||
          WasCreatedByThisClient(window)) &&
         (!modal_parent || delegate_->HasRootForAccessPolicy(modal_parent) ||
          WasCreatedByThisClient(modal_parent));
}

bool DefaultAccessPolicy::CanReorderWindow(
    const ServerWindow* window,
    const ServerWindow* relative_window,
    mojom::OrderDirection direction) const {
  return WasCreatedByThisClient(window) &&
         WasCreatedByThisClient(relative_window);
}

bool DefaultAccessPolicy::CanDeleteWindow(const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool DefaultAccessPolicy::CanGetWindowTree(const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window) ||
         delegate_->ShouldInterceptEventsForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanDescendIntoWindowForWindowTree(
    const ServerWindow* window) const {
  return (WasCreatedByThisClient(window) &&
          !delegate_->IsWindowRootOfAnotherTreeForAccessPolicy(window)) ||
         delegate_->HasRootForAccessPolicy(window) ||
         delegate_->ShouldInterceptEventsForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanEmbed(const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool DefaultAccessPolicy::CanChangeWindowVisibility(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanChangeWindowOpacity(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetWindowCompositorFrameSink(
    const ServerWindow* window) const {
  // Once a window embeds another app, the embedder app is no longer able to
  // call SetWindowSurfaceId() - this ability is transferred to the embedded
  // app.
  if (delegate_->IsWindowRootOfAnotherTreeForAccessPolicy(window))
    return false;
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetWindowBounds(const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool DefaultAccessPolicy::CanSetWindowTransform(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool DefaultAccessPolicy::CanSetWindowProperties(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window);
}

bool DefaultAccessPolicy::CanSetWindowTextInputState(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetCapture(const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetFocus(const ServerWindow* window) const {
  return !window || WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetClientArea(const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetHitTestMask(const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetAcceptDrops(const ServerWindow* window) const {
  return (WasCreatedByThisClient(window) &&
          !delegate_->IsWindowRootOfAnotherTreeForAccessPolicy(window)) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetEventTargetingPolicy(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanStackAbove(const ServerWindow* above,
                                        const ServerWindow* below) const {
  return delegate_->HasRootForAccessPolicy(above) &&
         delegate_->IsWindowCreatedByWindowManager(above) &&
         delegate_->HasRootForAccessPolicy(below) &&
         delegate_->IsWindowCreatedByWindowManager(below);
}

bool DefaultAccessPolicy::CanStackAtTop(const ServerWindow* window) const {
  return delegate_->HasRootForAccessPolicy(window) &&
         delegate_->IsWindowCreatedByWindowManager(window);
}

bool DefaultAccessPolicy::CanPerformWmAction(const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanSetCursorProperties(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanInitiateDragLoop(
    const ServerWindow* window) const {
  return WasCreatedByThisClient(window) ||
         delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::CanInitiateMoveLoop(
    const ServerWindow* window) const {
  return delegate_->HasRootForAccessPolicy(window);
}

bool DefaultAccessPolicy::ShouldNotifyOnHierarchyChange(
    const ServerWindow* window,
    const ServerWindow** new_parent,
    const ServerWindow** old_parent) const {
  if (!WasCreatedByThisClient(window)) {
    // The window may have been exposed to the client because of
    // ShouldInterceptEventsForAccessPolicy(). Notify the parent in this case,
    // but the |old_parent| and/or |new_parent| may need to be updated.
    if (delegate_->IsWindowKnownForAccessPolicy(window)) {
      if (*old_parent && !delegate_->IsWindowKnownForAccessPolicy(*old_parent))
        *old_parent = nullptr;
      if (*new_parent &&
          (!delegate_->IsWindowKnownForAccessPolicy(*new_parent) &&
           delegate_->ShouldInterceptEventsForAccessPolicy(*new_parent))) {
        *new_parent = nullptr;
      }
      return true;
    }

    // Let the client know about |window| if the client intercepts events for
    // |new_parent|.
    if (*new_parent &&
        delegate_->ShouldInterceptEventsForAccessPolicy(*new_parent)) {
      if (*old_parent &&
          !delegate_->ShouldInterceptEventsForAccessPolicy(*old_parent)) {
        *old_parent = nullptr;
      }
      return true;
    }
    return false;
  }

  if (*new_parent && !WasCreatedByThisClient(*new_parent) &&
      !delegate_->HasRootForAccessPolicy((*new_parent))) {
    *new_parent = nullptr;
  }

  if (*old_parent && !WasCreatedByThisClient(*old_parent) &&
      !delegate_->HasRootForAccessPolicy((*old_parent))) {
    *old_parent = nullptr;
  }
  return true;
}

const ServerWindow* DefaultAccessPolicy::GetWindowForFocusChange(
    const ServerWindow* focused) {
  if (WasCreatedByThisClient(focused) ||
      delegate_->HasRootForAccessPolicy(focused))
    return focused;
  return nullptr;
}

bool DefaultAccessPolicy::CanSetWindowManager() const {
  return false;
}

bool DefaultAccessPolicy::WasCreatedByThisClient(
    const ServerWindow* window) const {
  return window->owning_tree_id() == client_id_;
}

bool DefaultAccessPolicy::IsValidIdForNewWindow(
    const ClientWindowId& id) const {
  // Clients using DefaultAccessPolicy only see windows they have created (for
  // the embed point they choose the id), which should have the same client_id
  // as the client_id_ since we should have already filled in the real one.
  return base::checked_cast<ClientSpecificId>(id.client_id()) == client_id_;
}

}  // namespace ws
}  // namespace ui
