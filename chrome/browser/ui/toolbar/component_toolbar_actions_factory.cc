// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/toolbar/component_toolbar_actions_factory.h"

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/toolbar/media_router_action.h"
#include "chrome/browser/ui/toolbar/media_router_action_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "extensions/browser/extension_registry.h"

// static
const char ComponentToolbarActionsFactory::kMediaRouterActionId[] =
    "media_router_action";

ComponentToolbarActionsFactory::ComponentToolbarActionsFactory(
    Profile* profile) {
  if (media_router::MediaRouterEnabled(profile) &&
      MediaRouterActionController::IsActionShownByPolicy(profile)) {
    initial_ids_.insert(kMediaRouterActionId);
  }
}

ComponentToolbarActionsFactory::~ComponentToolbarActionsFactory() {}

std::set<std::string> ComponentToolbarActionsFactory::GetInitialComponentIds() {
  // TODO(takumif): Instead of keeping track of |initial_ids_|, simplify by
  // checking here whether MediaRouterAction should be visible.
  return initial_ids_;
}

void ComponentToolbarActionsFactory::OnAddComponentActionBeforeInit(
    const std::string& action_id) {
  initial_ids_.insert(action_id);
}

void ComponentToolbarActionsFactory::OnRemoveComponentActionBeforeInit(
    const std::string& action_id) {
  initial_ids_.erase(action_id);
}

std::unique_ptr<ToolbarActionViewController>
ComponentToolbarActionsFactory::GetComponentToolbarActionForId(
    const std::string& action_id,
    Browser* browser,
    ToolbarActionsBar* bar) {
  // Add component toolbar actions here.
  // This current design means that the ComponentToolbarActionsFactory is aware
  // of all actions. Since we should *not* have an excessive amount of these
  // (since each will have an action in the toolbar or overflow menu), this
  // should be okay. If this changes, we should rethink this design to have,
  // e.g., RegisterChromeAction().
  if (action_id == kMediaRouterActionId)
    return std::unique_ptr<ToolbarActionViewController>(
        new MediaRouterAction(browser, bar));

  NOTREACHED();
  return std::unique_ptr<ToolbarActionViewController>();
}
