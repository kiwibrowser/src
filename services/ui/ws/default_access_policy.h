// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_DEFAULT_ACCESS_POLICY_H_
#define SERVICES_UI_WS_DEFAULT_ACCESS_POLICY_H_

#include <stdint.h>

#include "base/macros.h"
#include "services/ui/ws/access_policy.h"

namespace ui {
namespace ws {

class AccessPolicyDelegate;

// AccessPolicy for all clients, except the window manager.
class DefaultAccessPolicy : public AccessPolicy {
 public:
  DefaultAccessPolicy();
  ~DefaultAccessPolicy() override;

  // AccessPolicy:
  void Init(ClientSpecificId client_id,
            AccessPolicyDelegate* delegate) override;
  bool CanRemoveWindowFromParent(const ServerWindow* window) const override;
  bool CanAddWindow(const ServerWindow* parent,
                    const ServerWindow* child) const override;
  bool CanAddTransientWindow(const ServerWindow* parent,
                             const ServerWindow* child) const override;
  bool CanRemoveTransientWindowFromParent(
      const ServerWindow* window) const override;
  bool CanSetModal(const ServerWindow* window) const override;
  bool CanSetChildModalParent(const ServerWindow* window,
                              const ServerWindow* modal_parent) const override;
  bool CanReorderWindow(const ServerWindow* window,
                        const ServerWindow* relative_window,
                        mojom::OrderDirection direction) const override;
  bool CanDeleteWindow(const ServerWindow* window) const override;
  bool CanGetWindowTree(const ServerWindow* window) const override;
  bool CanDescendIntoWindowForWindowTree(
      const ServerWindow* window) const override;
  bool CanEmbed(const ServerWindow* window) const override;
  bool CanChangeWindowVisibility(const ServerWindow* window) const override;
  bool CanChangeWindowOpacity(const ServerWindow* window) const override;
  bool CanSetWindowCompositorFrameSink(
      const ServerWindow* window) const override;
  bool CanSetWindowBounds(const ServerWindow* window) const override;
  bool CanSetWindowTransform(const ServerWindow* window) const override;
  bool CanSetWindowProperties(const ServerWindow* window) const override;
  bool CanSetWindowTextInputState(const ServerWindow* window) const override;
  bool CanSetCapture(const ServerWindow* window) const override;
  bool CanSetFocus(const ServerWindow* window) const override;
  bool CanSetClientArea(const ServerWindow* window) const override;
  bool CanSetHitTestMask(const ServerWindow* window) const override;
  bool CanSetAcceptDrops(const ServerWindow* window) const override;
  bool CanSetEventTargetingPolicy(const ServerWindow* window) const override;
  bool CanStackAbove(const ServerWindow* above,
                     const ServerWindow* below) const override;
  bool CanStackAtTop(const ServerWindow* window) const override;
  bool CanPerformWmAction(const ServerWindow* window) const override;
  bool CanSetCursorProperties(const ServerWindow* window) const override;
  bool CanInitiateDragLoop(const ServerWindow* window) const override;
  bool CanInitiateMoveLoop(const ServerWindow* window) const override;
  bool ShouldNotifyOnHierarchyChange(
      const ServerWindow* window,
      const ServerWindow** new_parent,
      const ServerWindow** old_parent) const override;
  const ServerWindow* GetWindowForFocusChange(
      const ServerWindow* focused) override;
  bool CanSetWindowManager() const override;
  bool IsValidIdForNewWindow(const ClientWindowId& id) const override;

 private:
  bool WasCreatedByThisClient(const ServerWindow* window) const;

  ClientSpecificId client_id_ = 0u;
  AccessPolicyDelegate* delegate_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(DefaultAccessPolicy);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_DEFAULT_ACCESS_POLICY_H_
